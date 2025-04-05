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

typedef u64 Entity_Flag;
typedef u64 Entity_Type;

typedef struct
{
  v2f clip_p;
  v2f clip_dims;
} Animation_Frame;

typedef struct Entity Entity;
struct Entity
{
  Entity_Flag flags;
  Entity_Type type;
  b32 last_face_dir;
  
  f32 current_animation_secs;
  f32 duration_animation_secs;
  u32 animation_frame_idx;
  Animation_Frame walking_animation[4];

  v3f p;
};

typedef struct
{
  M_Arena *arena;
  Game_QuadArray quads;
  Entity player;
} Game_State;

#endif //GAME_H
