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

Arena *temp_arena;

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

    char *cstr = (char *)alloc(temp(), count+1, 1, false);
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
HMM_Vec2 mouse_screen_delta;
HMM_Vec2 mouse_scroll;

bool get_mouse_down(sapp_mousebutton mouse, bool consume) { bool result = mouse_buttons_down[mouse]; if (consume) { mouse_buttons_down[mouse] = false; } return result; }
bool get_mouse_held(sapp_mousebutton mouse, bool consume) { bool result = mouse_buttons_held[mouse]; if (consume) { mouse_buttons_down[mouse] = false; } return result; }
bool get_mouse_up  (sapp_mousebutton mouse, bool consume) { bool result = mouse_buttons_up[mouse];   if (consume) { mouse_buttons_up[mouse]   = false; } return result; }

HMM_Vec2 get_mouse_scroll(bool consume) { HMM_Vec2 result = mouse_scroll; if (consume) { mouse_scroll = {}; } return result; }

bool get_input_down  (sapp_keycode input, bool consume) { bool result = inputs_down[input];   if (consume) { inputs_down[input]   = false; } return result; }
bool get_input_held  (sapp_keycode input, bool consume) { bool result = inputs_held[input];   if (consume) { inputs_down[input]   = false; } return result; }
bool get_input_up    (sapp_keycode input, bool consume) { bool result = inputs_up[input];     if (consume) { inputs_up[input]     = false; } return result; }
bool get_input_repeat(sapp_keycode input, bool consume) { bool result = inputs_repeat[input]; if (consume) { inputs_repeat[input] = false; } return result; }

////////////////////////////////////////////////////////////////////////////////

extern float ui_scale_factor = 1;

float Rect::width() {
    return max.X - min.X;
}

float Rect::height() {
    return max.Y - min.Y;
}

HMM_Vec2 Rect::size() {
    return {max.X - min.X, max.Y - min.Y};
}

Rect Rect::subrect(float xmin, float ymin, float xmax, float ymax, float top, float right, float bottom, float left) {
    float w = width();
    float h = height();

    Rect result = {};
    result.min.X = min.X + w * xmin + left   * ui_scale_factor;
    result.min.Y = min.Y + h * ymin + bottom * ui_scale_factor;
    result.max.X = min.X + w * xmax - right  * ui_scale_factor;
    result.max.Y = min.Y + h * ymax - top    * ui_scale_factor;
    return result;
}

Rect Rect::offset(float x, float y) {
    Rect result = *this;
    result.min.X += x * ui_scale_factor;
    result.min.Y += y * ui_scale_factor;
    result.max.X += x * ui_scale_factor;
    result.max.Y += y * ui_scale_factor;
    return result;
}

Rect Rect::inset(float top, float right, float bottom, float left) {
    Rect result = *this;
    result.min.X += left   * ui_scale_factor;
    result.min.Y += bottom * ui_scale_factor;
    result.max.X -= right  * ui_scale_factor;
    result.max.Y -= top    * ui_scale_factor;
    return result;
}

Rect Rect::inset(float all) {
    Rect result = *this;
    result.min.X += all * ui_scale_factor;
    result.min.Y += all * ui_scale_factor;
    result.max.X -= all * ui_scale_factor;
    result.max.Y -= all * ui_scale_factor;
    return result;
}

Rect Rect::inset_top(float amount) {
    Rect result = *this;
    result.max.Y -= amount * ui_scale_factor;
    return result;
}

Rect Rect::inset_right(float amount) {
    Rect result = *this;
    result.max.X -= amount * ui_scale_factor;
    return result;
}

Rect Rect::inset_bottom(float amount) {
    Rect result = *this;
    result.min.Y += amount * ui_scale_factor;
    return result;
}

Rect Rect::inset_left(float amount) {
    Rect result = *this;
    result.min.X += amount * ui_scale_factor;
    return result;
}

Rect Rect::grow(float top, float right, float bottom, float left) {
    Rect result = *this;
    result.min.X -= left   * ui_scale_factor;
    result.min.Y -= bottom * ui_scale_factor;
    result.max.X += right  * ui_scale_factor;
    result.max.Y += top    * ui_scale_factor;
    return result;
}

Rect Rect::grow(float all) {
    Rect result = *this;
    result.min.X -= all * ui_scale_factor;
    result.min.Y -= all * ui_scale_factor;
    result.max.X += all * ui_scale_factor;
    result.max.Y += all * ui_scale_factor;
    return result;
}

Rect Rect::grow_top(float amount) {
    Rect result = *this;
    result.max.Y += amount * ui_scale_factor;
    return result;
}

Rect Rect::grow_right(float amount) {
    Rect result = *this;
    result.max.X += amount * ui_scale_factor;
    return result;
}

Rect Rect::grow_bottom(float amount) {
    Rect result = *this;
    result.min.Y -= amount * ui_scale_factor;
    return result;
}

Rect Rect::grow_left(float amount) {
    Rect result = *this;
    result.min.X -= amount * ui_scale_factor;
    return result;
}

Rect Rect::cut_top(float pixels) {
    Rect result = *this;
    result.min.Y = FMIN(result.max.Y, result.max.Y - pixels * ui_scale_factor);
    this->max.Y -= pixels * ui_scale_factor;
    this->min.Y = FMIN(this->max.Y, this->min.Y);
    return result;
}

Rect Rect::cut_right(float pixels) {
    Rect result = *this;
    result.max.X = FMAX(result.min.X, result.min.X - pixels * ui_scale_factor);
    this->min.X -= pixels * ui_scale_factor;
    this->max.X = FMAX(this->min.X, this->max.X);
    return result;
}

Rect Rect::cut_bottom(float pixels) {
    Rect result = *this;
    result.max.Y = FMAX(result.min.Y, result.min.Y - pixels * ui_scale_factor);
    this->min.Y -= pixels * ui_scale_factor;
    this->max.Y = FMAX(this->min.Y, this->max.Y);
    return result;
}

Rect Rect::cut_left(float pixels) {
    Rect result = *this;
    result.min.X = FMIN(result.max.X, result.max.X - pixels * ui_scale_factor);
    this->max.X -= pixels * ui_scale_factor;
    this->min.X = FMIN(this->max.X, this->min.X);
    return result;
}

Rect Rect::top_rect(float height) {
    Rect result = *this;
    result.min.Y = result.max.Y - height * ui_scale_factor;
    return result;
}

Rect Rect::right_rect(float width) {
    Rect result = *this;
    result.min.X = result.max.X - width * ui_scale_factor;
    return result;
}

Rect Rect::bottom_rect(float height) {
    Rect result = *this;
    result.max.Y = result.min.Y + height * ui_scale_factor;
    return result;
}

Rect Rect::left_rect(float width) {
    Rect result = *this;
    result.max.X = result.min.X + width * ui_scale_factor;
    return result;
}

Rect Rect::subrect_unscaled(float xmin, float ymin, float xmax, float ymax, float top, float right, float bottom, float left) {
    float w = width();
    float h = height();

    Rect result = {};
    result.min.X = min.X + w * xmin + left;
    result.min.Y = min.Y + h * ymin + bottom;
    result.max.X = min.X + w * xmax - right;
    result.max.Y = min.Y + h * ymax - top;
    return result;
}

Rect Rect::offset_unscaled(float x, float y) {
    Rect result = *this;
    result.min.X += x;
    result.min.Y += y;
    result.max.X += x;
    result.max.Y += y;
    return result;
}

Rect Rect::inset_unscaled(float top, float right, float bottom, float left) {
    Rect result = *this;
    result.min.X += left;
    result.min.Y += bottom;
    result.max.X -= right;
    result.max.Y -= top;
    return result;
}

Rect Rect::inset_unscaled(float all) {
    Rect result = *this;
    result.min.X += all;
    result.min.Y += all;
    result.max.X -= all;
    result.max.Y -= all;
    return result;
}

Rect Rect::inset_top_unscaled(float amount) {
    Rect result = *this;
    result.max.Y -= amount;
    return result;
}

Rect Rect::inset_right_unscaled(float amount) {
    Rect result = *this;
    result.max.X -= amount;
    return result;
}

Rect Rect::inset_bottom_unscaled(float amount) {
    Rect result = *this;
    result.min.Y += amount;
    return result;
}

Rect Rect::inset_left_unscaled(float amount) {
    Rect result = *this;
    result.min.X += amount;
    return result;
}

Rect Rect::grow_unscaled(float top, float right, float bottom, float left) {
    Rect result = *this;
    result.min.X -= left;
    result.min.Y -= bottom;
    result.max.X += right;
    result.max.Y += top;
    return result;
}

Rect Rect::grow_unscaled(float all) {
    Rect result = *this;
    result.min.X -= all;
    result.min.Y -= all;
    result.max.X += all;
    result.max.Y += all;
    return result;
}

Rect Rect::grow_top_unscaled(float amount) {
    Rect result = *this;
    result.max.Y += amount;
    return result;
}

Rect Rect::grow_right_unscaled(float amount) {
    Rect result = *this;
    result.max.X += amount;
    return result;
}

Rect Rect::grow_bottom_unscaled(float amount) {
    Rect result = *this;
    result.min.Y -= amount;
    return result;
}

Rect Rect::grow_left_unscaled(float amount) {
    Rect result = *this;
    result.min.X -= amount;
    return result;
}

Rect Rect::cut_top_unscaled(float pixels) {
    Rect result = *this;
    result.min.Y = FMIN(result.max.Y, result.max.Y - pixels);
    this->max.Y -= pixels;
    this->min.Y = FMIN(this->max.Y, this->min.Y);
    return result;
}

Rect Rect::cut_right_unscaled(float pixels) {
    Rect result = *this;
    result.max.X = FMAX(result.min.X, result.min.X - pixels);
    this->min.X -= pixels;
    this->max.X = FMAX(this->min.X, this->max.X);
    return result;
}

Rect Rect::cut_bottom_unscaled(float pixels) {
    Rect result = *this;
    result.max.Y = FMAX(result.min.Y, result.min.Y - pixels);
    this->min.Y -= pixels;
    this->max.Y = FMAX(this->min.Y, this->max.Y);
    return result;
}

Rect Rect::cut_left_unscaled(float pixels) {
    Rect result = *this;
    result.min.X = FMIN(result.max.X, result.max.X - pixels);
    this->max.X -= pixels;
    this->min.X = FMIN(this->max.X, this->min.X);
    return result;
}

Rect Rect::top_rect_unscaled(float height) {
    Rect result = *this;
    result.min.Y = result.max.Y - height;
    return result;
}

Rect Rect::right_rect_unscaled(float width) {
    Rect result = *this;
    result.min.X = result.max.X - width;
    return result;
}

Rect Rect::bottom_rect_unscaled(float height) {
    Rect result = *this;
    result.max.Y = result.min.Y + height;
    return result;
}

Rect Rect::left_rect_unscaled(float width) {
    Rect result = *this;
    result.max.X = result.min.X + width;
    return result;
}

Rect Rect::fit_aspect(float aspect, Fit_Aspect kind/* = Fit_Aspect::AUTO*/) {
    float width  = this->width();
    float height = this->height();
    float this_aspect = width / height;

    Rect result = {};
    if (kind == Fit_Aspect::KEEP_HEIGHT || (kind == Fit_Aspect::AUTO && this_aspect >= aspect)) {
        result = subrect_unscaled(0.5f, 0.5f, 0.5f, 0.5f, -height/2, -height/2 * aspect, -height/2, -height/2 * aspect);
    }
    else if (kind == Fit_Aspect::KEEP_WIDTH || (kind == Fit_Aspect::AUTO && this_aspect < aspect)) {
        result = subrect_unscaled(0.5f, 0.5f, 0.5f, 0.5f, -width/2 / aspect, -width/2, -width/2 / aspect, -width/2);
    }
    else {
        assert(false);
    }
    return result;
}

Rect Rect::encapsulate(Rect other) {
    Rect result = *this;
    result.min.X = FMIN(result.min.X, other.min.X);
    result.min.Y = FMIN(result.min.Y, other.min.Y);
    result.max.X = FMAX(result.max.X, other.max.X);
    result.max.Y = FMAX(result.max.Y, other.max.Y);
    return result;
}

Rect Rect::slide(float x, float y) {
    Rect result = *this;
    HMM_Vec2 diff = size() * v2(x, y);
    result.min += diff;
    result.max += diff;
    return result;
}

Rect Rect::center_rect()        { return subrect(0.5, 0.5, 0.5, 0.5, 0, 0, 0, 0); }
Rect Rect::top_center_rect()    { return subrect(0.5, 1, 0.5, 1, 0, 0, 0, 0); }
Rect Rect::right_center_rect()  { return subrect(1, 0.5, 1, 0.5, 0, 0, 0, 0); }
Rect Rect::bottom_center_rect() { return subrect(0.5, 0, 0.5, 0, 0, 0, 0, 0); }
Rect Rect::left_center_rect()   { return subrect(0, 0.5, 0, 0.5, 0, 0, 0, 0); }

Rect Rect::top_left_rect()      { return subrect(0, 1, 0, 1, 0, 0, 0, 0); }
Rect Rect::top_right_rect()     { return subrect(1, 1, 1, 1, 0, 0, 0, 0); }
Rect Rect::bottom_left_rect()   { return subrect(0, 0, 0, 0, 0, 0, 0, 0); }
Rect Rect::bottom_right_rect()  { return subrect(1, 0, 1, 0, 0, 0, 0, 0); }