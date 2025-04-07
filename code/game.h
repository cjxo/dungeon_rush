/* date = March 23rd 2025 7:37 pm */

#ifndef GAME_H
#define GAME_H

typedef union
{
  u64 h[1];
} R_Handle;

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
  Game_Quad *quads;
  u64 capacity;
  u64 count;
  
  // R_Handle tex;
  s32 tex_width;
  s32 tex_height;
} Game_QuadArray;

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
  Animation_Frame frames[4];
} Animation_Config;

typedef struct
{
  Animation_Frame frame;
  b32 is_full_cycle;
  b32 just_switched; // newly switched to a new frame
} Animation_Tick_Result;

typedef u32 Attack_Type;
enum
{
  AttackType_ShadowSlash,
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
};

typedef u64 Entity_Type;
enum
{
  EntityType_Player,
  EntityType_GreenSkull,
  EntityType_Count,
};

typedef struct Entity Entity;
struct Entity
{
  Entity_Type type;
  Entity_Flag flags;
  b32 last_face_dir;
  
  Animation_Config animation;
  
  u32 attack_count;
  Attack attacks[4];
  
  // TODO(cj): Migrate from AABB to OBB, for oriented objects
  v3f p;
  v3f half_dims;
  
  // NOTE(cj): although real, I prefer nonnegative integer
  f32 max_hp;
  f32 current_hp;
};

typedef struct
{
  M_Arena *arena;
  Game_QuadArray quads;
  
  u64 entity_count;
  Entity entities[512];
  
  f32 skull_enemy_spawn_timer_sec;
  Animation_Config walk_animation;
  
#if defined(DR_DEBUG)
  b32 dbg_draw_entity_wires;
  Game_QuadArray dbg_wire_quads;
#endif
} Game_State;

function Animation_Tick_Result tick_animation(Animation_Config *anim, f32 seconds_elapsed);

inline function Entity *make_entity(Game_State *game, Entity_Type type, Entity_Flag flags);
inline function Entity *make_enemy_green_skull(Game_State *game, v3f p);

#endif //GAME_H
