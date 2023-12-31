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

static List<Rect> pushed_scissors;
Rect              current_scissor_rect;

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

void draw_init() {
    commands.allocator = default_allocator();
    vertices.allocator = default_allocator();
    pushed_layers.allocator = default_allocator();
    pushed_scissors.allocator = default_allocator();
}

void draw_update() {
    current_scissor_rect = full_screen_rect();
}

int64_t draw_get_next_serial() {
    last_serial += 1;
    return last_serial;
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

void draw_push_scissor(Rect rect) {
    pushed_scissors.add(current_scissor_rect);
    Draw_Command *cmd = commands.add_count(1);
    cmd->layer = current_draw_layer;
    cmd->serial = draw_get_next_serial();
    cmd->scissor = true;
    cmd->scissor_rect = rect;
    current_scissor_rect = rect;
}

void draw_pop_scissor() {
    current_scissor_rect = pushed_scissors.pop();
    Draw_Command *cmd = commands.add_count(1);
    cmd->layer = current_draw_layer;
    cmd->serial = draw_get_next_serial();
    cmd->scissor = true;
    cmd->scissor_rect = current_scissor_rect;
}

Draw_Command *draw_quad(Rect rect, HMM_Vec4 color) {
    return draw_quad(rect.min, rect.max, color);
}

Draw_Command *draw_quad(HMM_Vec2 min, HMM_Vec2 max, HMM_Vec4 color) {
    Draw_Command *cmd = commands.add_count(1);
    cmd->layer = current_draw_layer;
    cmd->min = min;
    cmd->max = max;
    cmd->color = color;
    cmd->serial = draw_get_next_serial();
    return cmd;
}

static int compare_draw_commands(const void *_a, const void *_b) {
    const Draw_Command *a = (const Draw_Command *)_a;
    const Draw_Command *b = (const Draw_Command *)_b;
    if (a->layer == b->layer) {
        return a->serial - b->serial;
    }
    return a->layer - b->layer;
}

struct Batch_Region {
    int64_t first_vertex;
    int64_t vertex_count;
};

void draw_flush() {
    qsort(commands.data, commands.count, sizeof(Draw_Command), compare_draw_commands);

    List<Batch_Region> batches = make_list<Batch_Region>(temp(), commands.count);
    FOR (i, 0, commands.count-1) {
        Draw_Command *cmd = &commands[i];
        if (cmd->scissor) {
            continue;
        }

        HMM_Vec2 p1 = cmd->min;
        HMM_Vec2 p2 = {cmd->min.X, cmd->max.Y};
        HMM_Vec2 p3 = cmd->max;
        HMM_Vec2 p4 = {cmd->max.X, cmd->min.Y};

        Vertex *quad_vertices = vertices.add_count(6);
        quad_vertices[0] = {{p1.X, p1.Y, 0, 1}, cmd->color};
        quad_vertices[1] = {{p3.X, p3.Y, 0, 1}, cmd->color};
        quad_vertices[2] = {{p2.X, p2.Y, 0, 1}, cmd->color};
        quad_vertices[3] = {{p1.X, p1.Y, 0, 1}, cmd->color};
        quad_vertices[4] = {{p4.X, p4.Y, 0, 1}, cmd->color};
        quad_vertices[5] = {{p3.X, p3.Y, 0, 1}, cmd->color};
    }

    maybe_resize_vertex_buffer(vertices.count);
    sg_update_buffer(vertex_buffer, {vertices.data, sizeof(Vertex) * vertices.count});

    sg_bindings bindings = {};
    bindings.vertex_buffers[0] = vertex_buffer;
    sg_apply_bindings(&bindings);

    int64_t vertex_cursor = 0;
    FOR (i, 0, commands.count-1) {
        Draw_Command *cmd = &commands[i];
        if (cmd->scissor) {
            Rect sr = cmd->scissor_rect;
            sg_apply_scissor_rectf(sr.min.X, sr.min.Y, sr.width(), sr.height(), false);
        }
        else {
            int64_t first_vertex = vertex_cursor;
            int64_t vertex_count = 0;
            while (i < commands.count && !commands[i].scissor) {
                vertex_count += 6;
                i += 1;
            }
            sg_draw(first_vertex, vertex_count, 1);
            vertex_cursor += vertex_count;
            i -= 1;
        }
    }

    last_serial = 0;
    commands.reset();
    vertices.reset();
}