#include "ui.h"
#include "draw.h"

static List<Widget> all_widgets;
static List<int64_t> pushed_ids;
static List<Widget *> pushed_scroll_views;
static Widget *current_scroll_view;

static uint64_t current_id;

static uint64_t ui_active_widget;
static uint64_t ui_hot_widget;
static uint64_t ui_hot_draggable_widget;

static HMM_Vec2 ui_mouse_position_on_set_active;

static float ui_dt_for_last_frame;

static int64_t ui_last_serial;

static bool ui_used_widget_marker_for_this_frame;

Rect full_screen_rect_value;

static uint64_t current_drag_drop_payload_id;
static void    *current_drag_drop_payload;

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

void ui_init() {
    all_widgets.allocator = default_allocator();
    pushed_ids.allocator = default_allocator();
    pushed_scroll_views.allocator = default_allocator();
}

void ui_new_frame(float dt) {
    assert(pushed_ids.count == 0 && "somebody forgot to pop a UI id");
    current_id = fnv8(nullptr, 0);

    ui_dt_for_last_frame = dt;
    ui_last_serial = 0;

    FOR (i, 0, all_widgets.count-1) {
        Widget *widget = &all_widgets[i];
        if (widget->used_marker != ui_used_widget_marker_for_this_frame) {
            all_widgets[i] = all_widgets[all_widgets.count-1];
            all_widgets.count -= 1;
            i -= 1;
            continue;
        }
    }

    qsort(all_widgets.data, all_widgets.count, sizeof(Widget), compare_widgets);

    ui_hot_widget = 0;
    ui_hot_draggable_widget = 0;
    FOR (i, 0, all_widgets.count-1) {
        Widget *widget = &all_widgets[i];
        if (widget->flags & WIDGET_FLAG_NOT_CLICKABLE) {
            continue;
        }
        if (widget->rect.min.X <= mouse_screen_position.X &&
            widget->rect.min.Y <= mouse_screen_position.Y &&
            widget->rect.max.X >= mouse_screen_position.X &&
            widget->rect.max.Y >= mouse_screen_position.Y) {
            ui_hot_widget = widget->id;
            if (widget->flags & WIDGET_FLAG_DRAGGABLE) {
                ui_hot_draggable_widget = widget->id;
            }
        }
    }

    ui_used_widget_marker_for_this_frame = !ui_used_widget_marker_for_this_frame;
    full_screen_rect_value = {{0, 0}, {sapp_widthf(), sapp_heightf()}};
    ui_scale_factor = sapp_heightf() / 1080.0f;
}

void ui_end_frame() {
    if (current_drag_drop_payload_id != 0 && !get_mouse_held(SAPP_MOUSEBUTTON_LEFT, false)) {
        current_drag_drop_payload_id = 0;
        current_drag_drop_payload = nullptr;
    }
}

void ui_push_id(String id) {
    pushed_ids.add(current_id);
    current_id = fnv8_combine(current_id, id.data, id.count);
}

void ui_push_id(int64_t index) {
    pushed_ids.add(current_id);
    current_id = fnv8_combine(current_id, (uint8_t *)&index, sizeof(index));
}

void ui_push_id(void *ptr) {
    pushed_ids.add(current_id);
    current_id = fnv8_combine(current_id, (uint8_t *)&ptr, sizeof(ptr));
}

void ui_pop_id() {
    assert(pushed_ids.count > 0);
    current_id = pushed_ids.pop();
}

int64_t ui_get_next_serial() {
    ui_last_serial += 1;
    return ui_last_serial;
}

////////////////////////////////////////////////////////////////////////////////

static Widget *try_get_existing_widget(uint64_t id) {
    // note(josh): in a real implementation you would probably want a hashtable here but this is fine for demo purposes
    FOR (i, 0, all_widgets.count-1) {
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

static void force_set_active_widget(uint64_t id) {
    ui_active_widget = id;
    ui_mouse_position_on_set_active = mouse_screen_position;
}

Widget *update_widget(Rect rect, String id, Widget_Flags flags = 0) {
    uint64_t real_id = calculate_id(id);
    Widget *widget = try_get_existing_widget(real_id);
    if (widget == nullptr) {
        widget = all_widgets.add_count(1);
        *widget = {};
        widget->is_new = true;
    }
    else {
        widget->is_new = false;
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
            if (get_mouse_down(SAPP_MOUSEBUTTON_LEFT, true)) {
                force_set_active_widget(widget->id);
            }
        }
        if (ui_active_widget == widget->id && !(widget->flags & WIDGET_FLAG_DRAGGABLE)) {
            if (HMM_LenV2(ui_mouse_position_on_set_active - mouse_screen_position) > 5 && ui_hot_draggable_widget != 0) {
                force_set_active_widget(ui_hot_draggable_widget);
            }
        }
        if (ui_active_widget == widget->id) {
            if (!get_mouse_held(SAPP_MOUSEBUTTON_LEFT, false)) {
                if (ui_hot_widget == widget->id) {
                    widget->clicked = true;
                }
                ui_active_widget = 0;
            }
        }
    }

    widget->active = widget->id == ui_active_widget;
    widget->hot    = widget->id == ui_hot_widget;

    if (widget->active)  { widget->active_t  = clamp(0, 1, widget->active_t  + ui_dt_for_last_frame * 10); }
    else                 { widget->active_t  = clamp(0, 1, widget->active_t  - ui_dt_for_last_frame * 10); }
    if (widget->hot)     { widget->hot_t     = clamp(0, 1, widget->hot_t     + ui_dt_for_last_frame * 10); }
    else                 { widget->hot_t     = clamp(0, 1, widget->hot_t     - ui_dt_for_last_frame * 10); }
    if (widget->clicked) { widget->clicked_t = 1;                                                         }
    else                 { widget->clicked_t = clamp(0, 1, widget->clicked_t - ui_dt_for_last_frame * 2); }
    if (widget->dropped) { widget->dropped_t = 1;                                                         }
    else                 { widget->dropped_t = clamp(0, 1, widget->dropped_t - ui_dt_for_last_frame * 2); }

    widget->released_on_top = false;
    if (widget->hot && get_mouse_up(SAPP_MOUSEBUTTON_LEFT, true)) {
        widget->released_on_top = true;
    }
    return widget;
}

Widget *ui_button(Rect rect, String id, Button_Settings settings) {
    expand_current_scroll_view(rect);
    Widget *button = update_widget(rect, id);
    HMM_Vec4 color = settings.color;
    color = HMM_LerpV4(color, button->hot_t,     settings.hover_color);
    color = HMM_LerpV4(color, button->active_t,  settings.press_color);
    color = HMM_LerpV4(color, button->clicked_t, settings.click_color);
    color *= settings.color_multiplier;
    draw_quad(rect, color);
    return button;
}

////////////////////////////////////////////////////////////////////////////////

Grid_Layout make_grid_layout(Rect rect, int64_t w, int64_t h, Grid_Layout_Kind kind) {
    Grid_Layout grid = {};
    grid.cur_x = -1;
    grid.cur_y = -1;
    if (kind == Grid_Layout_Kind::ELEMENT_SIZE) {
        grid.element_width       = w;
        grid.element_height      = h;
        grid.elements_per_row    = IMAX(1, (int64_t)(rect.width()  / w));
        grid.elements_per_column = IMAX(1, (int64_t)(rect.height() / h));
    }
    else {
        assert(kind == Grid_Layout_Kind::ELEMENT_COUNT);
        grid.element_width       = rect.width()  / (float)w;
        grid.element_height      = rect.height() / (float)h;
        grid.elements_per_row    = w;
        grid.elements_per_column = h;
    }
    grid.root_entry_rect = rect.top_left_rect().grow_unscaled(0, grid.element_width, grid.element_height, 0);
    return grid;
}

////////////////////////////////////////////////////////////////////////////////

Widget *drag_drop_source(Rect rect, String id, uint64_t payload_id, void *payload, Rect *out_mouse_rect) {
    expand_current_scroll_view(rect);
    Widget *widget = update_widget(rect, id, WIDGET_FLAG_DRAGGABLE);
    if (widget->active) {
        current_drag_drop_payload_id = payload_id;
        current_drag_drop_payload    = payload;
    }
    *out_mouse_rect = {mouse_screen_position, mouse_screen_position};
    return widget;
}

////////////////////////////////////////////////////////////////////////////////

Widget *drag_drop_target(Rect rect, String id, uint64_t payload_id, void **payload) {
    expand_current_scroll_view(rect);
    Widget *widget = update_widget(rect, id, WIDGET_FLAG_DROPPABLE);
    widget->dropped = false;
    if (widget->released_on_top && current_drag_drop_payload_id == payload_id) {
        widget->dropped = true;
        *payload = current_drag_drop_payload;
    }
    return widget;
}

////////////////////////////////////////////////////////////////////////////////

Widget *push_scroll_view(Rect rect, String id, Scroll_View_Flags flags, Rect *out_content_rect) {
    Widget *widget = update_widget(rect, id, WIDGET_FLAG_DRAGGABLE);
    widget->scroll_view_flags = flags;
    widget->scroll_view_current_offset = HMM_LerpV2(widget->scroll_view_current_offset, 20 * ui_dt_for_last_frame, widget->scroll_view_target_offset);
    if (HMM_LenV2(widget->scroll_view_current_offset - widget->scroll_view_target_offset) < 1) {
        widget->scroll_view_current_offset = widget->scroll_view_target_offset;
    }
    widget->scroll_view_content_rect = rect.offset(widget->scroll_view_current_offset.X, widget->scroll_view_current_offset.Y);
    *out_content_rect = widget->scroll_view_content_rect;
    pushed_scroll_views.add(current_scroll_view);
    current_scroll_view = widget;
    draw_push_scissor(rect);
    return widget;
}

void pop_scroll_view() {
    assert(current_scroll_view != nullptr);

    HMM_Vec2 new_target_offset = current_scroll_view->scroll_view_target_offset;
    if (current_scroll_view->active) {
        HMM_Vec2 mouse_delta = mouse_screen_delta;
        if (!(current_scroll_view->scroll_view_flags & SCROLL_VIEW_HORIZONTAL)) mouse_delta.X = 0;
        if (!(current_scroll_view->scroll_view_flags & SCROLL_VIEW_VERTICAL))   mouse_delta.Y = 0;
        new_target_offset += mouse_delta;
    }
    if (ui_hot_draggable_widget == current_scroll_view->id) {
        HMM_Vec2 scroll = get_mouse_scroll(true);
        if (!(current_scroll_view->scroll_view_flags & SCROLL_VIEW_HORIZONTAL)) scroll.X = 0;
        if (!(current_scroll_view->scroll_view_flags & SCROLL_VIEW_VERTICAL))   scroll.Y = 0;
        new_target_offset -= scroll * 25;
    }

    Rect target_content_rect = current_scroll_view->scroll_view_content_rect.offset(-current_scroll_view->scroll_view_current_offset.X, -current_scroll_view->scroll_view_current_offset.Y);
    target_content_rect = target_content_rect.offset(new_target_offset.X, new_target_offset.Y);

    float top_delta    = target_content_rect.max.Y - current_scroll_view->rect.max.Y;
    float right_delta  = target_content_rect.max.X - current_scroll_view->rect.max.X;
    float bottom_delta = target_content_rect.min.Y - current_scroll_view->rect.min.Y;
    float left_delta   = target_content_rect.min.X - current_scroll_view->rect.min.X;

    if (top_delta < 0) {
        new_target_offset.Y -= top_delta;
    }
    else if (bottom_delta > 0) {
        new_target_offset.Y -= bottom_delta;
    }

    if (right_delta < 0) {
        new_target_offset.X -= right_delta;
    }
    else if (left_delta > 0) {
        new_target_offset.X -= left_delta;
    }

    current_scroll_view->scroll_view_target_offset = new_target_offset;

    current_scroll_view = pushed_scroll_views.pop();
    draw_pop_scissor();
}

void expand_current_scroll_view(Rect rect) {
    if (current_scroll_view == nullptr) {
        return;
    }
    current_scroll_view->scroll_view_content_rect = current_scroll_view->scroll_view_content_rect.encapsulate(rect);
}