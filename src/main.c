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
#define WIN_WIDTH  1280
#define WIN_HEIGHT  720
#define TARGET_FPS   60

// Menu text
#define FONT_SIZE 60
#define MENU_TITLE  "Gravi2D - 2D Gravity Sim"
#define MENU_PROMPT "Press Enter or click to start."
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

// Screen types
enum {
    SCREEN_MENU,
    SCREEN_GAME,
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

typedef struct {
    bool  active;
    int   color;
    float size;
    float timer;
    Vec2  pos;
    Vec2  vel;
} Obj;

typedef uint32_t ObjID;

// Special object IDs
enum {
    O_SPECIAL_BLACKHOLE,
    O_SPECIAL_WHITEHOLE,
    O_SPECIAL_COUNT
};

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

// Variable settings (controlled by user input)
static int screen_type = SCREEN_MENU;
static int G = 1; // Gravitational constant
static int spawn_rate_mul = 50;
static bool settings_window = false;
static int setting_selection = SETTING_GRAVITY;
static int obj_radius_max = OBJ_RADIUS_MAX / 2;

// Simulation boundaries
// Use OBJ_SIZE_MAX to slightly extend boundaries off screen
static const float x_max = WIN_WIDTH + OBJ_RADIUS_MAX;
static const float x_min = -OBJ_RADIUS_MAX;
static const float y_max = WIN_HEIGHT + OBJ_RADIUS_MAX;
static const float y_min = -OBJ_RADIUS_MAX;

// Object array
static Obj obj[OBJ_CAP];
static size_t obj_count = 0;
static size_t obj_max = 0;

//-----------------------------------------------------------------------------
// Forward function declarations
//-----------------------------------------------------------------------------

void game_init(void);
void game_update(float dt);
void game_render(void);
void menu_update(void);
void menu_render(void);
void pause_update(void);
void init_object_blackhole(void);
void init_object_whitehole(void);
void render_pause_message(void);
void render_settings_window(void);
void input_update_settings(float dt);

//-----------------------------------------------------------------------------
// Main game loop and raylib window handling
//-----------------------------------------------------------------------------

int main(void)
{
    InitWindow(WIN_WIDTH, WIN_HEIGHT, "Gravi2D");
    SetTargetFPS(TARGET_FPS);

    game_init();

    while (!WindowShouldClose()) {
        switch (screen_type) {
            case SCREEN_MENU: {
                menu_update();
                menu_render();
            } break;
            case SCREEN_GAME: {
                float dt = GetFrameTime();
                game_update(dt);
                game_render();
            } break;
            case SCREEN_PAUSE: {
                float dt = GetFrameTime();
                input_update_settings(dt);
                pause_update();
                game_render();
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

void game_init(void)
{
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
            o->pos.x = GetRandomValue(x_min, x_max);
            o->pos.y = y_min;
            o->vel.x = 0;
            o->vel.y = OBJ_START_SPEED;
        } break;
        case FROM_BOTTOM: {
            o->pos.x = GetRandomValue(x_min, x_max);
            o->pos.y = y_max;
            o->vel.x = 0;
            o->vel.y = -OBJ_START_SPEED;
        } break;
        case FROM_LEFT: {
            o->pos.x = x_min;
            o->pos.y = GetRandomValue(y_min, y_max);
            o->vel.x = OBJ_START_SPEED;
            o->vel.y = 0;
        } break;
        case FROM_RIGHT: {
            o->pos.x = x_max;
            o->pos.y = GetRandomValue(y_min, y_max);
            o->vel.x = -OBJ_START_SPEED;
            o->vel.y = 0;
        } break;
        default: break;
    }

    o->size = GetRandomValue(OBJ_RADIUS_MIN, obj_radius_max);
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

    if (spawn_timer >= OBJ_SPAWN_RATE_BASE * spawn_rate_mul) {
        spawn_timer -= OBJ_SPAWN_RATE_BASE * spawn_rate_mul;

        spawn_object_random();
    }
}

bool is_in_bounds(Vec2 pos)
{
    if (pos.x > x_max || pos.x < x_min) return false;
    if (pos.y > y_max || pos.y < y_min) return false;

    return true;
}

void calculate_velocity(Obj *o, ObjID id)
{
    // O(n) time per call
    // O(n^2) to call this function for all objects
    for (ObjID i = 0; i <= obj_max; i++) {
        if (i == id) continue;

        Obj *o_attract = &obj[i];
        if (!o_attract->active) continue;

        // Calculate the gravitational acceleration exerted on the subject.
        // Newton's Law of Universal Gravitation:
        // F = G * (m1 * m2) / r^2
        // Formula for Gravitational Acceleration (frictionless acceleration):
        // g = G * m / r^2

        float r_sqr = vec2_distance_sqr(o->pos, o_attract->pos);

        if (r_sqr < (o_attract->size * o_attract->size) / 2.0f) {
            // Avoid huge jumps in acceleration
            continue; // Skip; Objects ghost when they are close
        }

        // Use area of 2D circle as mass substitute
        float m = (o_attract->size * o_attract->size) * PI;
        float g = (float)G * m / r_sqr;

        Vec2 slope = vec2_subtract(o->pos, o_attract->pos);
        Vec2 velocity = vec2_scale(vec2_normalize(slope), g);

        if (i == O_SPECIAL_WHITEHOLE) {
            // The Not-So-Big Bang! Special case.
            velocity.x = -velocity.x;
            velocity.y = -velocity.y;
        }

        o->vel.x += velocity.x;
        o->vel.y += velocity.y;
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
            calculate_velocity(o, i);
        }

        o->pos.x += o->vel.x * dt;
        o->pos.y += o->vel.y * dt;
    }
}

void setting_scroll_up(void)
{
    if (setting_selection - 1 >= 0) {
        setting_selection--;
    } else setting_selection = 0;
}

void setting_scroll_down(void)
{
    if (setting_selection + 1 < SETTING_COUNT) {
        setting_selection++;
    } else setting_selection = SETTING_COUNT - 1;
}

void increment_setting(void)
{
    switch (setting_selection) {
        case SETTING_GRAVITY: {
            G++;
            if (G > 1000) {
                G = 1000;
            }
        } break;
        case SETTING_SPAWN_RATE: {
            spawn_rate_mul++;
            if (spawn_rate_mul > 1000) {
                spawn_rate_mul = 1000;
            }
        } break;
        case SETTING_MAX_SIZE: {
            obj_radius_max++;
            if (obj_radius_max > OBJ_RADIUS_MAX) {
                obj_radius_max = OBJ_RADIUS_MAX;
            }
        } break;
        default: break;
    }
}

void decrement_setting(void)
{
    switch (setting_selection) {
        case SETTING_GRAVITY: {
            G--;
            if (G < -1000) {
                G = -1000;
            }
        } break;
        case SETTING_SPAWN_RATE: {
            spawn_rate_mul--;
            if (spawn_rate_mul <= 0) {
                spawn_rate_mul = 1;
            }
        } break;
        case SETTING_MAX_SIZE: {
            obj_radius_max--;
            if (obj_radius_max < OBJ_RADIUS_MIN) {
                obj_radius_max = OBJ_RADIUS_MIN;
            }
        } break;
        default: break;
    }
}

void input_update_settings(float dt)
{
    // Create timer and limit input to 5 actions per second
    const float cooldown_duration = 0.2f;
    static bool action_activated = false;
    static float action_timer = 0.0f;

    if (IsKeyPressed(KEY_TAB)) settings_window = !settings_window;

    if (action_activated) {
        action_timer += dt;
        if (action_timer >= cooldown_duration) {
            action_timer -= cooldown_duration;
            action_activated = false;
        }
    }

    if (settings_window && !action_activated) {
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
    if (IsKeyPressed(KEY_SPACE)) screen_type = SCREEN_PAUSE;

    input_update_settings(dt);
    input_update_blackhole();
    input_update_whitehole();
}

void game_update(float dt)
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

void game_render(void)
{
    BeginDrawing(); {
        ClearBackground(BLACK);

        // Do not render special objects created by user
        for (ObjID i = O_SPECIAL_COUNT; i <= obj_max; i++) {
            Obj *o = &obj[i];            

            if (o->active) render_obj(o);
        }

        if (settings_window) render_settings_window();
        if (screen_type == SCREEN_PAUSE) render_pause_message();

    } EndDrawing();
}

// Settings window, menu screen, and pause screen

void draw_setting_text(int y_offset, Color color, const char *text, ...)
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
    x = (WIN_WIDTH - text_width) / 2;
    y = (WIN_HEIGHT - (WIN_HEIGHT / 4)) + (font_size * y_offset);
    DrawText(buffer, x, y, font_size, color);
}

void render_settings_window(void)
{
    int offset;
    Color color;

    offset = 0;
    color = (setting_selection == SETTING_GRAVITY) ? WHITE : GRAY;
    draw_setting_text(offset, color, SETTING_TEXT_GRAVITY, G);

    offset++;
    color = (setting_selection == SETTING_SPAWN_RATE) ? WHITE : GRAY;
    draw_setting_text(offset, color, SETTING_TEXT_SPAWN_RATE, spawn_rate_mul);

    offset++;
    color = (setting_selection == SETTING_MAX_SIZE) ? WHITE : GRAY;
    draw_setting_text(offset, color, SETTING_TEXT_MAX_SIZE, obj_radius_max);
}

void pause_update(void)
{
    if (IsKeyPressed(KEY_SPACE)) screen_type = SCREEN_GAME;
}

void render_pause_message(void)
{
    int text_width = MeasureText(PAUSE_TEXT, FONT_SIZE);
    int x_text = (WIN_WIDTH - text_width) / 2;
    int y_text = (WIN_HEIGHT / 2) - (FONT_SIZE / 2);

    DrawText(PAUSE_TEXT, x_text, y_text, FONT_SIZE, WHITE);
}

void menu_update(void)
{
    if (IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
        IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        screen_type = SCREEN_GAME;
    }
}

void menu_render(void)
{
    BeginDrawing(); {
        ClearBackground(BLACK);

        int x, y;

        int title_width = MeasureText(MENU_TITLE, FONT_SIZE);
        x = (WIN_WIDTH - title_width) / 2;
        y = (WIN_HEIGHT / 4) - (FONT_SIZE / 2);
        DrawText(MENU_TITLE, x, y, FONT_SIZE, ORANGE);

        int prompt_width = MeasureText(MENU_PROMPT, FONT_SIZE/2);
        x = (WIN_WIDTH - prompt_width) / 2;
        y = (WIN_HEIGHT / 3) + (FONT_SIZE / 2);
        DrawText(MENU_PROMPT, x, y, FONT_SIZE/2, WHITE);

        int text_width = MeasureText(MENU_TEXT, FONT_SIZE/3);
        x = (WIN_WIDTH - text_width) / 2;
        y = WIN_HEIGHT / 2;
        DrawText(MENU_TEXT, x, y, FONT_SIZE/3, WHITE);

        const char *subtitle_text = random_subtitles[subtitle];
        int subtitle_width = MeasureText(subtitle_text, FONT_SIZE/2);
        x = (WIN_WIDTH - subtitle_width) / 2;
        y = WIN_HEIGHT - (WIN_HEIGHT / 4);
        DrawText(subtitle_text, x, y, FONT_SIZE/2, ORANGE);

    } EndDrawing();
}

