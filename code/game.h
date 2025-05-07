/* date = March 23rd 2025 7:37 pm */

#ifndef GAME_H
#define GAME_H

inline function R_Game_Quad *game_acquire_quad(R_Game_QuadArray *quads);
inline function R_Game_Quad *game_add_rect(R_Game_QuadArray *quads, v3f p, v3f dims, v4f colour);
inline function R_Game_Quad *game_add_tex(R_Game_QuadArray *quads, v3f p, v3f dims, v4f mod);
inline function R_Game_Quad *game_add_tex_clipped(R_Game_QuadArray *quads, v3f p, v3f dims,
                                                  v2f clip_p, v2f clip_dims, v4f mod,
                                                  b32 flip_horizontal);

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
  // entities
  AnimationFrames_PlayerWalk,
  AnimationFrames_GreenSkullWalk,
  
  // attacks
  AnimationFrames_ShadowSlash,
  AnimationFrames_Bite,
  
  // consumables
  AnimationFrames_HealthPotion,
  
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

typedef u64 Consumable_Type;
enum
{
  ConsumableType_HealthPotion,
  ConsumableType_Count,
};

typedef struct
{
  Consumable_Type type;
  v3f p, dims;
  Animation_Config animation;
} Consumable;

typedef u64 StatusEffect_Type;
enum
{
  StatusEffectType_Healing,
  StatusEffectType_Count,
};
typedef struct
{
  b32 is_valid;
  // this value depends on the type. must be nonnegative.
  f32 intensity;
  
  // this counts down to zero.
  f32 duration_current_secs;
  f32 duration_max_secs;
} StatusEffect;

#if 0
// alternate definition of a consumable...
// just define its status effect.......... (and store multiple properties
// of that effect, which is bad.... or maybe there is a way around this?......)
typedef struct
{
  v3f p, dims;
  
  StatusEffect_Type status_effect;
  f32 intensity;
  
  AnimationFrame_For animation_type;
  Animation_Config animation;
} Consumable;
#endif

#define ExperienceGem_G 700.0f
typedef struct Experience_Gem Experience_Gem;
struct Experience_Gem
{
  v3f p;
  v3f dims;
  f32 countdown_secs_before_dead;
  
  v3f dP;
  f32 t_countdown;
  
  Experience_Gem *next;
};

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

#define DefineStaticArray(T, name, cap)\
u64 name##_count;\
T name[cap]

typedef struct
{
  PRNG32 prng;
  
  //DefineStaticArray(Entity, entities, 512);
  u64 entity_count;
  Entity entities[512];
  
  u64 consumables_count;
  Consumable consumables[32];
  
  // NOTE(cj): player status effects.
  // my status effects overwrites, not stacks.
  StatusEffect status_effects[StatusEffectType_Count];
  
  Experience_Gem *experience_gems;
  Experience_Gem *free_experience_gems;
  
  // NOTE(cj): Wave spawner variables
  u32 wave_number;
  f32 next_wave_cooldown_timer;
  f32 next_wave_cooldown_max;
  u32 enemies_to_spawn;
  u32 max_enemies_to_spawn;
  f32 skull_enemy_spawn_timer_sec;
  f32 spawn_cooldown;
  
  // NOTE(cj): Consumable spawner variables
  f32 consumable_spawn_timer_sec;
  f32 consumable_spawn_cooldown;
  
#if defined(DR_DEBUG)
  b32 dbg_draw_entity_wires;
#endif
} Game_State;

inline function Entity *make_entity(Game_State *game, Entity_Type type, Entity_Flag flags);
inline function Entity *make_enemy_green_skull(Game_State *game, v3f p);

#endif //GAME_H
