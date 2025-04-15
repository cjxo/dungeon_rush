/* date = March 23rd 2025 7:37 pm */

#ifndef GAME_H
#define GAME_H

#define Game_MaxQuads 1024
typedef struct
{
  v3f p;
  v3f dims;
  v4f colour;
  v2f uvs[4];
  u32 tex_id;
} Game_Quad;

typedef struct
{
  f32 advance;
  f32 clip_x, clip_y;
  f32 clip_width, clip_height;
  f32 x_offset;
} Glyph_Data;

typedef struct
{
  Game_Quad *quads;
  u64 capacity;
  u64 count;
  
  // R_Handle tex;
  s32 tex_width;
  s32 tex_height;
} Game_QuadArray;

typedef struct
{
  // NOTE(cj): "Game Stuff" must be drawn first
  Game_QuadArray filled_quads;
  Game_QuadArray wire_quads;
  
  // TODO(cj): We should have UI_Quad soon. We have multiple textures
  // in UI (I think at most two).
  // NOTE(cj): "UI Stuff" must be drawn last
  Game_QuadArray glyph_quads;
  Glyph_Data glyphs[128];
} Renderer_State;

inline function Game_Quad *game_acquire_quad(Game_QuadArray *quads);
inline function Game_Quad *game_add_rect(Game_QuadArray *quads, v3f p, v3f dims, v4f colour);
inline function Game_Quad *game_add_tex(Game_QuadArray *quads, v3f p, v3f dims, v4f mod);
inline function Game_Quad *game_add_tex_clipped(Game_QuadArray *quads, v3f p, v3f dims,
                                                v2f clip_p, v2f clip_dims, v4f mod,
                                                b32 flip_horizontal);

// TODO(cj): Remove this soon. This is temporary. This should only be in the 
// UI render pass
function void game_draw_textf(Renderer_State *renderer, v2f p, String_U8_Const str, v4f colour);

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
