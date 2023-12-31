#include "core.h"
#include "draw.h"
#include "ui.h"

#define UI_DRAG_DROP_ITEM_LAYER (10000)

Array<8, HMM_Vec4> ability_bar_items;

bool sidebar_open;
float sidebar_open_t;

void app_init() {
    ability_bar_items[0] = {0.5, 1, 0.5, 1};
    ability_bar_items[1] = {1, 0.5, 0.5, 1};
    ability_bar_items[2] = {0.25, 1, 1, 1};
}

void app_update(float dt, float time_since_startup) {
    Button_Settings default_button_settings = {};
    HMM_Vec4 bg_rect_color = {0.25, 0.25, 0.25, 1};

    // center scroll list
    {
        ui_push_id("center scroll list");
        defer (ui_pop_id());

        Rect bg_rect = full_screen_rect().center_rect().grow(400, 600, 400, 600).offset(0, 50);
        draw_quad(bg_rect, bg_rect_color);

        Rect content_rect = {};
        push_scroll_view(bg_rect, "scroll view", SCROLL_VIEW_VERTICAL, &content_rect);
        defer (pop_scroll_view());

        uint64_t rng = make_random(276372);
        Rect cursor_rect = content_rect.top_rect();
        for (int64_t i = 0; i < 20; i++) {
            ui_push_id(i);
            defer (ui_pop_id());
            Rect entry_rect = cursor_rect.cut_top(75);
            expand_current_scroll_view(entry_rect);
            Rect button_rect = entry_rect.inset(5);
            Button_Settings button_settings = default_button_settings;
            button_settings.color_multiplier = {random_range_float(&rng, 0.6f, 1.0f), random_range_float(&rng, 0.6f, 1.0f), random_range_float(&rng, 0.6f, 1.0f), 1.0};
            if (ui_button(button_rect, "", button_settings)->clicked) {
            }
        }
    }

    // ability bar
    {
        ui_push_id("ability bar");
        defer (ui_pop_id());

        Rect bar_rect = full_screen_rect().bottom_center_rect().grow(100, 400, 0, 400);
        draw_quad(bar_rect, bg_rect_color);

        Grid_Layout grid = make_grid_layout(bar_rect, 8, 1, Grid_Layout_Kind::ELEMENT_COUNT);
        FOR (i, 0, ability_bar_items.count()-1) {
            ui_push_id(i);
            defer (ui_pop_id());

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
    {
        ui_push_id("top left ui");
        defer (ui_pop_id());

        uint64_t rng = make_random(276372);

        int64_t bg_serial = draw_get_next_serial();
        int64_t entry_count = 3 + ((int64_t)time_since_startup) % 7;
        float open_t_eased = ease_ping_pong(sidebar_open_t, sidebar_open, ease_out_quart, ease_in_quart);
        Rect cut = full_screen_rect().top_left_rect().grow_right(200).slide(-1 + open_t_eased, 0);
        Rect bg_rect = cut;
        FOR (i, 0, entry_count-1) {
            ui_push_id(i);
            defer (ui_pop_id());

            Button_Settings button_settings = default_button_settings;
            button_settings.color_multiplier = {random_range_float(&rng, 0.6f, 1.0f), random_range_float(&rng, 0.6f, 1.0f), random_range_float(&rng, 0.6f, 1.0f), 1.0};
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

sg_pipeline pipeline;

uint64_t last_frame_start_time;

void frame() {
    temp_arena->reset();

    // note(josh): we aren't bothering with a fixed timestep update loop for this example. in a real application you ideally wouldn't have a variable dt like we have here
    double dt = stm_sec(stm_laptime(&last_frame_start_time));
    double time_since_startup = stm_sec(stm_now());

    ui_new_frame(dt);
    draw_update();

    app_update(dt, time_since_startup);

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
        sg_apply_pipeline(pipeline);
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

void init() {
    stm_setup();

    sg_desc desc = {};
    sg_setup(&desc);

    ui_init();
    draw_init();

    sg_shader_desc shader_desc = {};
    shader_desc.label = "simple shader";
    shader_desc.attrs[0].sem_name = "POS";
    shader_desc.attrs[1].sem_name = "COLOR";
    shader_desc.vs.uniform_blocks[0].size = sizeof(HMM_Mat4);
    shader_desc.vs.uniform_blocks[0].uniforms[0] = {"mvp", SG_UNIFORMTYPE_MAT4, 1};
    shader_desc.vs.source = R"DONE(#version 300 es
        layout(location=0) in vec4 in_pos;
        layout(location=1) in vec4 in_color;
        out vec4 fs_color;
        uniform mat4 mvp;
        void main() {
            gl_Position = mvp * in_pos;
            fs_color = in_color;
        }
        )DONE";
    shader_desc.fs.source = R"DONE(#version 300 es
        precision mediump float;
        in vec4 fs_color;
        out vec4 FragColor;
        void main() {
            FragColor = fs_color;
        }
        )DONE";

    sg_shader shd = sg_make_shader(&shader_desc);
    assert(shd.id != SG_INVALID_ID);

    sg_pipeline_desc pipeline_desc = {};
    pipeline_desc.label = "simple pipeline";
    pipeline_desc.shader = shd;
    pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4;
    pipeline_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    pipeline_desc.blend_color = {1, 1, 1, 1},
    pipeline_desc.colors[0].blend.enabled = true;
    pipeline_desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    pipeline_desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA;
    pipeline_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline = sg_make_pipeline(&pipeline_desc);

    last_frame_start_time = stm_now();

    app_init();
}

int main(int argc, char* argv[]) {
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