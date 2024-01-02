#pragma once

#include "core.h"
#include "stb.h"

////////////////////////////////////////////////////////////////////////////////

struct Vertex {
    HMM_Vec4 position;
    HMM_Vec4 uv;
    HMM_Vec4 color;
};

enum class Draw_Command_Kind {
    QUAD,
    TEXT,
    SCISSOR,
};

struct Font {
    sg_image image;
    int64_t size;
    int64_t bitmap_dim;
    int64_t ascender;
    int64_t descender;
    int64_t line_height;
    stbtt_bakedchar chars[96];
};

struct Draw_Command_Scissor {
    Rect rect;
};

struct Draw_Command_Text {
    Font *font;
    String string;
    HMM_Vec2 position;
};

struct Draw_Command {
    Draw_Command_Kind kind;

    HMM_Vec2 min;
    HMM_Vec2 max;
    HMM_Vec4 color;
    int64_t serial;
    int64_t layer;
    sg_image image;
    sg_pipeline pipeline;

    Draw_Command_Scissor scissor;
    Draw_Command_Text    text;
};

Font *load_font_from_file(const char *filepath, int64_t size);

////////////////////////////////////////////////////////////////////////////////

extern int64_t current_draw_layer;
extern sg_pipeline textured_pipeline;
extern sg_pipeline text_pipeline;

////////////////////////////////////////////////////////////////////////////////

void draw_init();
void draw_update();

int64_t draw_get_next_serial();
void draw_set_next_serial(int64_t s);

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

Draw_Command *draw_text(String text, HMM_Vec2 position, Font *font, HMM_Vec4 color);
float calculate_text_width(String text, Font *font);

void draw_flush();

