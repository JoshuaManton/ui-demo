#pragma once

#include "core.h"
#include "draw.h"

////////////////////////////////////////////////////////////////////////////////

typedef uint64_t Widget_Flags; enum {
    WIDGET_FLAG_NOT_CLICKABLE = 1 << 0,
    WIDGET_FLAG_DRAGGABLE     = 1 << 1,
    WIDGET_FLAG_DROPPABLE     = 1 << 2,
    WIDGET_FLAG_BLOCKER       = 1 << 3,
};

typedef uint64_t Scroll_View_Flags; enum {
    SCROLL_VIEW_HORIZONTAL = 1 << 0,
    SCROLL_VIEW_VERTICAL   = 1 << 1,
};

struct Widget {
    uint64_t id;
    Widget_Flags flags;
    Rect rect;
    Rect hit_rect;
    int64_t serial;
    int64_t render_layer;
    bool used_marker;
    bool is_new;

    bool active;
    bool hot;
    bool clicked;
    bool released_on_top;
    bool dropped;

    float active_t;
    float hot_t;
    float clicked_t;
    float dropped_t;

    Scroll_View_Flags scroll_view_flags;
    HMM_Vec2 scroll_view_target_offset;
    HMM_Vec2 scroll_view_current_offset;
    Rect scroll_view_content_rect;
};

////////////////////////////////////////////////////////////////////////////////

void ui_init();
void ui_new_frame(float dt);
void ui_end_frame();

static Rect full_screen_rect() {
    extern Rect full_screen_rect_value;
    return full_screen_rect_value;
}

void ui_push_id(String id);
void ui_push_id(int64_t index);
void ui_push_id(void *ptr);
void ui_pop_id();
#define UI_PUSH_ID(id) ui_push_id(id); defer (ui_pop_id());

int64_t ui_get_next_serial();

////////////////////////////////////////////////////////////////////////////////

enum class Text_VAlign {
    TOP,
    CENTER,
    BOTTOM,
    BASELINE,
};

enum class Text_HAlign {
    LEFT,
    CENTER,
    RIGHT,
};

struct Text_Settings {
    Font *font;
    Text_VAlign valign;
    Text_HAlign halign;
    HMM_Vec4 color;
};

Rect ui_text(Rect rect, String string, Text_Settings settings);

////////////////////////////////////////////////////////////////////////////////

Widget *ui_blocker(Rect rect, String id);

////////////////////////////////////////////////////////////////////////////////

struct Button_Settings {
    HMM_Vec4 color       = {0.8f, 0.8f, 0.8f, 1.0f};
    HMM_Vec4 hover_color = {0.65f, 0.65f, 0.65f, 1.0f};
    HMM_Vec4 press_color = {0.5f, 0.5f, 0.5f, 1.0f};
    HMM_Vec4 click_color = {1, 1, 1, 1};
    HMM_Vec4 color_multiplier = {1, 1, 1, 1};
};

Widget *ui_button(Rect rect, String id, Button_Settings settings, String text = {}, Text_Settings text_settings = {});

////////////////////////////////////////////////////////////////////////////////

Widget *drag_drop_source(Rect rect, String id, uint64_t payload_id, void *payload, Rect *out_mouse_rect);

////////////////////////////////////////////////////////////////////////////////

Widget *drag_drop_target(Rect rect, String id, uint64_t payload_id, void **payload);

////////////////////////////////////////////////////////////////////////////////

Widget *push_scroll_view(Rect rect, String id, Scroll_View_Flags flags, Rect *out_content_rect);
void pop_scroll_view();
void expand_current_scroll_view(Rect rect);

////////////////////////////////////////////////////////////////////////////////

enum Grid_Layout_Kind {
    ELEMENT_COUNT,
    ELEMENT_SIZE,
};

struct Grid_Layout {
    float element_width;
    float element_height;

    int64_t cur_x = -1;
    int64_t cur_y = -1;
    int64_t elements_per_row;
    int64_t elements_per_column;

    Rect root_entry_rect;

    Rect next() {
        cur_x = (cur_x + 1) % elements_per_row;
        if (cur_x == 0) {
            cur_y += 1;
        }
        Rect result = root_entry_rect.offset_unscaled(cur_x * element_width, -cur_y * element_height);
        expand_current_scroll_view(result);
        return result;
    }

    Rect get_rect_for_index(int64_t index) {
        int64_t x = index % elements_per_row;
        int64_t y = index / elements_per_row;
        Rect result = root_entry_rect.offset_unscaled(x * element_width, -y * element_height);
        expand_current_scroll_view(result);
        return result;
    }
};

Grid_Layout make_grid_layout(Rect rect, float w, float h, Grid_Layout_Kind kind);

////////////////////////////////////////////////////////////////////////////////

// todo(josh): text input