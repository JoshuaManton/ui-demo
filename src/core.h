#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "external/HandmadeMath.h"
#include "sokol_impl.h"

#pragma warning(disable : 4505) // unreferenced function with internal linkage has been removed
#pragma warning(disable : 4996) // This function or variable may be unsafe. Consider using fopen_s instead.

////////////////////////////////////////////////////////////////////////////////

#define FOR(i, lo, hi) for (int64_t i = lo; i <= hi; i++)
#define FORR(i, lo, hi) for (int64_t i = hi; i >= lo; i--)
#define UNUSED(x) ((void)x)

////////////////////////////////////////////////////////////////////////////////

static float clamp(float a, float b, float x) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
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

static float FMAX(float a, float b) {
    if (a > b) return a;
    return b;
}

static float FMIN(float a, float b) {
    if (a < b) return a;
    return b;
}

static HMM_Vec2 v2(float x, float y) { return {x, y}; }
static HMM_Vec4 v4(float x, float y, float z, float w) { return {x, y, z, w}; }

// thanks Demetri
static uint32_t random_next(uint64_t *state) {
    uint64_t old = *state ^ 0xc90fdaa2adf85459ULL;
    *state = *state * 6364136223846793005ULL + 0xc90fdaa2adf85459ULL;
    uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
    uint32_t rot = old >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((- (int64_t) rot) & 31));
}

static uint64_t make_random(uint64_t seed) {
    random_next(&seed);
    return seed;
}

static int64_t random_range_int(uint64_t *state, int64_t lo, int64_t hi) {
    assert(hi >= lo);
    uint32_t r = random_next(state);
    int64_t range = hi - lo + 1;
    int64_t result = lo + (int64_t)(r % range);
    return result;
}

static float random_range_float(uint64_t *state, float lo, float hi) {
    assert(hi >= lo);
    uint32_t r = random_next(state);
    double t = (double)r / (double)UINT32_MAX;
    return (float)(lo + (hi - lo) * t);
}

static int64_t sign(float f) {
    if (f > 0) return 1;
    if (f < 0) return -1;
    return 0;
}

static float move_toward(float from, float to, float speed) {
    assert(speed > 0);
    float delta = to - from;
    if (fabsf(delta) <= speed) {
        return to;
    }
    return from + sign(delta) * speed;
}

static float ease_t(float value, float duration) {
    return clamp(0.0, 1.0, value / duration);
}

static float ease_ping_pong(float value, bool use_ping, float (*ping)(float), float (*pong)(float)) {
    if (use_ping) {
        return ping(value);
    }
    return pong(value);
}

static float ease_in_quart(float x) {
    return x * x * x * x;
}

static float ease_out_quart(float x) {
    return (float)(1 - pow(1 - x, 4));
}

static float smoothstep(float a, float b, float t) {
    t = clamp(0.0, 1.0, (t - a) / (b - a));
    return (float)(t * t * (3.0 - 2.0 * t));
}

static HMM_Vec4 random_color(uint64_t *rng) {
    return {random_range_float(rng, 0.6f, 1.0f), random_range_float(rng, 0.6f, 1.0f), random_range_float(rng, 0.6f, 1.0f), 1.0};
}

static float sin_turns(float t) {
    return sinf(2 * HMM_PI32 * t);
}

static float cos_turns(float t) {
    return cosf(2 * HMM_PI32 * t);
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
    UNUSED(data);
    UNUSED(align);
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

extern Arena *temp_arena;

static Allocator temp() {
    return temp_arena->allocator();
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct List {
    T *data;
    int64_t count;
    int64_t capacity;
    Allocator allocator;

    T *add_count(int64_t to_add) {
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

    T pop() {
        assert(count > 0);
        T result = data[count-1];
        count -= 1;
        return result;
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

template<typename T>
List<T> make_list(Allocator allocator, int64_t capacity = 8) {
    List<T> result = {};
    result.allocator = allocator;
    result.maybe_resize(capacity);
    return result;
}

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
extern HMM_Vec2 mouse_screen_delta;
extern HMM_Vec2 mouse_scroll;

bool get_mouse_down(sapp_mousebutton mouse, bool consume);
bool get_mouse_held(sapp_mousebutton mouse, bool consume);
bool get_mouse_up  (sapp_mousebutton mouse, bool consume);

HMM_Vec2 get_mouse_scroll(bool consume);

bool get_input_down  (sapp_keycode input, bool consume);
bool get_input_held  (sapp_keycode input, bool consume);
bool get_input_up    (sapp_keycode input, bool consume);
bool get_input_repeat(sapp_keycode input, bool consume);

////////////////////////////////////////////////////////////////////////////////

extern float ui_scale_factor/* = 1*/;

enum Fit_Aspect {
    AUTO,
    KEEP_WIDTH,
    KEEP_HEIGHT,
};

struct Rect {
    HMM_Vec2 min;
    HMM_Vec2 max;

    float width();
    float height();
    HMM_Vec2 size();

    Rect subrect(float xmin, float ymin, float xmax, float ymax, float top, float right, float bottom, float left);

    Rect offset(HMM_Vec2 v);
    Rect offset(float x, float y);

    Rect inset(float top, float right, float bottom, float left);
    Rect inset(float all);
    Rect inset_top(float amount);
    Rect inset_right(float amount);
    Rect inset_bottom(float amount);
    Rect inset_left(float amount);

    Rect grow(float top, float right, float bottom, float left);
    Rect grow(float all);
    Rect grow_top(float amount);
    Rect grow_right(float amount);
    Rect grow_bottom(float amount);
    Rect grow_left(float amount);

    Rect cut_top(float height);
    Rect cut_right(float width);
    Rect cut_bottom(float height);
    Rect cut_left(float width);

    Rect top_rect(float height = 0);
    Rect right_rect(float width = 0);
    Rect bottom_rect(float height = 0);
    Rect left_rect(float width = 0);

    Rect subrect_unscaled(float xmin, float ymin, float xmax, float ymax, float top, float right, float bottom, float left);

    Rect offset_unscaled(HMM_Vec2 v);
    Rect offset_unscaled(float x, float y);

    Rect inset_unscaled(float top, float right, float bottom, float left);
    Rect inset_unscaled(float all);
    Rect inset_top_unscaled(float amount);
    Rect inset_right_unscaled(float amount);
    Rect inset_bottom_unscaled(float amount);
    Rect inset_left_unscaled(float amount);

    Rect grow_unscaled(float top, float right, float bottom, float left);
    Rect grow_unscaled(float all);
    Rect grow_top_unscaled(float amount);
    Rect grow_right_unscaled(float amount);
    Rect grow_bottom_unscaled(float amount);
    Rect grow_left_unscaled(float amount);

    Rect cut_top_unscaled(float height);
    Rect cut_right_unscaled(float width);
    Rect cut_bottom_unscaled(float height);
    Rect cut_left_unscaled(float width);

    Rect top_rect_unscaled(float height = 0);
    Rect right_rect_unscaled(float width = 0);
    Rect bottom_rect_unscaled(float height = 0);
    Rect left_rect_unscaled(float width = 0);

    Rect fit_aspect(float aspect, Fit_Aspect kind = Fit_Aspect::AUTO);

    Rect encapsulate(Rect other);
    Rect slide(float x, float y);
    Rect lerp_to(Rect other, float t);

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

////////////////////////////////////////////////////////////////////////////////

// thanks bill
template <typename T> struct gbRemoveReference       { typedef T Type; };
template <typename T> struct gbRemoveReference<T &>  { typedef T Type; };
template <typename T> struct gbRemoveReference<T &&> { typedef T Type; };

template <typename T> inline T &&gb_forward(typename gbRemoveReference<T>::Type &t)  { return static_cast<T &&>(t); }
template <typename T> inline T &&gb_forward(typename gbRemoveReference<T>::Type &&t) { return static_cast<T &&>(t); }
template <typename T> inline T &&gb_move   (T &&t)                                   { return static_cast<typename gbRemoveReference<T>::Type &&>(t); }
template <typename F>
struct gbprivDefer {
    F f;
    gbprivDefer(F &&f) : f(gb_forward<F>(f)) {}
    ~gbprivDefer() { f(); }
};
template <typename F> gbprivDefer<F> gb__defer_func(F &&f) { return gbprivDefer<F>(gb_forward<F>(f)); }

#define GB_DEFER_1(x, y) x##y
#define GB_DEFER_2(x, y) GB_DEFER_1(x, y)
#define GB_DEFER_3(x)    GB_DEFER_2(x, __COUNTER__)
#define defer(code)      auto GB_DEFER_3(_defer_) = gb__defer_func([&]()->void{code;})
