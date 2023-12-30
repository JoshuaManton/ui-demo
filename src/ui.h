#pragma once

#include "core.h"

////////////////////////////////////////////////////////////////////////////////

typedef uint64_t Widget_Flags; enum {
    WIDGET_FLAG_NOT_CLICKABLE = 1 << 0,
};

struct Widget {
    uint64_t id;
    Widget_Flags flags;
    Rect rect;
    int64_t serial;
    int64_t render_layer;
    bool used_marker;

    bool active;
    bool hot;
    bool clicked;

    float active_t;
    float hot_t;
    float clicked_t;
};

////////////////////////////////////////////////////////////////////////////////

void ui_new_frame(float dt);

void ui_push_id(String id);
void ui_pop_id();

int64_t ui_get_next_serial();

////////////////////////////////////////////////////////////////////////////////

struct Button_Settings {
    HMM_Vec4 color       = {0.8, 0.8, 0.8, 1};
    HMM_Vec4 hover_color = {0.65, 0.65, 0.65, 1.0};
    HMM_Vec4 press_color = {0.5, 0.5, 0.5, 1.0};
    HMM_Vec4 click_color = {1, 1, 1, 1};
};

Widget *ui_button(Rect rect, String id, Button_Settings settings);

////////////////////////////////////////////////////////////////////////////////

// todo(josh): grid layout

////////////////////////////////////////////////////////////////////////////////

// todo(josh): drag drop source

////////////////////////////////////////////////////////////////////////////////

// todo(josh): drag drop target

////////////////////////////////////////////////////////////////////////////////

// todo(josh): scroll view

////////////////////////////////////////////////////////////////////////////////

// todo(josh): text input