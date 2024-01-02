#include "draw.h"
#include "core.h"
#include "ui.h"

static int64_t last_serial;

static List<Draw_Command> commands;
static List<Vertex>       vertices;

static sg_buffer vertex_buffer;
static int64_t   vertex_buffer_capacity;

static List<int64_t> pushed_layers;
int64_t              current_draw_layer;

static List<HMM_Vec4> pushed_colors;
HMM_Vec4              current_color_multiplier;

static List<Rect> pushed_scissors;
Rect              current_scissor_rect;

static List<Font> all_fonts;

static List<int64_t> queued_serials;

static sg_image white_image;

static sg_sampler linear_clamp_sampler;
static sg_sampler linear_repeat_sampler;

static void maybe_resize_vertex_buffer(int64_t required) {
    if (required <= vertex_buffer_capacity) {
        return;
    }
    if (vertex_buffer.id) {
        sg_destroy_buffer(vertex_buffer);
    }
    sg_buffer_desc buffer_desc = {};
    buffer_desc.size = sizeof(Vertex) * required;
    buffer_desc.label = "draw vertices";
    buffer_desc.usage = SG_USAGE_STREAM;
    vertex_buffer = sg_make_buffer(&buffer_desc);
    vertex_buffer_capacity = required;
}

sg_pipeline textured_pipeline;
sg_pipeline text_pipeline;

void draw_init() {
    commands.allocator = default_allocator();
    vertices.allocator = default_allocator();
    pushed_layers.allocator = default_allocator();
    pushed_scissors.allocator = default_allocator();
    pushed_colors.allocator = default_allocator();
    all_fonts.allocator = default_allocator();
    queued_serials.allocator = default_allocator();

    // make white image
    uint8_t white_image_data[] = {255, 255, 255, 255};
    sg_image_desc white_image_desc = {};
    white_image_desc.type = SG_IMAGETYPE_2D;
    white_image_desc.width = 1;
    white_image_desc.height = 1;
    white_image_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    white_image_desc.data.subimage[0][0] = SG_RANGE(white_image_data);
    white_image = sg_make_image(&white_image_desc);

    // make samplers
    sg_sampler_desc linear_clamp_sampler_desc = {};
    linear_clamp_sampler_desc.min_filter = SG_FILTER_LINEAR;
    linear_clamp_sampler_desc.mag_filter = SG_FILTER_LINEAR;
    linear_clamp_sampler_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    linear_clamp_sampler_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    linear_clamp_sampler = sg_make_sampler(&linear_clamp_sampler_desc);

    sg_sampler_desc linear_repeat_sampler_desc = linear_clamp_sampler_desc;
    linear_repeat_sampler_desc.wrap_u = SG_WRAP_REPEAT;
    linear_repeat_sampler_desc.wrap_v = SG_WRAP_REPEAT;
    linear_repeat_sampler = sg_make_sampler(&linear_repeat_sampler_desc);

    // textured pipeline
    {
        sg_shader_desc shader_desc = {};
        shader_desc.label = "simple shader";
        shader_desc.attrs[0].sem_name = "POS";
        shader_desc.attrs[1].sem_name = "UV";
        shader_desc.attrs[2].sem_name = "COLOR";
        shader_desc.vs.uniform_blocks[0].size = sizeof(HMM_Mat4);
        shader_desc.vs.uniform_blocks[0].uniforms[0] = {"mvp", SG_UNIFORMTYPE_MAT4, 1};
        shader_desc.vs.source = R"DONE(#version 300 es
            layout(location=0) in vec4 in_pos;
            layout(location=1) in vec2 in_uv;
            layout(location=2) in vec4 in_color;
            out vec2 fs_uv;
            out vec4 fs_color;
            uniform mat4 mvp;
            void main() {
                gl_Position = mvp * in_pos;
                fs_uv = in_uv;
                fs_color = in_color;
            }
            )DONE";
        shader_desc.fs.source = R"DONE(#version 300 es
            precision mediump float;
            in vec2 fs_uv;
            in vec4 fs_color;
            out vec4 FragColor;
            void main() {
                FragColor = fs_color;
            }
            )DONE";

        sg_shader shd = sg_make_shader(&shader_desc);
        assert(shd.id != SG_INVALID_ID);

        sg_pipeline_desc pipeline_desc = {};
        pipeline_desc.label = "textured pipeline";
        pipeline_desc.shader = shd;
        pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4;
        pipeline_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
        pipeline_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT4;
        pipeline_desc.blend_color = {1, 1, 1, 1},
        pipeline_desc.colors[0].blend.enabled = true;
        pipeline_desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
        pipeline_desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        pipeline_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA;
        pipeline_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        textured_pipeline = sg_make_pipeline(&pipeline_desc);
    }

    // text pipeline
    {
        sg_shader_desc shader_desc = {};
        shader_desc.label = "text shader";
        shader_desc.attrs[0].sem_name = "POS";
        shader_desc.attrs[1].sem_name = "UV";
        shader_desc.attrs[2].sem_name = "COLOR";
        shader_desc.vs.uniform_blocks[0].size = sizeof(HMM_Mat4);
        shader_desc.vs.uniform_blocks[0].uniforms[0] = {"mvp", SG_UNIFORMTYPE_MAT4, 1};
        shader_desc.vs.source = R"DONE(#version 300 es
            layout(location=0) in vec4 in_pos;
            layout(location=1) in vec2 in_uv;
            layout(location=2) in vec4 in_color;
            out vec2 fs_uv;
            out vec4 fs_color;
            uniform mat4 mvp;
            void main() {
                gl_Position = mvp * in_pos;
                fs_uv = in_uv;
                fs_color = in_color;
            }
            )DONE";
        shader_desc.fs.images[0].used = true;
        shader_desc.fs.images[0].image_type = SG_IMAGETYPE_2D;
        shader_desc.fs.samplers[0].used = true;
        // shader_desc.fs.samplers[0].sampler_type = SG_IMAGETYPE_2D;
        shader_desc.fs.image_sampler_pairs[0].used = true;
        shader_desc.fs.image_sampler_pairs[0].image_slot = 0;
        shader_desc.fs.image_sampler_pairs[0].sampler_slot = 0;
        shader_desc.fs.image_sampler_pairs[0].glsl_name = "tex";
        // shader_desc.fs.images[0].name = "tex";
        shader_desc.fs.source = R"DONE(#version 300 es
            precision mediump float;
            uniform sampler2D tex;
            in vec2 fs_uv;
            in vec4 fs_color;
            out vec4 FragColor;
            void main() {
                float v = texture(tex, fs_uv).r;
                FragColor = vec4(1, 1, 1, v) * fs_color;
            }
            )DONE";

        sg_shader shd = sg_make_shader(&shader_desc);
        assert(shd.id != SG_INVALID_ID);

        sg_pipeline_desc pipeline_desc = {};
        pipeline_desc.label = "text pipeline";
        pipeline_desc.shader = shd;
        pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4;
        pipeline_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
        pipeline_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT4;
        pipeline_desc.blend_color = {1, 1, 1, 1},
        pipeline_desc.colors[0].blend.enabled = true;
        pipeline_desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
        pipeline_desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        pipeline_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA;
        pipeline_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        text_pipeline = sg_make_pipeline(&pipeline_desc);
    }
}

void draw_update() {
    current_scissor_rect = full_screen_rect();
    current_color_multiplier = v4(1, 1, 1, 1);
}

int64_t draw_get_next_serial() {
    if (queued_serials.count > 0) {
        return queued_serials.pop();
    }
    last_serial += 1;
    return last_serial;
}

void draw_set_next_serial(int64_t s) {
    queued_serials.add(s);
}

void draw_push_layer(int64_t layer) {
    pushed_layers.add(current_draw_layer);
    current_draw_layer = layer;
}

void draw_push_layer_relative(int64_t delta) {
    pushed_layers.add(current_draw_layer);
    current_draw_layer = current_draw_layer + delta;
}

void draw_pop_layer() {
    current_draw_layer = pushed_layers.pop();
}

void draw_push_color_multiplier(HMM_Vec4 color) {
    pushed_colors.add(current_color_multiplier);
    current_color_multiplier = color;
}

void draw_pop_color_multiplier() {
    current_color_multiplier = pushed_colors.pop();
}

Rect draw_clip_rect_to_current_scissor(Rect rect) {
    rect.min.X = FMAX(rect.min.X, current_scissor_rect.min.X);
    rect.min.X = FMIN(rect.min.X, current_scissor_rect.max.X);
    rect.min.Y = FMAX(rect.min.Y, current_scissor_rect.min.Y);
    rect.min.Y = FMIN(rect.min.Y, current_scissor_rect.max.Y);
    rect.max.X = FMIN(rect.max.X, current_scissor_rect.max.X);
    rect.max.X = FMAX(rect.max.X, current_scissor_rect.min.X);
    rect.max.Y = FMIN(rect.max.Y, current_scissor_rect.max.Y);
    rect.max.Y = FMAX(rect.max.Y, current_scissor_rect.min.Y);
    return rect;
}

void draw_push_scissor(Rect rect) {
    pushed_scissors.add(current_scissor_rect);
    rect = draw_clip_rect_to_current_scissor(rect);
    Draw_Command *cmd = commands.add_count(1);
    cmd->kind = Draw_Command_Kind::SCISSOR;
    cmd->layer = current_draw_layer;
    cmd->serial = draw_get_next_serial();
    cmd->scissor.rect = rect;
    current_scissor_rect = rect;
}

void draw_pop_scissor() {
    current_scissor_rect = pushed_scissors.pop();
    Draw_Command *cmd = commands.add_count(1);
    cmd->kind = Draw_Command_Kind::SCISSOR;
    cmd->layer = current_draw_layer;
    cmd->serial = draw_get_next_serial();
    cmd->scissor.rect = current_scissor_rect;
}

Draw_Command *draw_quad(Rect rect, HMM_Vec4 color) {
    return draw_quad(rect.min, rect.max, color);
}

Draw_Command *draw_quad(HMM_Vec2 min, HMM_Vec2 max, HMM_Vec4 color) {
    Draw_Command *cmd = commands.add_count(1);
    cmd->kind = Draw_Command_Kind::QUAD;
    cmd->layer = current_draw_layer;
    cmd->min = min;
    cmd->max = max;
    cmd->color = color * current_color_multiplier;
    cmd->serial = draw_get_next_serial();
    cmd->pipeline = textured_pipeline;
    return cmd;
}

Draw_Command *draw_text(String text, HMM_Vec2 position, Font *font, HMM_Vec4 color) {
    Draw_Command *cmd = commands.add_count(1);
    cmd->kind = Draw_Command_Kind::TEXT;
    cmd->layer = current_draw_layer;
    cmd->min = position;
    cmd->max = position;
    cmd->color = color * current_color_multiplier;
    cmd->image = font->image;
    cmd->pipeline = text_pipeline;
    // cmd->sampler = linear_clamp_sampler;
    cmd->text.font = font;
    cmd->text.string = text;
    cmd->text.position = position;
    cmd->serial = draw_get_next_serial();
    return cmd;
}

Font *load_font_from_file(const char *filepath, int64_t size) {
    Font *result = all_fonts.add_count(1);

    FILE *file = fopen(filepath, "rb");
    assert(file != nullptr);

    fseek(file, 0, SEEK_END);
    int64_t file_size = ftell(file);
    rewind(file);
    unsigned char *ttf_data = (unsigned char *)alloc(default_allocator(), file_size + 1, size, false);
    defer (free(default_allocator(), ttf_data));
    fread(ttf_data, 1, file_size, file);
    ttf_data[file_size] = 0;
    fclose(file);
    int stbtt_result = 0;
    int dim = 32;
    uint8_t *font_bitmap = nullptr;
    do {
        dim *= 2;
        if (font_bitmap != nullptr) free(default_allocator(), font_bitmap);
        font_bitmap = (uint8_t *)alloc(default_allocator(), dim * dim, size, true);
        stbtt_result = stbtt_BakeFontBitmap(ttf_data, 0, (float)size, font_bitmap, dim, dim, 32, 96, result->chars);
    } while (stbtt_result <= 0);
    defer (free(default_allocator(), font_bitmap));

    stbtt_fontinfo font_info = {};
    stbtt_InitFont(&font_info, ttf_data, 0);

    int ascent, descent, line_height;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_height);
    float scale = stbtt_ScaleForPixelHeight(&font_info, (float)size);
    line_height = ascent - descent + line_height;

    sg_image_desc desc = {};
    desc.type = SG_IMAGETYPE_2D;
    desc.width = dim;
    desc.height = dim;
    desc.pixel_format = SG_PIXELFORMAT_R8;
    desc.data.subimage[0][0] = {font_bitmap, (size_t)(dim * dim)};
    result->image = sg_make_image(&desc);
    result->size = size;
    result->bitmap_dim = dim;
    result->ascender = (int64_t)((float)ascent * scale);
    result->descender = (int64_t)((float)descent * scale);
    result->line_height = (int64_t)((float)line_height * scale);
    return result;
}

float calculate_text_width(String text, Font *font) {
    HMM_Vec2 position = {};
    FOR (i, 0, text.count-1) {
        char c = text[i];
        if (c >= 32 && c < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->chars, (int)font->bitmap_dim, (int)font->bitmap_dim, c-32, &position.X, &position.Y, &q, 1);
       }
    }
    return position.X;
}

static int compare_draw_commands(const void *_a, const void *_b) {
    const Draw_Command *a = (const Draw_Command *)_a;
    const Draw_Command *b = (const Draw_Command *)_b;
    if (a->layer == b->layer) {
        return (int)(a->serial - b->serial);
    }
    return (int)(a->layer - b->layer);
}

struct Batch_Region {
    int64_t  first_vertex;
    int64_t  vertex_count;
    Draw_Command *cmd;
    bool skip;
};

void draw_flush() {
    if (commands.count == 0) return;

    qsort(commands.data, commands.count, sizeof(Draw_Command), compare_draw_commands);

    List<Batch_Region> batch_regions = make_list<Batch_Region>(temp());
    FOR (i, 0, commands.count-1) {
        Draw_Command *cmd = &commands[i];
        Batch_Region region = {};
        region.first_vertex = vertices.count;
        region.cmd = cmd;

        HMM_Vec2 p1 = cmd->min;
        HMM_Vec2 p2 = {cmd->min.X, cmd->max.Y};
        HMM_Vec2 p3 = cmd->max;
        HMM_Vec2 p4 = {cmd->max.X, cmd->min.Y};

        if (cmd->kind == Draw_Command_Kind::QUAD) {
            Vertex *quad_vertices = vertices.add_count(6);
            quad_vertices[0] = {{p1.X, p1.Y, 0, 1}, {0, 0}, cmd->color};
            quad_vertices[1] = {{p3.X, p3.Y, 0, 1}, {0, 0}, cmd->color};
            quad_vertices[2] = {{p2.X, p2.Y, 0, 1}, {0, 0}, cmd->color};
            quad_vertices[3] = {{p1.X, p1.Y, 0, 1}, {0, 0}, cmd->color};
            quad_vertices[4] = {{p4.X, p4.Y, 0, 1}, {0, 0}, cmd->color};
            quad_vertices[5] = {{p3.X, p3.Y, 0, 1}, {0, 0}, cmd->color};
        }
        else if (cmd->kind == Draw_Command_Kind::TEXT) {
            p1.Y = sapp_height() - p1.Y;
            FOR (j, 0, cmd->text.string.count-1) {
                char c = cmd->text.string[j];
                if (c >= 32 && c < 128) {
                    stbtt_aligned_quad q;
                    stbtt_GetBakedQuad(cmd->text.font->chars, (int)cmd->text.font->bitmap_dim, (int)cmd->text.font->bitmap_dim, c-32, &p1.X, &p1.Y, &q, 1);
                    q.y0 = sapp_height() - q.y0;
                    q.y1 = sapp_height() - q.y1;
                    Vertex *char_vertices = vertices.add_count(6);
                    char_vertices[0] = {{q.x0, q.y1, 0, 1}, {q.s0, q.t1, 0, 0}, cmd->color};
                    char_vertices[1] = {{q.x1, q.y0, 0, 1}, {q.s1, q.t0, 0, 0}, cmd->color};
                    char_vertices[2] = {{q.x0, q.y0, 0, 1}, {q.s0, q.t0, 0, 0}, cmd->color};
                    char_vertices[3] = {{q.x0, q.y1, 0, 1}, {q.s0, q.t1, 0, 0}, cmd->color};
                    char_vertices[4] = {{q.x1, q.y1, 0, 1}, {q.s1, q.t1, 0, 0}, cmd->color};
                    char_vertices[5] = {{q.x1, q.y0, 0, 1}, {q.s1, q.t0, 0, 0}, cmd->color};
               }
            }
        }

        region.vertex_count = vertices.count - region.first_vertex;
        batch_regions.add(region);
    }

    assert(batch_regions.count > 0);
    Batch_Region *current_batch_region = &batch_regions[0];
    FOR (i, 1, batch_regions.count-1) {
        Batch_Region *region = &batch_regions[i];
        bool can_batch = true;
        if (region->cmd->kind != current_batch_region->cmd->kind) can_batch = false;
        else if (region->cmd->image.id != current_batch_region->cmd->image.id) can_batch = false;
        else if (region->cmd->pipeline.id != current_batch_region->cmd->pipeline.id) can_batch = false;
        else if (region->cmd->kind == Draw_Command_Kind::SCISSOR) can_batch = false;
        else if (region->cmd->kind == Draw_Command_Kind::TEXT && region->cmd->text.font != current_batch_region->cmd->text.font) can_batch = false;

        if (can_batch) {
            assert(region->cmd->kind != Draw_Command_Kind::QUAD || region->cmd->kind != Draw_Command_Kind::TEXT);
            current_batch_region->vertex_count += region->vertex_count;
            region->skip = true;
        }
        else {
            current_batch_region = region;
        }
    }

    maybe_resize_vertex_buffer(vertices.count);
    sg_update_buffer(vertex_buffer, {vertices.data, sizeof(Vertex) * vertices.count});

    FOR (i, 0, batch_regions.count-1) {
        Batch_Region *region = &batch_regions[i];
        if (region->skip) {
            continue;
        }
        if (region->cmd->pipeline.id != 0) {
            sg_apply_pipeline(region->cmd->pipeline);
            HMM_Mat4 screen_proj = HMM_Orthographic_LH_ZO(0, sapp_widthf(), 0, sapp_heightf(), -1000, 1000);
            sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(screen_proj));
            sg_bindings bindings = {};
            bindings.vertex_buffers[0] = vertex_buffer;
            if (region->cmd->image.id != 0) {
                bindings.fs.images[0] = region->cmd->image;
                bindings.fs.samplers[0] = linear_clamp_sampler;
            }
            sg_apply_bindings(&bindings);
        }
        switch (region->cmd->kind) {
            case Draw_Command_Kind::QUAD: {
                sg_draw((int)region->first_vertex, (int)region->vertex_count, 1);
                break;
            }
            case Draw_Command_Kind::TEXT: {
                sg_draw((int)region->first_vertex, (int)region->vertex_count, 1);
                break;
            }
            case Draw_Command_Kind::SCISSOR: {
                Rect sr = region->cmd->scissor.rect;
                sg_apply_scissor_rectf(sr.min.X, sr.min.Y, sr.width(), sr.height(), false);
                break;
            }
            default: {
                assert(false);
            }
        }
    }

    last_serial = 0;
    commands.reset();
    vertices.reset();
}