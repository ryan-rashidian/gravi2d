/*
 * Gravi2D - 2D Gravity Simulation
 *
 * Licensed under the MIT License. Copyright (c) 2026 Ryan A. Rashidian
 */

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include <raylib.h>

#include "vec2d.h"

//-----------------------------------------------------------------------------
// Macros for various settings and menu text
//-----------------------------------------------------------------------------

// Window settings
#define INIT_WIN_WIDTH  1280
#define INIT_WIN_HEIGHT  720
#define TARGET_FPS   60

// Menu text
#define FONT_SIZE 60
#define MENU_TITLE  "Gravi2D - 2D Gravity Sim"
#define MENU_PROMPT "Press Enter or click to start."
#define MENU_OPTION_WINDOW "Window: < %s >"
#define MENU_TEXT \
    "Escape - Exit\n" \
    "Spacebar - Pause/Unpause\n" \
    "Tab - Simulation settings\n" \
    "Left-Click - Black Hole ability\n" \
    "Right-Click - White Hole ability"
#define MENU_SUBTITLE_COUNT 5
#define PAUSE_TEXT "PAUSED"
#define SETTING_TEXT_GRAVITY    "Gravitational Constant: < %d >"
#define SETTING_TEXT_SPAWN_RATE "Spawn Rate Multiplier:  < %d >"
#define SETTING_TEXT_MAX_SIZE   "Max size: < %d >"

// Object array and related components
#define OBJ_CAP 4096
#define OBJ_RADIUS_MIN 1.0f
#define OBJ_RADIUS_MAX 50.0f
#define OBJ_RADIUS_SPECIAL 200.0f
#define OBJ_START_SPEED 1.0f
#define OBJ_VEL_REFRESH 0.05f
#define OBJ_SPAWN_RATE_BASE 0.01f

//-----------------------------------------------------------------------------
// Typedefs and enumerations
//-----------------------------------------------------------------------------

// Window options
enum {
    OPT_WINDOWED,
    OPT_FULLSCREEN,
    OPT_WINDOWED_FULLSCREEN,
    OPT_COUNT
};

// Screen types
enum {
    SCREEN_MENU,
    SCREEN_SIM,
    SCREEN_PAUSE
};

// Settings options
enum {
    SETTING_GRAVITY,
    SETTING_SPAWN_RATE,
    SETTING_MAX_SIZE,
    SETTING_COUNT
};

// Enumeration for spawning directions
enum {
    FROM_TOP,
    FROM_BOTTOM,
    FROM_LEFT,
    FROM_RIGHT
};

// Object colors for rendering
enum {
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_PINK,
    COLOR_PURPLE,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_SKYBLUE,
    COLOR_COUNT
};

// Special object IDs
enum {
    O_SPECIAL_BLACKHOLE,
    O_SPECIAL_WHITEHOLE,
    O_SPECIAL_COUNT
};

typedef struct {
    bool  active;
    int   color;
    float size;
    float timer;
    Vec2  pos;
    Vec2  vel;
} Obj;

typedef uint32_t ObjID;

//-----------------------------------------------------------------------------
// Global variables and constants
//-----------------------------------------------------------------------------

// Random subtitle messages
static const char *random_subtitles[MENU_SUBTITLE_COUNT] = {
    "Made with all natural, free-range circles!",
    "Circles imported from Switzerland.",
    "Trusted by 2D aerospace engineers everywhere.",
    "I wonder how the universe computes O(n^2) loops...",
    "\"We are all made of circle dust\" - Some dude on TV"
};
static int subtitle;

// Window option strings for main menu
static const char *window_opts[OPT_COUNT] = {
    "Windowed",
    "Fullscreen",
    "Windowed-Fullscreen"
};

static struct {
    int width, height;
} window;

// Settings (controlled by user)
static struct {
    int window;
    int screen;
} settings;

// Variable parameters (controlled by user)
static struct {
    int G;
    int spawn_mul;
    bool popup_window;
    int popup_selection;
    int obj_radius_max;
} params;

// World/Simulation boundaries
static struct {
    int x_max, x_min;
    int y_max, y_min;
} world;

// Object array
static Obj obj[OBJ_CAP];
static size_t obj_count = 0;
static size_t obj_max = 0;

//-----------------------------------------------------------------------------
// Forward function declarations
//-----------------------------------------------------------------------------

void sim_init(void);
void sim_update(float dt);
void sim_render(void);
void menu_update(void);
void menu_render(void);
void pause_update(void);
void init_object_blackhole(void);
void init_object_whitehole(void);
void render_pause_message(void);
void render_settings_window(void);
void input_update_params(float dt);

//-----------------------------------------------------------------------------
// Main game loop and raylib window handling
//-----------------------------------------------------------------------------

int main(void)
{
    InitWindow(INIT_WIN_WIDTH, INIT_WIN_HEIGHT, "Gravi2D");
    SetTargetFPS(TARGET_FPS);

    sim_init();

    while (!WindowShouldClose()) {
        switch (settings.screen) {
            case SCREEN_MENU: {
                menu_update();
                menu_render();
            } break;
            case SCREEN_SIM: {
                float dt = GetFrameTime();
                sim_update(dt);
                sim_render();
            } break;
            case SCREEN_PAUSE: {
                float dt = GetFrameTime();
                input_update_params(dt);
                pause_update();
                sim_render();
            }
            default: break;
        }
    }

    CloseWindow();
    return 0;
}

//-----------------------------------------------------------------------------
// Function definitions
//-----------------------------------------------------------------------------

void get_sim_boundaries(void)
{
    int width = GetScreenWidth();
    int height = GetScreenHeight();

    world.x_max = width + OBJ_RADIUS_MAX;
    world.x_min = -OBJ_RADIUS_MAX;
    world.y_max = height + OBJ_RADIUS_MAX;
    world.y_min = -OBJ_RADIUS_MAX;
    window.width = width;
    window.height = height;
}

void settings_init(void)
{
    settings.window = OPT_WINDOWED;
    settings.screen = SCREEN_MENU;
    params.G = 1; // Gravitational constant
    params.spawn_mul = 50;
    params.popup_window = false;
    params.popup_selection = SETTING_GRAVITY;
    params.obj_radius_max = OBJ_RADIUS_MAX / 2;
    get_sim_boundaries();
}

void sim_init(void)
{
    settings_init();
    subtitle = GetRandomValue(0, MENU_SUBTITLE_COUNT - 1);
    init_object_blackhole();
    init_object_whitehole();
}

// Object array management

ObjID obj_alloc(void)
{
    // Reserve special objects at start of obj array
    for (ObjID i = O_SPECIAL_COUNT; i < OBJ_CAP; i++) {
        Obj *o = &obj[i];
        if (!o->active) {
            if (i > obj_max) obj_max = i;

            o->timer = 0.0f;
            o->active = true;
            obj_count++;
            return i;
        }
    }

    assert(0);
    return 0;
}

void obj_free(Obj *o)
{
    o->active = false;
    obj_count--;
}

// Spawning and object initialization

void spawn_object_random(void)
{
    if (obj_count >= OBJ_CAP - O_SPECIAL_COUNT - 1) return;

    ObjID id = obj_alloc();
    Obj *o = &obj[id];

    // Get a random spawn location
    int direction = GetRandomValue(0, 3);
    switch (direction) {
        case FROM_TOP: {
            o->pos.x = GetRandomValue(world.x_min, world.x_max);
            o->pos.y = world.y_min;
            o->vel.x = 0;
            o->vel.y = OBJ_START_SPEED;
        } break;
        case FROM_BOTTOM: {
            o->pos.x = GetRandomValue(world.x_min, world.x_max);
            o->pos.y = world.y_max;
            o->vel.x = 0;
            o->vel.y = -OBJ_START_SPEED;
        } break;
        case FROM_LEFT: {
            o->pos.x = world.x_min;
            o->pos.y = GetRandomValue(world.y_min, world.y_max);
            o->vel.x = OBJ_START_SPEED;
            o->vel.y = 0;
        } break;
        case FROM_RIGHT: {
            o->pos.x = world.x_max;
            o->pos.y = GetRandomValue(world.y_min, world.y_max);
            o->vel.x = -OBJ_START_SPEED;
            o->vel.y = 0;
        } break;
        default: break;
    }

    o->size = GetRandomValue(OBJ_RADIUS_MIN, params.obj_radius_max);
    o->color = GetRandomValue(0, COLOR_COUNT);
}

void init_object_blackhole(void)
{
    Obj *o = &obj[O_SPECIAL_BLACKHOLE];
    o->size = OBJ_RADIUS_SPECIAL;
    o->timer = 0.0f;
}

void init_object_whitehole(void)
{
    Obj *o = &obj[O_SPECIAL_WHITEHOLE];
    o->size = OBJ_RADIUS_SPECIAL;
    o->timer = 0.0f;
}

// Object systems

void spawn_update(float dt)
{
    static float spawn_timer = 0.0f;

    spawn_timer += dt;

    if (spawn_timer >= OBJ_SPAWN_RATE_BASE * params.spawn_mul) {
        spawn_timer -= OBJ_SPAWN_RATE_BASE * params.spawn_mul;

        spawn_object_random();
    }
}

bool is_in_bounds(Vec2 pos)
{
    if (pos.x > world.x_max || pos.x < world.x_min) return false;
    if (pos.y > world.y_max || pos.y < world.y_min) return false;

    return true;
}

void calculate_velocity(ObjID id)
{
    // O(n) time per call
    // O(n^2) to call this function for all objects
    Obj *o_subject = &obj[id];

    for (ObjID i = 0; i <= obj_max; i++) {
        if (i == id) continue;

        Obj *o_attract = &obj[i];
        if (!o_attract->active) continue;

        // Calculate the gravitational acceleration exerted on the subject.
        // Newton's Law of Universal Gravitation:
        // F = G * (m1 * m2) / r^2
        // Formula for Gravitational Acceleration (frictionless acceleration):
        // g = G * m / r^2

        float r_sqr = vec2_distance_sqr(o_subject->pos, o_attract->pos);

        if (r_sqr < (o_attract->size * o_attract->size) / 2.0f) {
            // Avoid huge jumps in acceleration
            continue; // Skip; Objects ghost when they are close
        }

        // Use area of 2D circle as mass substitute
        float m = (o_attract->size * o_attract->size) * PI;
        float g = (float)params.G * m / r_sqr;

        Vec2 slope = vec2_subtract(o_subject->pos, o_attract->pos);
        Vec2 v_accel = vec2_scale(vec2_normalize(slope), g);

        if (i == O_SPECIAL_WHITEHOLE) {
            // The Not-So-Big Bang! Special case.
            v_accel.x = -v_accel.x;
            v_accel.y = -v_accel.y;
        }

        o_subject->vel.x += v_accel.x;
        o_subject->vel.y += v_accel.y;
    }
}

void physics_update(float dt)
{
    for (ObjID i = O_SPECIAL_COUNT; i <= obj_max; i++) {
        Obj *o = &obj[i];
        if (!o->active) continue;

        if (!is_in_bounds(o->pos)) {
            obj_free(o);
            continue;
        }

        o->timer += dt;
        if (o->timer >= OBJ_VEL_REFRESH) {
            // Slight optimization for objects to only update on pre-defined
            // timer 'tick', decoupled from frame-rate and phase shifted.
            // NOTE: It might be better to just update everything on each
            // frame, perfectly syncronized.
            o->timer -= OBJ_VEL_REFRESH;
            calculate_velocity(i);
        }

        o->pos.x += o->vel.x * dt;
        o->pos.y += o->vel.y * dt;
    }
}

void setting_scroll_up(void)
{
    if (params.popup_selection - 1 >= 0) {
        params.popup_selection--;
    } else params.popup_selection = 0;
}

void setting_scroll_down(void)
{
    if (params.popup_selection + 1 < SETTING_COUNT) {
        params.popup_selection++;
    } else params.popup_selection = SETTING_COUNT - 1;
}

void increment_setting(void)
{
    switch (params.popup_selection) {
        case SETTING_GRAVITY: {
            params.G++;
            if (params.G > 1000) {
                params.G = 1000;
            }
        } break;
        case SETTING_SPAWN_RATE: {
            params.spawn_mul++;
            if (params.spawn_mul > 1000) {
                params.spawn_mul = 1000;
            }
        } break;
        case SETTING_MAX_SIZE: {
            params.obj_radius_max++;
            if (params.obj_radius_max > OBJ_RADIUS_MAX) {
                params.obj_radius_max = OBJ_RADIUS_MAX;
            }
        } break;
        default: break;
    }
}

void decrement_setting(void)
{
    switch (params.popup_selection) {
        case SETTING_GRAVITY: {
            params.G--;
            if (params.G < -1000) {
                params.G = -1000;
            }
        } break;
        case SETTING_SPAWN_RATE: {
            params.spawn_mul--;
            if (params.spawn_mul <= 0) {
                params.spawn_mul = 1;
            }
        } break;
        case SETTING_MAX_SIZE: {
            params.obj_radius_max--;
            if (params.obj_radius_max < OBJ_RADIUS_MIN) {
                params.obj_radius_max = OBJ_RADIUS_MIN;
            }
        } break;
        default: break;
    }
}

void input_update_params(float dt)
{
    // Create timer and limit input to 5 actions per second
    const float cooldown_duration = 0.2f;
    static bool action_activated = false;
    static float action_timer = 0.0f;

    if (IsKeyPressed(KEY_TAB)) params.popup_window = !params.popup_window;

    if (action_activated) {
        action_timer += dt;
        if (action_timer >= cooldown_duration) {
            action_timer -= cooldown_duration;
            action_activated = false;
        }
    }

    if (params.popup_window && !action_activated) {
        if (IsKeyDown(KEY_UP)) {
            action_activated = true;
            setting_scroll_up();
        }
        if (IsKeyDown(KEY_DOWN)) {
            action_activated = true;
            setting_scroll_down();
        }
        if (IsKeyDown(KEY_LEFT)) {
            action_activated = true;
            decrement_setting();
        }
        if (IsKeyDown(KEY_RIGHT)) {
            action_activated = true;
            increment_setting();
        }
    }
}

void input_update_blackhole(void)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        obj[O_SPECIAL_BLACKHOLE].active = true;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vec2 mouse_pos;
        mouse_pos.x = GetMouseX();
        mouse_pos.y = GetMouseY();
        obj[O_SPECIAL_BLACKHOLE].pos = mouse_pos;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        obj[O_SPECIAL_BLACKHOLE].active = false;
    }
}

void input_update_whitehole(void)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        obj[O_SPECIAL_WHITEHOLE].active = true;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vec2 mouse_pos;
        mouse_pos.x = GetMouseX();
        mouse_pos.y = GetMouseY();
        obj[O_SPECIAL_WHITEHOLE].pos = mouse_pos;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        obj[O_SPECIAL_WHITEHOLE].active = false;
    }
}

void input_update(float dt)
{
    if (IsKeyPressed(KEY_SPACE)) settings.screen = SCREEN_PAUSE;

    input_update_params(dt);
    input_update_blackhole();
    input_update_whitehole();
}

void sim_update(float dt)
{
    input_update(dt);
    spawn_update(dt);
    physics_update(dt);
}

// Game rendering functions

void render_obj(Obj *o)
{
    Color rl_color;

    switch(o->color) {
        case COLOR_RED:     { rl_color = RED;     } break;
        case COLOR_ORANGE:  { rl_color = ORANGE;  } break;
        case COLOR_PINK:    { rl_color = PINK;    } break;
        case COLOR_PURPLE:  { rl_color = PURPLE;  } break;
        case COLOR_YELLOW:  { rl_color = YELLOW;  } break;
        case COLOR_GREEN:   { rl_color = GREEN;   } break;
        case COLOR_BLUE:    { rl_color = BLUE;    } break;
        case COLOR_SKYBLUE: { rl_color = SKYBLUE; } break;
        default: { rl_color = RED; } break;
    }

    DrawCircle(o->pos.x, o->pos.y, o->size, rl_color);
}

void sim_render(void)
{
    BeginDrawing(); {
        ClearBackground(BLACK);

        // Do not render special objects created by user
        for (ObjID i = O_SPECIAL_COUNT; i <= obj_max; i++) {
            Obj *o = &obj[i];            

            if (o->active) render_obj(o);
        }

        if (params.popup_window) render_settings_window();
        if (settings.screen == SCREEN_PAUSE) render_pause_message();

    } EndDrawing();
}

// Settings window, menu screen, and pause screen

void draw_option_text(int y_offset, Color color, const char *text, ...)
{
    char buffer[256];
    int text_width;
    int x, y;
    int font_size = FONT_SIZE / 2;
    va_list va;

    va_start(va, text);
    vsnprintf(buffer, 256, text, va);
    va_end(va);

    text_width = MeasureText(buffer, font_size);
    x = (window.width - text_width) / 2;
    y = (window.height - (window.height / 4)) + (font_size * y_offset);
    DrawText(buffer, x, y, font_size, color);
}

void render_settings_window(void)
{
    int offset;
    Color color;

    offset = 0;
    color = (params.popup_selection == SETTING_GRAVITY) ? WHITE : GRAY;
    draw_option_text(
        offset, color,
        SETTING_TEXT_GRAVITY,
        params.G
    );

    offset++;
    color = (params.popup_selection == SETTING_SPAWN_RATE) ? WHITE : GRAY;
    draw_option_text(
        offset, color,
        SETTING_TEXT_SPAWN_RATE,
        params.spawn_mul
    );

    offset++;
    color = (params.popup_selection == SETTING_MAX_SIZE) ? WHITE : GRAY;
    draw_option_text(
        offset, color,
        SETTING_TEXT_MAX_SIZE,
        params.obj_radius_max
    );
}

void pause_update(void)
{
    if (IsKeyPressed(KEY_SPACE)) settings.screen = SCREEN_SIM;
}

void render_pause_message(void)
{
    int text_width = MeasureText(PAUSE_TEXT, FONT_SIZE);
    int x_text = (window.width - text_width) / 2;
    int y_text = (window.height / 2) - (FONT_SIZE / 2);

    DrawText(PAUSE_TEXT, x_text, y_text, FONT_SIZE, WHITE);
}

void set_window_option(void)
{
    switch (settings.window) {
        case OPT_WINDOWED: break; // Default
        case OPT_FULLSCREEN: {
            ToggleFullscreen();
            get_sim_boundaries();
        } break;
        case OPT_WINDOWED_FULLSCREEN: {
            ToggleBorderlessWindowed();
            get_sim_boundaries();
        } break;
        default: break;
    }
}

void menu_update(void)
{
    if (IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
        IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        settings.screen = SCREEN_SIM;
        set_window_option();
    }
    
    if (IsKeyPressed(KEY_LEFT)) {
        settings.window--;
        if (settings.window < OPT_WINDOWED) {
            settings.window = OPT_WINDOWED_FULLSCREEN;
        }
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        settings.window++;
        if (settings.window > OPT_WINDOWED_FULLSCREEN) {
            settings.window = OPT_WINDOWED;
        }
    }
}

void menu_render(void)
{
    BeginDrawing(); {
        ClearBackground(BLACK);

        int x, y;

        int title_width = MeasureText(MENU_TITLE, FONT_SIZE);
        x = (window.width - title_width) / 2;
        y = (window.height / 4) - (FONT_SIZE / 2);
        DrawText(MENU_TITLE, x, y, FONT_SIZE, ORANGE);

        int prompt_width = MeasureText(MENU_PROMPT, FONT_SIZE/2);
        x = (window.width - prompt_width) / 2;
        y = (window.height / 3) + (FONT_SIZE / 2);
        DrawText(MENU_PROMPT, x, y, FONT_SIZE/2, WHITE);

        draw_option_text(
            0, WHITE, MENU_OPTION_WINDOW,
            window_opts[settings.window]
        );

        int text_width = MeasureText(MENU_TEXT, FONT_SIZE/3);
        x = (window.width - text_width) / 2;
        y = window.height / 2;
        DrawText(MENU_TEXT, x, y, FONT_SIZE/3, GRAY);

        const char *subtitle_text = random_subtitles[subtitle];
        int subtitle_width = MeasureText(subtitle_text, FONT_SIZE/2);
        x = (window.width - subtitle_width) / 2;
        y = window.height - (window.height / 8);
        DrawText(subtitle_text, x, y, FONT_SIZE/2, ORANGE);

    } EndDrawing();
}

