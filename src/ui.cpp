#include "ui.h"
#include "draw.h"

#define MAX_WIDGETS (1024)
Array<MAX_WIDGETS, Widget> all_widgets;
int64_t widget_count;

#define MAX_PUSHED_IDS (1024)
Array<MAX_PUSHED_IDS, int64_t> pushed_ids;
int64_t pushed_id_count;

uint64_t current_id;

uint64_t ui_active_widget;
uint64_t ui_hot_widget;

float ui_dt_for_last_frame;

int64_t ui_last_serial;

bool ui_used_widget_marker_for_this_frame;

static uint64_t fnv8_combine(uint64_t h, uint8_t *data, int64_t len) {
    FOR (i, 0, len-1) {
        h = (h * 0x100000001b3) ^ (uint64_t)data[i];
    }
    return h;
}

static uint64_t fnv8(uint8_t *data, int64_t len) {
    return fnv8_combine(0xcbf29ce484222325, data, len);
}

static int compare_widgets(const void *_a, const void *_b) {
    const Widget *a = (const Widget *)_a;
    const Widget *b = (const Widget *)_b;
    if (a->render_layer == b->render_layer) {
        return a->serial - b->serial;
    }
    return a->render_layer - b->render_layer;
}

void ui_new_frame(float dt) {
    assert(pushed_id_count == 0 && "somebody forgot to pop a UI id");
    current_id = fnv8(nullptr, 0);

    ui_dt_for_last_frame = dt;
    ui_last_serial = 0;

    FOR (i, 0, widget_count-1) {
        Widget *widget = &all_widgets[i];
        if (widget->used_marker != ui_used_widget_marker_for_this_frame) {
            all_widgets[i] = all_widgets[widget_count-1];
            widget_count -= 1;
            i -= 1;
            continue;
        }
    }

    qsort(all_widgets.data, widget_count, sizeof(Widget), compare_widgets);

    ui_hot_widget = 0;
    FOR (i, 0, widget_count-1) {
        Widget *widget = &all_widgets[i];
        if (widget->flags & WIDGET_FLAG_NOT_CLICKABLE) {
            continue;
        }
        if (widget->rect.min.X <= mouse_screen_position.X &&
            widget->rect.min.Y <= mouse_screen_position.Y &&
            widget->rect.max.X >= mouse_screen_position.X &&
            widget->rect.max.Y >= mouse_screen_position.Y) {
            ui_hot_widget = widget->id;
        }
    }

    ui_used_widget_marker_for_this_frame = !ui_used_widget_marker_for_this_frame;
}

void ui_push_id(String id) {
    assert((pushed_id_count+1) <= MAX_PUSHED_IDS);
    pushed_ids[pushed_id_count++] = current_id;
    current_id = fnv8_combine(current_id, id.data, id.count);
}

void ui_pop_id() {
    assert(pushed_id_count > 0);
    pushed_id_count -= 1;
    current_id = pushed_ids[pushed_id_count];
}

int64_t ui_get_next_serial() {
    ui_last_serial += 1;
    return ui_last_serial;
}

////////////////////////////////////////////////////////////////////////////////

static Widget *try_get_existing_widget(uint64_t id) {
    // note(josh): in a real implementation you would probably want a hashtable here but this is fine for demo purposes
    FOR (i, 0, widget_count-1) {
        Widget *widget = &all_widgets[i];
        if (widget->id == id) {
            return widget;
        }
    }
    return nullptr;
}

static uint64_t calculate_id(String id) {
    return fnv8_combine(current_id, id.data, id.count);
}

Widget *update_widget(Rect rect, String id, Widget_Flags flags = 0) {
    uint64_t real_id = calculate_id(id);
    Widget *widget = try_get_existing_widget(real_id);
    if (widget == nullptr) {
        assert((widget_count+1) <= MAX_WIDGETS);
        widget = &all_widgets[widget_count++];
        *widget = {};
    }
    assert(widget != nullptr);
    widget->flags = flags;
    widget->rect = rect;
    widget->id = real_id;
    widget->serial = ui_get_next_serial();
    widget->render_layer = current_draw_layer;
    widget->used_marker = ui_used_widget_marker_for_this_frame;
    widget->clicked = false;
    if (!(flags & WIDGET_FLAG_NOT_CLICKABLE)) {
        if (ui_hot_widget == widget->id) {
            if (get_mouse_held(SAPP_MOUSEBUTTON_LEFT, true)) {
                ui_active_widget = widget->id;
            }
        }
        if (ui_active_widget == widget->id) {
            if (!get_mouse_held(SAPP_MOUSEBUTTON_LEFT, true)) {
                if (ui_hot_widget == widget->id) {
                    widget->clicked = true;
                }
                ui_active_widget = 0;
            }
        }
    }

    widget->active = widget->id == ui_active_widget;
    widget->hot    = widget->id == ui_hot_widget;

    if (widget->active)  { widget->active_t  = clamp(0, 1, widget->active_t  + ui_dt_for_last_frame * 5); }
    else                 { widget->active_t  = clamp(0, 1, widget->active_t  - ui_dt_for_last_frame * 5); }
    if (widget->hot)     { widget->hot_t     = clamp(0, 1, widget->hot_t     + ui_dt_for_last_frame * 5); }
    else                 { widget->hot_t     = clamp(0, 1, widget->hot_t     - ui_dt_for_last_frame * 5); }
    if (widget->clicked) { widget->clicked_t = 1;                                                         }
    else                 { widget->clicked_t = clamp(0, 1, widget->clicked_t - ui_dt_for_last_frame * 2); }

    return widget;
}

Widget *ui_button(Rect rect, String id, Button_Settings settings) {
    Widget *button = update_widget(rect, id);
    HMM_Vec4 color = settings.color;
    color = HMM_LerpV4(color, button->hot_t,     settings.hover_color);
    color = HMM_LerpV4(color, button->active_t,  settings.press_color);
    color = HMM_LerpV4(color, button->clicked_t, settings.click_color);
    draw_quad(rect, color);
    return button;
}