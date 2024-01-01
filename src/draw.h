#pragma once

#include "core.h"

////////////////////////////////////////////////////////////////////////////////

struct Vertex {
    HMM_Vec4 position;
    HMM_Vec4 color;
};

struct Draw_Command {
    HMM_Vec2 min;
    HMM_Vec2 max;
    HMM_Vec4 color;
    int64_t serial;
    int64_t layer;

    bool scissor;
    Rect scissor_rect;
};

////////////////////////////////////////////////////////////////////////////////

extern int64_t current_draw_layer;

////////////////////////////////////////////////////////////////////////////////

void draw_init();
void draw_update();

int64_t draw_get_next_serial();

void draw_push_layer(int64_t layer);
void draw_push_layer_relative(int64_t delta);
void draw_pop_layer();

void draw_push_color_multiplier(HMM_Vec4 color);
void draw_pop_color_multiplier();
#define DRAW_PUSH_COLOR_MULTIPLIER(color) draw_push_color_multiplier(color); defer (draw_pop_color_multiplier());

void draw_push_scissor(Rect rect);
void draw_pop_scissor();

Rect draw_clip_rect_to_current_scissor(Rect rect);

Draw_Command *draw_quad(Rect rect, HMM_Vec4 color);
Draw_Command *draw_quad(HMM_Vec2 min, HMM_Vec2 max, HMM_Vec4 color);

void draw_flush();

