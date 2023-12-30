#include "core.h"

#include <cstdarg>

////////////////////////////////////////////////////////////////////////////////

Arena *bootstrap_arena(Allocator backing_allocator, int64_t capacity) {
    uint8_t *memory = (uint8_t *)alloc(backing_allocator, capacity, 16, false);
    Arena *arena = (Arena *)memory;
    *arena = {};
    arena->memory = memory;
    arena->capacity = capacity;
    arena->start = sizeof(Arena);
    arena->cursor = arena->start;
    return arena;
}

static void *arena_allocator_proc(void *data, void *old_ptr, int64_t size, int64_t align, Allocator_Mode mode) {
    if (mode == ALLOCATOR_MODE_ALLOC) {
        if (size == 0) {
            return nullptr;
        }
        Arena *arena = (Arena *)data;
        arena->cursor = align_forward(arena->cursor, align);
        uint8_t *result = arena->memory + arena->cursor;
        arena->cursor += size;
        assert(arena->cursor <= arena->capacity);
        return result;
    }
    else {
        assert(mode == ALLOCATOR_MODE_FREE);
        // freeing from arenas does nothing
        return nullptr;
    }
}

Allocator Arena::allocator() {
    return {this, arena_allocator_proc};
}

void Arena::reset() {
    cursor = start;
}

void Arena::destroy() {
    free(backing_allocator, memory);
}

////////////////////////////////////////////////////////////////////////////////

Array<16 * 1024 * 1024, uint8_t> per_frame_arena;
int64_t per_frame_arena_cursor;

void *talloc(int64_t size) {
    per_frame_arena_cursor = align_forward(per_frame_arena_cursor, 16);
    void *result = &per_frame_arena[per_frame_arena_cursor];
    per_frame_arena_cursor += size;
    return result;
}

////////////////////////////////////////////////////////////////////////////////

bool String::operator ==(String b) {
    if (count != b.count) {
        return false;
    }
    if (data == b.data) {
        return true;
    }
    for (int64_t i = 0; i < count; i++) {
        if (data[i] != b.data[i]) {
            return false;
        }
    }
    return true;
}

uint8_t &String::operator [](int64_t index) {
    assert(index >= 0);
    assert(index < count);
    return data[index];
}

String tprint(char* format, ...) {
    va_list args;

    va_start(args, format);
    int count = vsnprintf(nullptr, 0, format, args);
    va_end(args);

    char *cstr = (char *)talloc(count+1);
    va_start(args, format);
    vsnprintf(cstr, count + 1, format, args);
    va_end(args);
    cstr[count] = 0;

    String result;
    result.data = (uint8_t *)cstr;
    result.count = count;
    return result;
}

////////////////////////////////////////////////////////////////////////////////

Array<3, bool> mouse_buttons_down;
Array<3, bool> mouse_buttons_held;
Array<3, bool> mouse_buttons_up;

Array<512, bool> inputs_down;
Array<512, bool> inputs_held;
Array<512, bool> inputs_up;
Array<512, bool> inputs_repeat;

HMM_Vec2 mouse_screen_position;

bool get_mouse_down(sapp_mousebutton mouse, bool consume) { bool result = mouse_buttons_down[mouse]; if (consume) { mouse_buttons_down[mouse] = false; } return result; }
bool get_mouse_held(sapp_mousebutton mouse, bool consume) { bool result = mouse_buttons_held[mouse]; if (consume) { mouse_buttons_down[mouse] = false; } return result; }
bool get_mouse_up  (sapp_mousebutton mouse, bool consume) { bool result = mouse_buttons_up[mouse];   if (consume) { mouse_buttons_up[mouse]   = false; } return result; }

bool get_input_down  (sapp_keycode input, bool consume) { bool result = inputs_down[input];   if (consume) { inputs_down[input]   = false; } return result; }
bool get_input_held  (sapp_keycode input, bool consume) { bool result = inputs_held[input];   if (consume) { inputs_down[input]   = false; } return result; }
bool get_input_up    (sapp_keycode input, bool consume) { bool result = inputs_up[input];     if (consume) { inputs_up[input]     = false; } return result; }
bool get_input_repeat(sapp_keycode input, bool consume) { bool result = inputs_repeat[input]; if (consume) { inputs_repeat[input] = false; } return result; }

////////////////////////////////////////////////////////////////////////////////

float Rect::width() {
    return max.X - min.X;
}

float Rect::height() {
    return max.Y - min.Y;
}

Rect Rect::subrect(float xmin, float ymin, float xmax, float ymax, float top, float right, float bottom, float left) {
    float w = width();
    float h = height();

    Rect result = {};
    result.min.X = min.X + w * xmin + left;
    result.min.Y = min.Y + h * ymin + bottom;
    result.max.X = min.X + w * xmax - right;
    result.max.Y = min.Y + h * ymax - top;
    return result;
}

Rect Rect::offset(float x, float y) {
    Rect result = *this;
    result.min.X += x;
    result.min.Y += y;
    result.max.X += x;
    result.max.Y += y;
    return result;
}

Rect Rect::inset(float top, float right, float bottom, float left) {
    Rect result = *this;
    result.min.X += left;
    result.min.Y += bottom;
    result.max.X -= right;
    result.max.Y -= top;
    return result;
}

Rect Rect::cut_top(float height) {
    Rect result = *this;
    max.Y -= height;
    result.min.Y = max.Y;
    return result;
}

Rect Rect::cut_right(float width) {
    Rect result = *this;
    max.X -= width;
    result.min.X = max.X;
    return result;
}

Rect Rect::cut_bottom(float height) {
    Rect result = *this;
    max.Y += height;
    result.max.Y = min.Y;
    return result;
}

Rect Rect::cut_left(float width) {
    Rect result = *this;
    max.X += width;
    result.max.X = min.X;
    return result;
}

Rect Rect::top_rect(float height) {
    Rect result = *this;
    result.min.Y = result.max.Y - height;
    return result;
}

Rect Rect::right_rect(float width) {
    Rect result = *this;
    result.min.X = result.max.X - width;
    return result;
}

Rect Rect::bottom_rect(float height) {
    Rect result = *this;
    result.max.Y = result.min.Y + height;
    return result;
}

Rect Rect::left_rect(float width) {
    Rect result = *this;
    result.max.X = result.min.X + width;
    return result;
}