#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "external/HandmadeMath.h"
#include "sokol_impl.h"

////////////////////////////////////////////////////////////////////////////////

#define FOR(i, lo, hi) for (int64_t i = lo; i <= hi; i++)
#define FORR(i, lo, hi) for (int64_t i = hi; i >= lo; i--)

////////////////////////////////////////////////////////////////////////////////

static float clamp(float a, float b, float x) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

static float random_range(float a, float b) {
    float result = a + ((float)rand() / (float)RAND_MAX) * (b-a);
    return result;
}

static int64_t align_forward(int64_t cursor, int64_t alignment) {
    int64_t a = alignment;
    assert(a > 0);
    int64_t result = ((cursor + a - 1) / a) * a;
    return result;
}

static int64_t IMAX(int64_t a, int64_t b) {
    if (a > b) return a;
    return b;
}

static int64_t IMIN(int64_t a, int64_t b) {
    if (a < b) return a;
    return b;
}

static int64_t FMAX(float a, float b) {
    if (a > b) return a;
    return b;
}

static int64_t FMIN(float a, float b) {
    if (a < b) return a;
    return b;
}

////////////////////////////////////////////////////////////////////////////////

enum Allocator_Mode {
    ALLOCATOR_MODE_ALLOC,
    ALLOCATOR_MODE_FREE,
};

struct Allocator {
    void *data;
    void *(*proc)(void *data, void *old_ptr, int64_t size, int64_t align, Allocator_Mode mode);
};

static void *alloc(Allocator allocator, int64_t size, int64_t align, bool zero) {
    void *result = allocator.proc(allocator.data, nullptr, size, align, ALLOCATOR_MODE_ALLOC);
    if (zero) {
        memset(result, 0, size);
    }
    return result;

}
static void free(Allocator allocator, void *ptr) {
    allocator.proc(allocator.data, ptr, 0, 0, ALLOCATOR_MODE_FREE);
}

////////////////////////////////////////////////////////////////////////////////

static void *default_allocator_proc(void *data, void *old_ptr, int64_t size, int64_t align, Allocator_Mode mode) {
    if (mode == ALLOCATOR_MODE_ALLOC) {
        void *result = malloc(size);
        return result;
    }
    else {
        assert(mode == ALLOCATOR_MODE_FREE);
        free(old_ptr);
        return nullptr;
    }
}

static Allocator default_allocator() {
    return {nullptr, default_allocator_proc};
}

////////////////////////////////////////////////////////////////////////////////

struct Arena {
    uint8_t *memory;
    int64_t capacity;
    int64_t cursor;
    int64_t start;
    Allocator backing_allocator;

    Allocator allocator();
    void reset();
    void destroy();
};

Arena *bootstrap_arena(Allocator backing_allocator, int64_t size);

////////////////////////////////////////////////////////////////////////////////

void *talloc(int64_t size);

////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct List {
    T *data;
    int64_t count;
    int64_t capacity;
    Allocator allocator;

    T *add(int64_t to_add = 1) {
        maybe_resize(to_add);
        assert((count + to_add) <= capacity);
        T *result = &data[count];
        memset(result, 0, sizeof(T) * to_add);
        count += to_add;
        return result;
    }

    T *add(T elem) {
        maybe_resize(1);
        assert((count + 1) <= capacity);
        T *result = &data[count];
        *result = elem;
        count += 1;
        return result;
    }

    bool maybe_resize(int64_t to_add) {
        if ((count+to_add) <= capacity) {
            return false;
        }
        int64_t new_capacity = IMAX(capacity * 2 + 8, count + to_add);
        T *new_data = (T *)alloc(allocator, new_capacity * sizeof(T), alignof(T), false);
        memcpy(new_data, data, count * sizeof(T));
        free(allocator, data);
        data = new_data;
        capacity = new_capacity;
        return true;
    }

    void ordered_remove_by_index(int64_t index) {
        assert(index >= 0);
        assert(index < count);
        for (int64_t i = index; i < count-1; i++) {
            data[i] = data[i+1];
        }
        count -= 1;
    }

    void unordered_remove_by_index(int64_t index) {
        assert(index >= 0);
        assert(index < count);
        data[index] = data[count-1];
        count -= 1;
    }

    void reset() {
        count = 0;
    }

    T &operator[](int64_t index) {
        assert(index >= 0);
        assert(index < count);
        return data[index];
    }
};

////////////////////////////////////////////////////////////////////////////////

template<int64_t N, typename T>
struct Array {
    T data[N];

    T &operator[](int64_t index) {
        assert(index >= 0);
        assert(index < N);
        return data[index];
    }

    constexpr int64_t count() {
        return N;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct String {
    uint8_t *data;
    int64_t count;

    bool  operator ==(String);
    uint8_t &operator [](int64_t);

    uint8_t *begin() { return data; }
    uint8_t *end()   { return data+count; }

    String()
    : data(nullptr)
    , count(0) {
    }

    String(const char *c) {
        data = (uint8_t *)c;
        this->count = strlen(c);
    }

    String(const char *c, int64_t count) {
        data = (uint8_t *)c;
        this->count = count;
    }

    String(uint8_t *c, int64_t count) {
        data = c;
        this->count = count;
    }
};

#define STRING_COUNT_DATA(str) (int)(str).count, (str).data
#define STRING_DATA_COUNT(str) (str).data, (int)(str).count

String tprint(char* format, ...);

////////////////////////////////////////////////////////////////////////////////

extern Array<3, bool> mouse_buttons_down;
extern Array<3, bool> mouse_buttons_held;
extern Array<3, bool> mouse_buttons_up;

extern Array<512, bool> inputs_down;
extern Array<512, bool> inputs_held;
extern Array<512, bool> inputs_up;
extern Array<512, bool> inputs_repeat;

extern HMM_Vec2 mouse_screen_position;

bool get_mouse_down(sapp_mousebutton mouse, bool consume);
bool get_mouse_held(sapp_mousebutton mouse, bool consume);
bool get_mouse_up  (sapp_mousebutton mouse, bool consume);

bool get_input_down  (sapp_keycode input, bool consume);
bool get_input_held  (sapp_keycode input, bool consume);
bool get_input_up    (sapp_keycode input, bool consume);
bool get_input_repeat(sapp_keycode input, bool consume);

////////////////////////////////////////////////////////////////////////////////

struct Rect {
    HMM_Vec2 min;
    HMM_Vec2 max;

    float width();
    float height();

    Rect subrect(float xmin, float ymin, float xmax, float ymax, float top, float right, float bottom, float left);

    Rect offset(HMM_Vec2 v);
    Rect offset(float x, float y);

    Rect inset(float top, float right, float bottom, float left);
    Rect inset(float all);

    Rect grow(float top, float right, float bottom, float left);
    Rect grow(float all);

    Rect cut_top(float height);
    Rect cut_right(float width);
    Rect cut_bottom(float height);
    Rect cut_left(float width);

    Rect top_rect(float height = 0);
    Rect right_rect(float width = 0);
    Rect bottom_rect(float height = 0);
    Rect left_rect(float width = 0);

    Rect center_rect();
    Rect top_center_rect();
    Rect right_center_rect();
    Rect bottom_center_rect();
    Rect left_center_rect();

    Rect top_left_rect();
    Rect top_right_rect();
    Rect bottom_left_rect();
    Rect bottom_right_rect();
};