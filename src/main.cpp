#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#include "external/HandmadeMath.h"

#include "core.h"
#include "draw.h"
#include "ui.h"

sg_pipeline pipeline;

uint64_t last_frame_start_time;

void frame() {
    // note(josh): we aren't bothering with a fixed timestep update loop for this example. in a real application you ideally wouldn't have a variable dt like we have here
    double dt = stm_sec(stm_laptime(&last_frame_start_time));
    double time_since_startup = stm_sec(stm_now());

    ui_new_frame(dt);

    float window_width  = (float)sapp_width();
    float window_height = (float)sapp_height();

    // printf("%f %f\n", mouse_screen_position.X, mouse_screen_position.Y);

    // update
    {
        Rect full_screen_rect = {{0, 0}, {(float)sapp_width(), (float)sapp_height()}};
        Rect thing = full_screen_rect.subrect(0.15f, 0.25f, 0.85f, 0.85f, 0, 0, 0, 0);
        draw_quad(thing, {0.25, 0.25, 0.25, 1});

        Button_Settings button_settings = {};
        Rect cursor_rect = thing.top_rect();
        for (int64_t i = 0; i < 5; i++) {
            ui_push_id(tprint("%lld", i));
            Rect button_rect = cursor_rect.cut_top(75).inset(5, 5, 5, 5);
            if (ui_button(button_rect, "", button_settings)->clicked) {
                printf("Clicked %lld\n", i);
            }
            ui_pop_id();
        }

        // srand((int)time_since_startup);
        // int64_t count = 1 + (rand() % 250);
        // for (int64_t i = 0; i < count; i++) {
        //     HMM_Vec2 minimum = {random_range(0, window_width), random_range(0, window_height)};
        //     HMM_Vec2 maximum = minimum + HMM_V2(random_range(50, 100), random_range(50, 100));
        //     draw_quad(minimum, maximum, {random_range(0.5f,1), random_range(0.5f,1), random_range(0.5f,1), 1});
        // }
    }

    // render
    {
        sg_pass_action pass_action = {};
        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value = {0.1f, 0.1f, 0.1f, 1.0f};
        sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
        sg_apply_pipeline(pipeline);
        HMM_Mat4 screen_proj = HMM_Orthographic_LH_ZO(0, window_width, 0, window_height, -1000, 1000);
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
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_UP: {
            assert(evt->mouse_button >= SAPP_MOUSEBUTTON_LEFT);
            assert(evt->mouse_button <= SAPP_MOUSEBUTTON_MIDDLE);
            mouse_buttons_held[evt->mouse_button] = false;
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_SCROLL: {
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_MOVE: {
            mouse_screen_position.X = evt->mouse_x;
            mouse_screen_position.Y = evt->mouse_y;
            mouse_screen_position.Y = window_height - mouse_screen_position.Y - 1;
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
    pipeline_desc.shader = shd;
    pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4;
    pipeline_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    pipeline_desc.label = "simple pipeline";
    pipeline = sg_make_pipeline(&pipeline_desc);

    last_frame_start_time = stm_now();
}

int main(int argc, char* argv[]) {
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