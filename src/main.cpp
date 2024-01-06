#include "core.h"
#include "draw.h"
#include "ui.h"

#define UI_DRAG_DROP_ITEM_LAYER (10000)

Font *roboto_font_small;
Font *roboto_font_medium;
Font *roboto_font_large;
float dt;
float time_since_startup;

Text_Settings default_text_settings;



////////////////////////////////////////////////////////////////////////////////
//
// Rects and drawing quads
//

void example_rects(Rect rect) {
    draw_quad(rect, {.5f, 1, .5f, 1});

    rect = rect.inset(50);
    draw_quad(rect, {1, .5f, .5f, 1});

    rect = rect.center_rect().grow(100, 200, 100, 200);
    draw_quad(rect, {.5f, .5f, 1, 1});

    rect = rect.offset(0, 300);
    draw_quad(rect, {.5f, 1, 1, 1});
}



////////////////////////////////////////////////////////////////////////////////
//
// Text
//

void example_text(Rect rect) {
    String str = "Henglo!";
    str.count = ((int64_t)(time_since_startup * 2) % str.count) + 1;
    ui_text(rect, str, default_text_settings);
}



////////////////////////////////////////////////////////////////////////////////
//
// Buttons
//

void example_buttons(Rect rect) {
    Rect button_rect = rect.center_rect().grow(50, 100, 50, 100);
    if (ui_button(button_rect, "button1", {})->clicked) {
        printf("Clicked 1!\n");
    }

    Rect button_rect2 = button_rect.offset(0, -110);
    if (ui_button(button_rect2, "button2", {})->clicked) {
        printf("Clicked 2!\n");
    }
}



////////////////////////////////////////////////////////////////////////////////
//
// More buttons
//

void example_more_buttons(Rect rect) {
    Rect cut = rect.subrect(0.5f, 0.7f, 0.5f, 0.7f, 0, -100, 0, -100);
    for (int64_t i = 0; i < 5; i++) {
        UI_PUSH_ID(i);
        Rect button_rect = cut.cut_top(100).inset(5);
        if (ui_button(button_rect, "", {})->clicked) {
            printf("Clicked %lld!\n", i);
        }
    }
}



////////////////////////////////////////////////////////////////////////////////
//
// Serial numbers
//

void example_serial_numbers(Rect rect) {
    String str = "Henglo!";
    str.count = ((int64_t)(time_since_startup * 2) % str.count) + 1;
    int64_t bg_serial = draw_get_next_serial();
    Rect text_rect = ui_text(rect, str, default_text_settings);
    draw_set_next_serial(bg_serial);
    draw_quad(text_rect, {0, 0.35f, 0, 1});
}



////////////////////////////////////////////////////////////////////////////////
//
// Layers
//

void example_layers(Rect rect) {
    ui_text(rect, "todo", default_text_settings);
}



////////////////////////////////////////////////////////////////////////////////
//
// Scroll views
//

void example_scroll_views(Rect rect) {
    ui_text(rect, "todo", default_text_settings);
}



////////////////////////////////////////////////////////////////////////////////
//
// Grids
//

void example_grids(Rect rect) {
    ui_text(rect, "todo", default_text_settings);
}



////////////////////////////////////////////////////////////////////////////////
//
// Drag and drop
//

void example_drag_and_drop(Rect rect) {
    ui_text(rect, "todo", default_text_settings);
}



////////////////////////////////////////////////////////////////////////////////
//
// Grid with selectable elements
//

int64_t selected_element = -1;
float selected_element_t;
bool something_is_focused;

void draw_selectable_element(Rect entry_rect, int64_t index) {
    UI_PUSH_ID(index);

    Button_Settings button_settings = {};
    uint64_t rng = make_random(index);
    button_settings.color_multiplier = random_color(&rng);

    // if clicked, this is the new focused element
    if (ui_button(entry_rect, "", button_settings)->clicked) {
        if (!something_is_focused) {
            something_is_focused = true;
            selected_element = index;
            selected_element_t = 0;
        }
    }
}

void draw_grid_with_selectable_elements(Rect scroll_view_rect, Rect content_rect) {
    // draw all unfocused elements
    Grid_Layout grid = make_grid_layout(content_rect, 4, 2.5f, Grid_Layout_Kind::ELEMENT_COUNT);
    int64_t focused_element = -1;
    for (int64_t element = 0; element < 18; element++) {
        Rect entry_rect = grid.next();
        expand_current_scroll_view(entry_rect);
        if (selected_element == element) {
            // skip the focused element. we'll draw it after
            focused_element = element;
            continue;
        }
        draw_selectable_element(entry_rect.inset(5), element);
    }

    // lerp animation value
    if (something_is_focused && focused_element != -1) {
        selected_element_t += dt * 4;
        if (selected_element_t > 1) selected_element_t = 1;
        ui_blocker(scroll_view_rect, "grid blocker");
    }
    else {
        selected_element_t -= dt * 4;
        if (selected_element_t < 0) selected_element_t = 0;
    }

    // draw focused element, if any
    float eased_selected_thing_t = ease_ping_pong(selected_element_t, something_is_focused, ease_out_quart, ease_in_quart);
    if (focused_element != -1) {
        // darken background
        draw_quad(scroll_view_rect, {0, 0, 0, 0.8f * eased_selected_thing_t});

        // lerp entry rect according to anim time
        Rect entry_rect = grid.get_rect_for_index(focused_element);
        expand_current_scroll_view(entry_rect);
        entry_rect = entry_rect.inset(5).lerp_to(scroll_view_rect.inset(25), eased_selected_thing_t);
        draw_selectable_element(entry_rect, focused_element);

        if (something_is_focused) {
            // block the element button, doesn't make sense to click when it's focused
            ui_blocker(entry_rect, "entry blocker");

            // draw the close button
            Button_Settings close_button_settings = {};
            close_button_settings.color_multiplier = {1, .5, .5, 1};
            if (ui_button(entry_rect.top_right_rect().grow(0, 0, 35, 35).offset(-4, -4), "close", close_button_settings)->clicked) {
                something_is_focused = false;
            }
        }
    }
}

void example_grid_with_selectable_elements(Rect rect) {
    Rect main_rect = rect.center_rect().grow(300, 450, 300, 450);
    draw_quad(main_rect, {0.25, 0.25, 0.25, 1});
    Rect content_rect = {};
    push_scroll_view(main_rect, "grid", SCROLL_VIEW_VERTICAL, &content_rect);
    defer (pop_scroll_view());
    draw_grid_with_selectable_elements(main_rect, content_rect);
}



////////////////////////////////////////////////////////////////////////////////
//
// Auto-scaling
//

void example_autoscaling(Rect rect) {
    ui_text(rect, "todo", default_text_settings);
}



////////////////////////////////////////////////////////////////////////////////
//
// Main loop
//

bool sidebar_open;
float sidebar_open_t;

bool middle_thing_open;
float middle_thing_open_t;

int64_t selected_example;

bool do_example_button(Rect *cut, int64_t index, String button_text, Text_Settings ts) {
    UI_PUSH_ID(index);
    Rect rect = cut->cut_top(70).inset(5);
    Button_Settings bs = {};
    if (selected_example == index) {
        bs.color_multiplier = {.5, 1, .5, 1};
    }
    if (ui_button(rect, "", bs, button_text, ts)->clicked) {
        selected_example = index;
    }
    return selected_example == index;
}

void app_update() {
    Button_Settings default_button_settings = {};
    HMM_Vec4 bg_rect_color = {0.25, 0.25, 0.25, 1};

    // if (ui_button(full_screen_rect().top_right_rect().grow(0, 0, 100, 100), "open middle thing", default_button_settings)->clicked) {
    //     middle_thing_open = !middle_thing_open;
    // }

    // examples listing
    {
        Text_Settings example_button_ts = {};
        example_button_ts.font = roboto_font_medium;
        example_button_ts.halign = Text_HAlign::CENTER;
        example_button_ts.valign = Text_VAlign::CENTER;
        example_button_ts.color = {0.1f, 0.1f, 0.1f, 1};
        Rect full_screen = full_screen_rect();
        Rect sidebar_rect = full_screen.cut_left(400);
        draw_quad(sidebar_rect, {.05f, .05f, .05f, 1.0});
        Rect cut = sidebar_rect.top_rect();
        if (do_example_button(&cut, 0,  "Rects",            example_button_ts)) { UI_PUSH_ID("example"); example_rects(full_screen);                         }
        if (do_example_button(&cut, 1,  "Text",             example_button_ts)) { UI_PUSH_ID("example"); example_text(full_screen);                          }
        if (do_example_button(&cut, 2,  "Serial Numbers",   example_button_ts)) { UI_PUSH_ID("example"); example_serial_numbers(full_screen);                }
        if (do_example_button(&cut, 3,  "Buttons",          example_button_ts)) { UI_PUSH_ID("example"); example_buttons(full_screen);                       }
        if (do_example_button(&cut, 4,  "More Buttons",     example_button_ts)) { UI_PUSH_ID("example"); example_more_buttons(full_screen);                  }
        if (do_example_button(&cut, 5,  "Layers",           example_button_ts)) { UI_PUSH_ID("example"); example_layers(full_screen);                        }
        if (do_example_button(&cut, 6,  "Scroll Views",     example_button_ts)) { UI_PUSH_ID("example"); example_scroll_views(full_screen);                  }
        if (do_example_button(&cut, 7,  "Grid Layout",      example_button_ts)) { UI_PUSH_ID("example"); example_grids(full_screen);                         }
        if (do_example_button(&cut, 8,  "Drag and Drop",    example_button_ts)) { UI_PUSH_ID("example"); example_drag_and_drop(full_screen);                 }
        if (do_example_button(&cut, 9,  "Grid + Modal",     example_button_ts)) { UI_PUSH_ID("example"); example_grid_with_selectable_elements(full_screen); }
        if (do_example_button(&cut, 10, "Auto-Scaling",     example_button_ts)) { UI_PUSH_ID("example"); example_autoscaling(full_screen);                   }
    }

    // center scroll list
    if (middle_thing_open) middle_thing_open_t = move_toward(middle_thing_open_t, 1, 4 * dt);
    else                   middle_thing_open_t = move_toward(middle_thing_open_t, 0, 4 * dt);

    if (middle_thing_open || middle_thing_open_t > 0) {
        UI_PUSH_ID("center scroll list");
        float open_t_eased = ease_ping_pong(middle_thing_open_t, middle_thing_open, ease_out_quart, ease_in_quart);
        DRAW_PUSH_COLOR_MULTIPLIER(v4(open_t_eased, open_t_eased, open_t_eased, open_t_eased));

        Rect full_rect = full_screen_rect().center_rect().grow(400, 600, 400, 600).offset(0, 50);
        full_rect = full_rect.offset(0, -75 * (1.0f-open_t_eased));
        draw_quad(full_rect, bg_rect_color);

        Grid_Layout grid = make_grid_layout(full_rect, 3, 1, Grid_Layout_Kind::ELEMENT_COUNT);
        Rect cut = full_rect;
        uint64_t rng = make_random(1235125);
        FOR (column_index, 0, 3-1) {
            UI_PUSH_ID(column_index);

            Rect column_rect = cut;
            if (column_index == 0) {
                column_rect = cut.cut_left(300);
            }
            else if (column_index == 1) {
                column_rect = cut.cut_left(600);
            }

            if (column_index == 1) {
                UI_PUSH_ID("bottom grid scroll");
                Rect content_rect = {};
                Rect scroll_view_rect = column_rect.cut_bottom(400);
                push_scroll_view(scroll_view_rect, "grid", SCROLL_VIEW_VERTICAL, &content_rect);
                defer (pop_scroll_view());

                draw_grid_with_selectable_elements(scroll_view_rect, content_rect);
            }

            Rect content_rect = {};
            push_scroll_view(column_rect, "scroll view", SCROLL_VIEW_VERTICAL, &content_rect);
            defer (pop_scroll_view());

            if (column_index == 1) {
                UI_PUSH_ID("top horizontal scroll");
                Rect horizontal_content_rect = {};
                push_scroll_view(content_rect.cut_top(150), "horizontal 1", SCROLL_VIEW_HORIZONTAL, &horizontal_content_rect);
                defer (pop_scroll_view());
                FOR (element, 0, 20) {
                    UI_PUSH_ID(element);
                    Rect entry_rect = horizontal_content_rect.cut_left(75);
                    expand_current_scroll_view(entry_rect);
                    Button_Settings button_settings = default_button_settings;
                    button_settings.color_multiplier = random_color(&rng);
                    if (ui_button(entry_rect.inset(5), "", button_settings)->clicked) {
                    }
                }
            }

            Rect cursor_rect = content_rect.top_rect();
            FOR (j, 0, 20-1) {
                UI_PUSH_ID(j);
                Rect entry_rect = cursor_rect.cut_top(75);
                expand_current_scroll_view(entry_rect);
                Button_Settings button_settings = default_button_settings;
                button_settings.color_multiplier = random_color(&rng);
                if (ui_button(entry_rect.inset(5), "", button_settings)->clicked) {
                }
            }
        }
    }

    // ability bar
    if (0) {
        UI_PUSH_ID("ability bar");

        static Array<8, HMM_Vec4> ability_bar_items;
        static bool ability_bar_initted = false;
        if (!ability_bar_initted) {
            ability_bar_initted = true;
            ability_bar_items[0] = {0.5, 1, 0.5, 1};
            ability_bar_items[1] = {1, 0.5, 0.5, 1};
            ability_bar_items[2] = {0.25, 1, 1, 1};
        }

        Rect bar_rect = full_screen_rect().bottom_center_rect().grow(100, 400, 0, 400);
        draw_quad(bar_rect, bg_rect_color);

        Grid_Layout grid = make_grid_layout(bar_rect, 8, 1, Grid_Layout_Kind::ELEMENT_COUNT);
        FOR (i, 0, ability_bar_items.count()-1) {
            UI_PUSH_ID(i);

            Rect entry_rect = grid.next().inset(5);
            int64_t entry_bg_serial = draw_get_next_serial();

            Widget *ddsource = nullptr;
            if (HMM_LenV4(ability_bar_items[i]) > 0) {
                Rect mouse_rect = {};
                ddsource = drag_drop_source(entry_rect, "src", 1, (void *)i, &mouse_rect);
                if (ddsource->active) {
                    draw_push_layer(UI_DRAG_DROP_ITEM_LAYER);
                    defer (draw_pop_layer());

                    Rect item_rect = mouse_rect.grow(25, 25, 25, 25);
                    draw_quad(item_rect, ability_bar_items[i]);
                }
            }

            void *dropped_payload = nullptr;
            Widget *ddtarget = drag_drop_target(entry_rect, "dst", 1, &dropped_payload);
            if (ddtarget->dropped) {
                int64_t payload = (int64_t)dropped_payload;
                HMM_Vec4 tmp = ability_bar_items[i];
                ability_bar_items[i] = ability_bar_items[payload];
                ability_bar_items[payload] = tmp;
            }

            HMM_Vec4 entry_color = {.5, .5, .5, 1};
            entry_color = HMM_LerpV4(entry_color, ddtarget->hot_t, v4(.75, .75, .75, 1));
            entry_color = HMM_LerpV4(entry_color, ddtarget->dropped_t, v4(1, 1, 1, 1));
            if (ddsource) {
                entry_color = HMM_LerpV4(entry_color, ddsource->active_t, v4(1, 1, 1, 1));
            }
            Draw_Command *entry_bg_cmd = draw_quad(entry_rect, entry_color);
            entry_bg_cmd->serial = entry_bg_serial;

            if (HMM_LenV4(ability_bar_items[i]) > 0 && ddsource != nullptr && ddsource->active == false) {
                draw_quad(entry_rect.inset(8, 8, 8, 8), ability_bar_items[i]);
            }
        }
    }

    // top left UI box
    if (0) {
        UI_PUSH_ID("top left ui");

        uint64_t rng = make_random(276372);

        int64_t bg_serial = draw_get_next_serial();
        int64_t entry_count = 3 + ((int64_t)time_since_startup) % 7;
        float open_t_eased = ease_ping_pong(sidebar_open_t, sidebar_open, ease_out_quart, ease_in_quart);
        Rect cut = full_screen_rect().top_left_rect().grow_right(200).slide(-1 + open_t_eased, 0);
        Rect bg_rect = cut;
        FOR (i, 0, entry_count-1) {
            UI_PUSH_ID(i);

            Button_Settings button_settings = default_button_settings;
            button_settings.color_multiplier = random_color(&rng);
            float height = random_range_float(&rng, 50, 150);
            Rect entry_rect = cut.cut_top(height).inset(5);
            if (ui_button(entry_rect, "", button_settings)->clicked) {
                printf("Clicked %lld\n", i);
            }
        }
        bg_rect = bg_rect.encapsulate(cut);
        Draw_Command *bg_command = draw_quad(bg_rect, bg_rect_color);
        bg_command->serial = bg_serial;

        // open/close tab
        {
            Rect open_close_button_rect = bg_rect.top_right_rect().grow(0, 100, 100, 0);
            if (ui_button(open_close_button_rect, "open/close", default_button_settings)->clicked) {
                sidebar_open = !sidebar_open;
            }

            if (sidebar_open) sidebar_open_t = move_toward(sidebar_open_t, 1, 5 * dt);
            else              sidebar_open_t = move_toward(sidebar_open_t, 0, 5 * dt);
        }
    }

    // srand((int)time_since_startup);
    // int64_t count = 1 + (rand() % 250);
    // for (int64_t i = 0; i < count; i++) {
    //     HMM_Vec2 minimum = {random_range(0, window_width), random_range(0, window_height)};
    //     HMM_Vec2 maximum = minimum + HMM_V2(random_range(50, 100), random_range(50, 100));
    //     draw_quad(minimum, maximum, {random_range(0.5f,1), random_range(0.5f,1), random_range(0.5f,1), 1});
    // }
}

uint64_t last_frame_start_time;

void frame() {
    temp_arena->reset();

    // note(josh): we aren't bothering with a fixed timestep update loop for this example. in a real application you ideally wouldn't have a variable dt like we have here
    dt                 = (float)stm_sec(stm_laptime(&last_frame_start_time));
    time_since_startup = (float)stm_sec(stm_now());

    ui_new_frame((float)dt);
    draw_update();

    app_update();

    ui_end_frame();
    mouse_buttons_down = {};
    mouse_buttons_up   = {};
    mouse_screen_delta = {};
    mouse_scroll = {};

    // render
    {
        sg_pass_action pass_action = {};
        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value = {0.1f, 0.1f, 0.1f, 1.0f};
        sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
        sg_apply_pipeline(textured_pipeline);
        HMM_Mat4 screen_proj = HMM_Orthographic_LH_ZO(0, sapp_widthf(), 0, sapp_heightf(), -1000, 1000);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(screen_proj));

        draw_flush();

        sg_end_pass();
        sg_commit();
    }
}

void event(const sapp_event *evt) {
    int64_t window_height = sapp_height();

    switch (evt->type) {
        case SAPP_EVENTTYPE_KEY_DOWN: {
            break;
        }
        case SAPP_EVENTTYPE_KEY_UP: {
            break;
        }
        case SAPP_EVENTTYPE_CHAR: {
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_DOWN: {
            assert(evt->mouse_button >= SAPP_MOUSEBUTTON_LEFT);
            assert(evt->mouse_button <= SAPP_MOUSEBUTTON_MIDDLE);
            mouse_buttons_held[evt->mouse_button] = true;
            mouse_buttons_down[evt->mouse_button] = true;
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_UP: {
            assert(evt->mouse_button >= SAPP_MOUSEBUTTON_LEFT);
            assert(evt->mouse_button <= SAPP_MOUSEBUTTON_MIDDLE);
            mouse_buttons_held[evt->mouse_button] = false;
            mouse_buttons_up[evt->mouse_button] = true;
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_SCROLL: {
            mouse_scroll = {evt->scroll_x, evt->scroll_y};
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_MOVE: {
            HMM_Vec2 old_pos = mouse_screen_position;
            mouse_screen_position.X = evt->mouse_x;
            mouse_screen_position.Y = evt->mouse_y;
            mouse_screen_position.Y = window_height - mouse_screen_position.Y - 1;
            mouse_screen_delta = mouse_screen_position - old_pos;
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_ENTER: {
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_LEAVE: {
            break;
        }
        case SAPP_EVENTTYPE_RESIZED: {
            break;
        }
        case SAPP_EVENTTYPE_CLIPBOARD_PASTED: {
            break;
        }
        case SAPP_EVENTTYPE_ICONIFIED: {
            break;
        }
        case SAPP_EVENTTYPE_RESTORED: {
            break;
        }
        case SAPP_EVENTTYPE_FOCUSED: {
            break;
        }
        case SAPP_EVENTTYPE_UNFOCUSED: {
            break;
        }
        case SAPP_EVENTTYPE_SUSPENDED: {
            break;
        }
        case SAPP_EVENTTYPE_RESUMED: {
            break;
        }
        case SAPP_EVENTTYPE_QUIT_REQUESTED: {
            break;
        }
        case SAPP_EVENTTYPE_FILES_DROPPED: {
            break;
        }
    }
}

void cleanup() {
    sg_shutdown();
}

void app_init() {
    roboto_font_small = load_font_from_file("resources/fonts/roboto.ttf", 24);
    assert(roboto_font_small != nullptr);
    roboto_font_medium = load_font_from_file("resources/fonts/roboto.ttf", 48);
    assert(roboto_font_medium != nullptr);
    roboto_font_large = load_font_from_file("resources/fonts/roboto.ttf", 96);
    assert(roboto_font_large != nullptr);

    default_text_settings.font   = roboto_font_large;
    default_text_settings.valign = Text_VAlign::CENTER;
    default_text_settings.halign = Text_HAlign::CENTER;
    default_text_settings.color  = v4(0.8f, 0.8f, 0.8f, 1);
}

void init() {
    stm_setup();

    sg_desc desc = {};
    sg_setup(&desc);

    ui_init();
    draw_init();

    last_frame_start_time = stm_now();

    app_init();
}

int main(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    temp_arena = bootstrap_arena(default_allocator(), 16 * 1024 * 1024);

    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.event_cb = event;
    desc.cleanup_cb = cleanup;
    desc.width = 1280;
    desc.height = 720;
    desc.high_dpi = true;
    desc.window_title = "UI Demo";
    desc.logger.func = slog_func;
    sapp_run(&desc);
    return 0;
}