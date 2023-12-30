#include "draw.h"
#include "core.h"

static int64_t last_serial;

static List<Draw_Command> commands;
static List<Vertex>       vertices;

static sg_buffer draw_vertices;
static int64_t   draw_vertices_capacity;

static Array<1024, int64_t> pushed_layers;
static int64_t pushed_layer_count;
int64_t current_draw_layer;

static void maybe_resize_vertex_buffer(int64_t required) {
    if (required > draw_vertices_capacity) {
        if (draw_vertices.id) {
            sg_destroy_buffer(draw_vertices);
        }
        sg_buffer_desc buffer_desc = {};
        buffer_desc.size = sizeof(Vertex) * required;
        buffer_desc.label = "draw vertices";
        buffer_desc.usage = SG_USAGE_STREAM;
        draw_vertices = sg_make_buffer(&buffer_desc);
        draw_vertices_capacity = required;
    }
}

void draw_init() {
    commands.allocator = default_allocator();
    vertices.allocator = default_allocator();
}

int64_t draw_get_next_serial() {
    last_serial += 1;
    return last_serial;
}

void draw_push_layer(int64_t layer) {
    pushed_layers[pushed_layer_count++] = current_draw_layer;
    current_draw_layer = layer;
}

void draw_push_layer_relative(int64_t delta) {
    pushed_layers[pushed_layer_count++] = current_draw_layer;
    current_draw_layer = current_draw_layer + delta;
}

void draw_pop_layer() {
    assert(pushed_layer_count > 0);
    pushed_layer_count -= 1;
    current_draw_layer = pushed_layers[pushed_layer_count];
}

Draw_Command *draw_quad(Rect rect, HMM_Vec4 color) {
    return draw_quad(rect.min, rect.max, color);
}

Draw_Command *draw_quad(HMM_Vec2 min, HMM_Vec2 max, HMM_Vec4 color) {
    Draw_Command *cmd = commands.add();
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

void draw_flush() {
    qsort(commands.data, commands.count, sizeof(Draw_Command), compare_draw_commands);

    FOR (i, 0, commands.count-1) {
        Draw_Command *cmd = &commands[i];

        HMM_Vec2 p1 = cmd->min;
        HMM_Vec2 p2 = {cmd->min.X, cmd->max.Y};
        HMM_Vec2 p3 = cmd->max;
        HMM_Vec2 p4 = {cmd->max.X, cmd->min.Y};

        Vertex *quad_vertices = vertices.add(6);
        quad_vertices[0] = {{p1.X, p1.Y, 0, 1}, cmd->color};
        quad_vertices[1] = {{p3.X, p3.Y, 0, 1}, cmd->color};
        quad_vertices[2] = {{p2.X, p2.Y, 0, 1}, cmd->color};
        quad_vertices[3] = {{p1.X, p1.Y, 0, 1}, cmd->color};
        quad_vertices[4] = {{p4.X, p4.Y, 0, 1}, cmd->color};
        quad_vertices[5] = {{p3.X, p3.Y, 0, 1}, cmd->color};
    }

    maybe_resize_vertex_buffer(vertices.count);
    sg_update_buffer(draw_vertices, {vertices.data, sizeof(Vertex) * vertices.count});

    sg_bindings bindings = {};
    bindings.vertex_buffers[0] = draw_vertices;
    sg_apply_bindings(&bindings);

    sg_draw(0, vertices.count, 1);

    last_serial = 0;
    commands.reset();
    vertices.reset();
}