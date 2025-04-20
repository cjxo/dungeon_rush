/* date = March 23rd 2025 7:37 pm */

#ifndef GAME_H
#define GAME_H

inline function R_Game_Quad *game_acquire_quad(R_Game_QuadArray *quads);
inline function R_Game_Quad *game_add_rect(R_Game_QuadArray *quads, v3f p, v3f dims, v4f colour);
inline function R_Game_Quad *game_add_tex(R_Game_QuadArray *quads, v3f p, v3f dims, v4f mod);
inline function R_Game_Quad *game_add_tex_clipped(R_Game_QuadArray *quads, v3f p, v3f dims,
                                                  v2f clip_p, v2f clip_dims, v4f mod,
                                                  b32 flip_horizontal);

inline function R_UI_Quad *ui_acquire_quad(R_UI_QuadArray *quads);
function v2f               ui_add_stringf(R_UI_QuadArray *quads, R_Font *font, v2f p, v4f colour, String_U8_Const str, ...);
function v2f               ui_query_string_dimsf(R_Font *font, String_U8_Const str, ...);

function R_UI_Quad *       ui_add_quad(R_UI_QuadArray *quads, v2f p, v2f dims,
                                       f32 smoothness, f32 vertex_roundness, v4f vertex_colours,
                                       f32 border_thickness, v4f border_colour);
function R_UI_Quad *       ui_add_quad_per_vertex_colours(R_UI_QuadArray *quads, v2f p, v2f dims,
                                                          f32 smoothness,
                                                          f32 vertex_roundness, v4f vertex_top_left_c, v4f vertex_bottom_left_c,
                                                          v4f vertex_top_right_c, v4f vertex_bottom_right_c,
                                                          f32 border_thickness, v4f border_colour);

function R_UI_Quad *       ui_add_quad_shadowed(R_UI_QuadArray *quads, v2f p, v2f dims,
                                                f32 smoothness,
                                                f32 vertex_roundness, v4f colour_per_vertex,
                                                f32 border_thickness, v4f border_colour,
                                                v2f shadow_offset,
                                                v2f shadow_dims_offset,
                                                v4f shadow_colour_per_vertex,
                                                f32 shadow_smoothness);

// ----------------------- //
typedef struct
{
  v2f clip_p;
  v2f clip_dims;
  v2f offset; // in texel space
} Animation_Frame;

typedef struct
{
  f32 current_secs;
  f32 duration_secs;
  u32 frame_idx;
} Animation_Config;

typedef struct
{
  Animation_Frame frame;
  b32 is_full_cycle;
  b32 just_switched; // newly switched to a new frame
} Animation_Tick_Result;

typedef struct
{
  Animation_Frame *frames;
  u64 count;
} Animation_Frames;

typedef u32 AnimationFrame_For;
enum
{
  // ents
  AnimationFrames_PlayerWalk,
  AnimationFrames_GreenSkullWalk,
  
  // attacks
  AnimationFrames_ShadowSlash,
  AnimationFrames_Bite,
  
  AnimationFrames_Count,
};

function Animation_Config create_animation_config(f32 duration_secs);
function Animation_Tick_Result tick_animation(Animation_Config *anim, Animation_Frames frame_info, f32 seconds_elapsed);
function Animation_Frames get_animation_frames(AnimationFrame_For frame_for);

typedef u32 Attack_Type;
enum
{
  AttackType_ShadowSlash,
  AttackType_Bite,
  AttackType_Count,
};

typedef struct
{
  Attack_Type type;
  Animation_Config animation;
  f32 current_secs;
  f32 interval_secs;
  f32 damage;
} Attack;

typedef u64 Entity_Flag;
enum
{
  EntityFlag_Hostile = 0x1,
  EntityFlag_DeleteMe = 0x2,
};

typedef u64 Entity_Type;
enum
{
  EntityType_Player,
  EntityType_GreenSkull,
  EntityType_Count,
};

typedef struct
{
  Animation_Config walk_animation;
  u32 attack_count;
  Attack attacks[4];
  
  u32 level;
  u32 current_experience;
  u32 max_experience;
} Player;

typedef struct
{
  Animation_Config animation;
  Attack attack;
} Enemy;

typedef struct Entity Entity;
struct Entity
{
  Entity_Type type;
  Entity_Flag flags;
  b32 last_face_dir;
  
  // TODO(cj): Migrate from AABB to OBB, for oriented objects
  v3f p;
  v3f dims;
  
  // NOTE(cj): ATTRIBUTES area
  // NOTE(cj): although real, I prefer nonnegative integer
  f32 max_hp;
  f32 current_hp;
  
  union
  {
    Player player;
    Enemy enemy;
  };
};

typedef struct
{
  M_Arena *arena;
  R_InputForRendering *renderer;
} Game_Memory;

typedef struct
{
  PRNG32 prng;
  
  u64 entity_count;
  Entity entities[512];
  
  u32 wave_number;
  f32 next_wave_cooldown_timer;
  f32 next_wave_cooldown_max;
  u32 enemies_to_spawn;
  u32 max_enemies_to_spawn;
  f32 skull_enemy_spawn_timer_sec;
  f32 spawn_cooldown;
  
#if defined(DR_DEBUG)
  b32 dbg_draw_entity_wires;
#endif
} Game_State;

inline function Entity *make_entity(Game_State *game, Entity_Type type, Entity_Flag flags);
inline function Entity *make_enemy_green_skull(Game_State *game, v3f p);

#endif //GAME_H
