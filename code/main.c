#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define NOMINMAX
#include <windows.h>
#include <timeapi.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>

#define STB_IMAGE_IMPLEMENTATION
#include "./ext/stb_image.h"

#include "base.h"
#include "windows_stuff.h"
#include "prng.h"
#include "mathematical_objects.h"
#include "renderer.h"
#include "ui.h"
#include "game.h"

#include "base.c"
#include "windows_stuff.c"
#include "mathematical_objects.c"
#include "renderer.c"
#include "prng.c"
#include "ui.c"

inline function R_Game_Quad *
game_acquire_quad(R_Game_QuadArray *quads)
{
  Assert(quads->count < quads->capacity);
  R_Game_Quad *result = quads->quads + quads->count++;
  return(result);
}

inline function R_Game_Quad *
game_add_rect(R_Game_QuadArray *quads, v3f p, v3f dims, v4f colour)
{
  R_Game_Quad *result = game_acquire_quad(quads);
  result->p = p;
  result->dims = dims;
  result->colour = colour;
  result->tex_id = 0;
  return(result);
}

inline function R_Game_Quad *
game_add_tex(R_Game_QuadArray *quads, v3f p, v3f dims, v4f mod)
{
  R_Game_Quad *result = game_add_rect(quads, p, dims, mod);
  result->uvs[0] = (v2f){ 0.0f, 1.0f };
  result->uvs[1] = (v2f){ 0.0f, 0.0f };
  result->uvs[2] = (v2f){ 1.0f, 1.0f };
  result->uvs[3] = (v2f){ 1.0f, 0.0f };
  result->tex_id = quads->tex.id;
  return(result);
}

inline function R_Game_Quad *
game_add_tex_clipped(R_Game_QuadArray *quads,
                     v3f p, v3f dims,
                     v2f clip_p, v2f clip_dims,
                     v4f mod,
                     b32 flip_horizontal)
{
  R_Texture2D tex = quads->tex;
  R_Game_Quad *result = game_add_rect(quads, p, dims, mod);
  f32 x_start = clip_p.x / (f32)tex.width;
  f32 x_end = (clip_p.x + clip_dims.x) / (f32)tex.width;
  
  f32 y_start = clip_p.y / (f32)tex.height;
  f32 y_end = (clip_p.y + clip_dims.y) / (f32)tex.height;
  
  if (flip_horizontal)
  {
    result->uvs[0] = (v2f){ x_end, y_end };
    result->uvs[1] = (v2f){ x_end, y_start };
    result->uvs[2] = (v2f){ x_start, y_end };
    result->uvs[3] = (v2f){ x_start, y_start };
  }
  else
  {
    result->uvs[0] = (v2f){ x_start, y_end };
    result->uvs[1] = (v2f){ x_start, y_start };
    result->uvs[2] = (v2f){ x_end, y_end };
    result->uvs[3] = (v2f){ x_end, y_start };
  }
  result->tex_id = tex.id;
  return(result);
}

function Animation_Config
create_animation_config(f32 duration_secs)
{
  Animation_Config result;
  result.current_secs = 0.0f;
  result.duration_secs = duration_secs;
  result.frame_idx = 0;
  return(result);
}

inline function Entity *
make_entity(Game_State *game, Entity_Type type, Entity_Flag flags)
{
  Assert((game->entity_count + 1) < ArrayCount(game->entities));
  Entity *result = game->entities + game->entity_count++;
  ClearStructP(result);
  result->type = type;
  result->flags = flags;
  return(result);
}

inline function Entity *
make_enemy_green_skull(Game_State *game, v3f p)
{
  Entity *result = make_entity(game, EntityType_GreenSkull, EntityFlag_Hostile);
  result->p = p;
  result->dims = (v3f){ 64, 64, 0 };
  result->max_hp = 12.0f;
  result->current_hp = result->max_hp;
  
  Animation_Config *anim = &result->enemy.animation;
  anim->current_secs = 0.0f;
  anim->duration_secs = 0.1f;
  anim->frame_idx = 0;
  
  result->dims = (v3f){ 64, 64, 0 };
  result->enemy.attack = (Attack)
  {
    .type = AttackType_Bite,
    .current_secs = 0.0f,
    .interval_secs = 1.0f,
    .damage = 4,
  };
  
  result->enemy.attack.animation = create_animation_config(0.04f);
  
  return(result);
}

function Animation_Frames
get_animation_frames(AnimationFrame_For frame_for)
{
  Assert(frame_for < AnimationFrames_Count);
  
  Animation_Frames result={0};
  
  static Animation_Frame player_walk[] =
  {
    {{0.0f,16.0f},{16.0f,16.0f},},
    {{16.0f,16.0f},{16.0f,16.0f},},
    {{32.0f,16.0f},{16.0f,16.0f},},
    {{48.0f,16.0f},{16.0f,16.0f},},
  };
  
  static Animation_Frame green_skull_walk[] =
  {
    {{64.0f,0.0f},{16.0f,16.0f}},
    {{80.0f,0.0f},{16.0f,16.0f}},
    {{96.0f,0.0f},{16.0f,16.0f}},
    {{112.0f,0.0f},{16.0f,16.0f}},
  };
  
  // 
  static Animation_Frame shadow_slash_attack[] =
  {
    {{0.0f,32.0f},{32.0f,32.0f},{-11.0f,-13.0f},},
    {{32.0f,32.0f},{16.0f,32.0f},{-3.0f,-16.0f},},
    {{48.0f,32.0f},{32.0f,32.0f},{14.0f,8.0f},},
    {{80.0f,32.0f},{32.0f,32.0f},{12.0f,11.0f},},
  };
  
  static Animation_Frame bite_attack[] =
  {
    {{0.0f,64.0f},{64.0f,48.0f},},
    {{64.0f,64.0f},{64.0f,48.0f},},
    {{128.0f,64.0f},{64.0f,48.0f},},
    {{192.0f, 64.0f},{64.0f,48.0f},},
  };
  
  static Animation_Frames table[] =
  {
    [AnimationFrames_PlayerWalk] = { player_walk, ArrayCount(player_walk) },
    [AnimationFrames_GreenSkullWalk] = { green_skull_walk, ArrayCount(green_skull_walk) },
    [AnimationFrames_ShadowSlash] = { shadow_slash_attack, ArrayCount(shadow_slash_attack) },
    [AnimationFrames_Bite] = { bite_attack, ArrayCount(bite_attack) },
  };
  
  result = table[frame_for];
  return(result);
}

function void
game_init(Game_State *game)
{
  // player entity
  {
    Entity *player = make_entity(game, EntityType_Player, 0);
    player->flags = 0;
    player->type = EntityType_Player;
    player->last_face_dir = 0;
    // NOTE(cj): the player's base HP is 50
    player->max_hp = player->current_hp = 50.0f;
    
    player->player.walk_animation = create_animation_config(0.15f);
    
    player->dims = (v3f){ 64, 64, 0 };
    player->player.attack_count = 1;
    player->player.attacks[0] = (Attack)
    {
      .type = AttackType_ShadowSlash,
      .current_secs = 1.0f,
      .interval_secs = 1.0f,
      .damage = 6,
    };
    
    player->player.attacks[0].animation = create_animation_config(0.04f);
    
    player->player.level = 1;
    player->player.current_experience = 0;
    player->player.max_experience = 20;
  }
  
  prng32_seed(&game->prng, 13123);
  
  //
  // NOTE(cj): Wave stufff
  //
  game->wave_number += 1;
  game->next_wave_cooldown_max = 4.0f;
  game->next_wave_cooldown_timer = game->next_wave_cooldown_max;
  game->enemies_to_spawn = 0;
  game->max_enemies_to_spawn = 10;
  game->skull_enemy_spawn_timer_sec = 0.0f;
  game->spawn_cooldown = 2.0f;
}

function Animation_Tick_Result
tick_animation(Animation_Config *anim, Animation_Frames frame_info, f32 seconds_elapsed)
{
  Animation_Frame *frames = frame_info.frames;
  u64 frame_count = frame_info.count;
  
  Animation_Tick_Result result;
  result.frame = frames[anim->frame_idx];
  result.is_full_cycle = 0;
  result.just_switched = 0;
  b32 time_is_up = anim->current_secs >= anim->duration_secs;
  if (time_is_up)
  {
    anim->current_secs = 0.0f;
    anim->frame_idx += 1;
    result.just_switched = 1;
    if (anim->frame_idx == frame_count)
    {
      anim->frame_idx = 0;
      result.is_full_cycle = 1;
    }
  }
  else
  {
    anim->current_secs += seconds_elapsed;
  } 
  
  return(result);
}

function b32
check_aabb_collision_xy(v2f center_a, v2f half_dims_a,
                        v2f center_b, v2f half_dims_b)
{
  f32 c_dist, r_add;
  
  c_dist = absolute_value_f32(center_a.x - center_b.x);
  r_add = half_dims_a.x + half_dims_b.x;
  if (c_dist > r_add)
  {
    return 0;
  }
  
  c_dist = absolute_value_f32(center_a.y - center_b.y);
  r_add = half_dims_a.y + half_dims_b.y;
  if (c_dist > r_add)
  {
    return 0;
  }
  
  return 1;
}

function void
draw_health_bar(R_Game_QuadArray *quads, Entity *entity)
{
  // a disadvantage of a center origin rect...
  f32 percent_occupy = (entity->current_hp / entity->max_hp);
  f32 percent_residue = 1.0f - percent_occupy;
  v3f hp_p = entity->p;
  hp_p.y += 48.0f;
  v3f hp_p_green = hp_p;
  hp_p_green.x -= percent_residue * 128.0f * 0.5f;
  
  game_add_rect(quads, hp_p, (v3f){ 128.0f, 8.0f, 0.0f }, (v4f){ 1, 0, 0, 1 });
  game_add_rect(quads, hp_p_green, (v3f){ 128.0f*percent_occupy, 8.0f, 0.0f }, (v4f){ 0, 1, 0, 1 });
}

function void
game_update_and_render(Game_State *game, OS_Input *input, Game_Memory *memory, f32 game_update_secs)
{
  R_InputForRendering *renderer = memory->renderer;
  // the player is always at 0th idx
  Entity *player = game->entities;
  
  if (game->next_wave_cooldown_timer <= game->next_wave_cooldown_max)
  {
    if (game->entity_count == 1)
    {
      game->next_wave_cooldown_timer += game_update_secs;
    }
  }
  else
  {
    if (game->enemies_to_spawn < game->max_enemies_to_spawn)
    {
      if (game->skull_enemy_spawn_timer_sec > game->spawn_cooldown)
      {
        ++game->enemies_to_spawn;
        v2f desired_camera_space_p = {0};
        
        f32 camera_width_half = (f32)renderer->reso_width * 0.5f;
        f32 camera_height_half = (f32)renderer->reso_height * 0.5f;
        f32 offset_amount = 50.0f;
        
        u32 spawn_area = prng32_rangeu32(&game->prng, 0, 8);
        switch (spawn_area)
        {
          case 0:
          {
            desired_camera_space_p.x = -camera_width_half - offset_amount;
            desired_camera_space_p.y = camera_height_half + offset_amount;
          } break;
          
          case 1:
          {
            desired_camera_space_p.x = 0.0f;
            desired_camera_space_p.y = camera_height_half + offset_amount;
          } break;
          
          case 2:
          {
            desired_camera_space_p.x = camera_width_half + offset_amount;
            desired_camera_space_p.y = camera_height_half + offset_amount;
          } break;
          
          case 3:
          {
            desired_camera_space_p.x = camera_width_half + offset_amount;
            desired_camera_space_p.y = 0.0f;
          } break;
          
          case 4:
          {
            desired_camera_space_p.x = camera_width_half + offset_amount;
            desired_camera_space_p.y = -camera_height_half - offset_amount;
          } break;
          
          case 5:
          {
            desired_camera_space_p.x = 0.0f;
            desired_camera_space_p.y = -camera_height_half - offset_amount;
          } break;
          
          case 6:
          {
            desired_camera_space_p.x = -camera_width_half - offset_amount;
            desired_camera_space_p.y = -camera_height_half - offset_amount;
          } break;
          
          case 7:
          {
            desired_camera_space_p.x = -camera_width_half - offset_amount;
            desired_camera_space_p.y = 0.0f;
          } break;
          
          InvalidDefaultCase();
        }
        
        v3f world_space_p =
        {
          desired_camera_space_p.x + player->p.x,
          desired_camera_space_p.y + player->p.y,
          0.0f
        };
        
        // test entity
        make_enemy_green_skull(game, world_space_p);
        game->skull_enemy_spawn_timer_sec = 0.0f;
      }
      else
      {
        game->skull_enemy_spawn_timer_sec += game_update_secs;
      }
    }
    else
    {
      game->next_wave_cooldown_timer = 0.0f;
      game->max_enemies_to_spawn += 1;
      game->enemies_to_spawn = 0;
      game->wave_number += 1;
      if (game->spawn_cooldown >= 0.75f)
      {
        game->spawn_cooldown -= 0.01f;
      }
    }
  }
  
  // hehehehehhehe... my mind just randomly told me to try this...
  // dont mind me.
  ForLoopU64(entity_idx, game->entity_count)
  {
    Entity *entity = game->entities + entity_idx;
    switch (entity->type)
    {
      case EntityType_Player:
      {
        //
        // NOTE(cj): Movement
        //
        f32 move_comp = 64.0f;
        f32 desired_move_x = 0.0f;
        f32 desired_move_y = 0.0f;
        if (OS_KeyHeld(input, OS_Input_KeyType_W))
        {
          desired_move_y += game_update_secs * move_comp;
        }
        
        if (OS_KeyHeld(input, OS_Input_KeyType_A))
        {
          desired_move_x -= game_update_secs * move_comp;
          entity->last_face_dir = 0;
        }
        
        if (OS_KeyHeld(input, OS_Input_KeyType_S))
        {
          desired_move_y -= game_update_secs * move_comp;
        }
        
        if (OS_KeyHeld(input, OS_Input_KeyType_D))
        {
          desired_move_x += game_update_secs * move_comp;
          entity->last_face_dir = 1;
        }
        
        if (desired_move_x && desired_move_y)
        {
          desired_move_x *= 0.70710678118f;
          desired_move_y *= 0.70710678118f;
        }
        
        entity->p.x += desired_move_x;
        entity->p.y += desired_move_y;
        
        
        //
        // NOTE(cj): Render HP 
        //
        draw_health_bar(&renderer->filled_quads, entity);
        
        //
        // NOTE(cj): Drawing/Animation update of player
        //
        if (desired_move_x || desired_move_y)
        {
          Animation_Frame walk_frame = tick_animation(&entity->player.walk_animation,
                                                      get_animation_frames(AnimationFrames_PlayerWalk),
                                                      game_update_secs).frame;
          game_add_tex_clipped(&renderer->filled_quads,
                               entity->p, entity->dims,
                               walk_frame.clip_p, walk_frame.clip_dims,
                               (v4f){1,1,1,1},
                               entity->last_face_dir);
        }
        else
        {
          game_add_tex_clipped(&renderer->filled_quads,
                               entity->p, entity->dims,
                               (v2f){0,0}, (v2f){16,16},
                               (v4f){1,1,1,1},
                               entity->last_face_dir);
        }
        
        //
        // TODO(cj): Should Attacks be generated entities?
        //
        
        //
        // NOTE(cj): Update Attacks
        //
        for (u64 attack_idx = 0;
             attack_idx < entity->player.attack_count;
             ++attack_idx)
        {
          Attack *attack = entity->player.attacks + attack_idx;
          if (attack->current_secs >= attack->interval_secs)
          {
            Animation_Tick_Result tick_result = tick_animation(&attack->animation,
                                                               get_animation_frames(AnimationFrames_ShadowSlash),
                                                               game_update_secs);
            Animation_Frame frame = tick_result.frame;
            
            f32 offset_x = frame.offset.x*3;
            f32 offset_y = -frame.offset.y*3;
            if (!entity->last_face_dir)
            {
              offset_x *= -1.0f;
              offset_x -= 16.0f;
            }
            else
            {
              offset_x += 16.0f;
            }
            
            v3f p = v3f_add(entity->p, (v3f) { offset_x, offset_y, 0 });
            v3f dims = (v3f){frame.clip_dims.x*3,frame.clip_dims.y*3,0};
            v2f half_dims = { dims.x*0.5f, dims.y*0.5f };
            // 
            // TODO(cj): HARDCODE: we need to remove this hardcoded value soon1
            //
            b32 just_switched_to_third_frame = (attack->animation.frame_idx == 3) && tick_result.just_switched;
            if (just_switched_to_third_frame)
            {
              //
              // NOTE(cj): Find hostile enemies to damage
              //
              for (u64 entity_to_collide_idx = 1;
                   entity_to_collide_idx < game->entity_count;
                   ++entity_to_collide_idx)
              {
                Entity *possible_collision = game->entities + entity_to_collide_idx;
                if (!!(possible_collision->flags & (EntityFlag_Hostile|EntityFlag_DeleteMe)))
                {
                  b32 attack_collided_with_entity = check_aabb_collision_xy(p.xy, half_dims,
                                                                            possible_collision->p.xy,
                                                                            (v2f)
                                                                            {
                                                                              possible_collision->dims.x*0.5f,
                                                                              possible_collision->dims.y*0.5f,
                                                                            });
                  
                  if (attack_collided_with_entity)
                  {
                    possible_collision->current_hp -= attack->damage;
                    if (possible_collision->current_hp <= 0.0f)
                    {
                      possible_collision->flags |= EntityFlag_DeleteMe;
                    }
                  }
                }
              }
            }
            
            game_add_tex_clipped(&renderer->filled_quads, p, dims,
                                 frame.clip_p, frame.clip_dims,
                                 (v4f){1,1,1,1},
                                 entity->last_face_dir);
            
            if (tick_result.is_full_cycle)
            {
              attack->current_secs = 0.0f;
            }
          }
          else
          {
            attack->current_secs += game_update_secs;
          }
        }
        
      } break;
      
      case EntityType_GreenSkull:
      {
        b32 delete_me = !!(entity->flags & EntityFlag_DeleteMe);
        if (!delete_me)
        {
          //
          // NOTE(cj): Update movement
          //
          f32 follow_speed = 32.0f;
          v3f to_player = v3f_sub_and_normalize_or_zero(player->p, entity->p);
          to_player.x *= follow_speed * game_update_secs;
          to_player.y *= follow_speed * game_update_secs;
          v3f_add_eq(&entity->p, to_player);
        }
        
        //
        // NOTE(cj): Render HP 
        //
        draw_health_bar(&renderer->filled_quads, entity);
        
        //
        // NOTE(cj): Render the green skull enemy
        //
        Animation_Tick_Result skull_tick_result = tick_animation(&entity->enemy.animation,
                                                                 get_animation_frames(AnimationFrames_GreenSkullWalk),
                                                                 game_update_secs);
        Animation_Frame skull_frame = skull_tick_result.frame;
        game_add_tex_clipped(&renderer->filled_quads, entity->p, entity->dims,
                             skull_frame.clip_p, skull_frame.clip_dims,
                             (v4f){1,1,1,1},
                             entity->last_face_dir);
        
        //
        // NOTE(cj): Damage the player
        // 
        b32 the_attack_already_started = (entity->enemy.attack.animation.frame_idx != 0) || (entity->enemy.attack.animation.current_secs > 0.0f);
        b32 i_collided_with_player = check_aabb_collision_xy(entity->p.xy, (v2f){ entity->dims.x*0.5f, entity->dims.y*0.5f },
                                                             player->p.xy,
                                                             (v2f)
                                                             {
                                                               player->dims.x*0.5f,
                                                               player->dims.y*0.5f,
                                                             });
        
        //
        // NOTE(cj): !the_attack_already_started = (entity->enemy.attack.animation.frame_idx == 0) && (entity->enemy.attack.animation.current_secs <= 0.0f)
        // My goal of this condition is to only delete the enemy if it is marked as DeleteMe and (most importantly) the attack hasn't started yet.
        // Because If the attack as started but we have deleted the enemy, the attack animation will not complete. We want a complete attack cycle 
        // before deleting the enemy.
        //
        if (!the_attack_already_started && delete_me)
        {
          if (entity_idx != (game->entity_count - 1))
          {
            game->entities[entity_idx--] = game->entities[game->entity_count - 1];
          }
          --game->entity_count;
        }
        else if (the_attack_already_started || i_collided_with_player)
        {
          Attack *attack = &entity->enemy.attack;
          if (attack->current_secs >= attack->interval_secs)
          {
            //queue_attack(game, *attack, player->p, 1, 0);
            Animation_Tick_Result tick_result = tick_animation(&attack->animation,
                                                               get_animation_frames(AnimationFrames_Bite),
                                                               game_update_secs);
            Animation_Frame frame = tick_result.frame;
            v3f dims = (v3f){frame.clip_dims.x*3,frame.clip_dims.y*3,0};
            
            // 
            // TODO(cj): HARDCODE: we need to remove this hardcoded value soon1
            //
            b32 just_switched_to_third_frame = (attack->animation.frame_idx == 3) && tick_result.just_switched;
            if (just_switched_to_third_frame)
            {
              //
              // NOTE(cj): Attack Player
              //
              player->current_hp -= attack->damage;
              if (player->current_hp <= 0.0f)
              {
                // TODO(cj): Game Over
                HeyDeveloperPleaseImplementMeSoon();
              }
            }
            
            //
            // NOTE(cj): Draw the bite animation ON player
            // (the player must be drawn first...!)
            //
            game_add_tex_clipped(&renderer->filled_quads, player->p, dims,
                                 frame.clip_p, frame.clip_dims,
                                 (v4f){1,1,1,1},
                                 0);
            
            if (tick_result.is_full_cycle)
            {
              attack->current_secs = 0.0f;
            }
          }
          else
          {
            attack->current_secs += game_update_secs;
          }
        }
      } break;
      
      InvalidDefaultCase();
    }
  }
  
  //
  // TODO(cj): "UI" with quotes. Hardcode for now. The following is the idea of the usage code of our future
  // UI Library. It is immediate.
  //
  
  v2f right_container_dims = v2f_make(180.0f, 720.0f);
  v2f left_container_dims = ui_query_string_dimsf(&renderer->font, str8("Enemies Alive: "));
  left_container_dims.y = 720.0f;
  
  f32 main_container_x = 0.0f;
  f32 main_container_y = 0.0f;
  v4f main_container_c = rgba(37,33,49,1);
  f32 padding_x = 14.0f;
  f32 padding_y = 14.0f;
  f32 gap = 8.0f;
  v2f main_container_dims = v2f_make(left_container_dims.x + right_container_dims.x + gap*2 + padding_x, 720);
  
  //ui_gap_x_next(game->ui_ctx, 8.0f);
  //ui_padding_x_next(game->ui_ctx, 14.0f);
  //ui_padding_y_next(game->ui_ctx, 14.0f);
  // ui_push_hcontainer(game->ui_ctx, str8("sidebar"));
  {
    //ui_gap_y_next(game->ui_ctx, 4.0f);
    //ui_bg_transparent_next(game->ui_ctx);
    //ui_push_vcontainer(game->ui_ctx, str8("left-side"));
    {
      //ui_push_labelf(game->ui_ctx, str8("Wave:"));
      //ui_push_labelf(game->ui_ctx, str8("Enemies Alive:"));
      //ui_push_labelf(game->ui_ctx, str8("Health:"));
      //ui_push_labelf(game->ui_ctx, str8("Experience:"));
    }
    
    //ui_gap_y_next(game->ui_ctx, 4.0f);
    //ui_bg_transparent_next(game->ui_ctx);
    //ui_push_vcontainer(game->ui_ctx, str8("right-side"));
    {
      //ui_push_labelf(game->ui_ctx, str8("%u"), game->wave_number);
      //ui_push_labelf(game->ui_ctx, str8("%u"), game->entity_count - 1);
      //ui_push_progress_bar_with_stringf(game->ui_ctx, player->current_hp, player->max_hp, str8("##player_hp##%u / %u"), (u32)player->current_hp, (u32)player->max_hp);
      //ui_push_progress_bar_with_stringf(game->ui_ctx, player->player.current_experience, player->player.max_experience, str8("##player_exp##%u / %u"), (u32)player->player.current_experience, (u32)player->player.max_experience);
    }
  }
  
  //
  // TODO(cj): once we have implemented the UI, remove this.
  //
  ui_add_quad(&renderer->ui_quads, v2f_make(main_container_x, main_container_y), main_container_dims, 1.0f, 8.0f, main_container_c, 2.0f, v4f_make(0.4f,0.2f,0.6f,1.0f));
  {
    ui_add_stringf(&renderer->ui_quads, &renderer->font, (v2f){padding_x,padding_y}, (v4f){1,1,1,1}, str8("Wave: %u"), game->wave_number);
    ui_add_stringf(&renderer->ui_quads, &renderer->font, (v2f){padding_x, 24+padding_y + gap}, (v4f){1,1,1,1}, str8("Enemies Alive: "));
    ui_add_stringf(&renderer->ui_quads, &renderer->font, (v2f){padding_x + left_container_dims.x + gap, 24+padding_y + gap}, (v4f){1,1,1,1}, str8("%u"), game->entity_count - 1);
    
    {
      v2f helf_label_dims = ui_add_stringf(&renderer->ui_quads, &renderer->font, (v2f){padding_x,48+padding_y + 2*gap}, (v4f){1,1,1,1}, str8("Health: "));
      f32 health_bar_border = 2.0f;
      f32 health_fill_percent = player->current_hp / player->max_hp;
      f32 health_bar_width = 180;
      v4f health_bar_c = rgba(224,120,86,1.0f);
      ui_add_quad(&renderer->ui_quads, v2f_make(padding_x + left_container_dims.x + gap, 48+padding_y + 2*gap), v2f_make(health_bar_width*health_fill_percent, helf_label_dims.y),
                  1.0f, 1.0f, health_bar_c, 0.0f, rgba(113,29,56,1));
      ui_add_quad_border(&renderer->ui_quads, v2f_make(padding_x + left_container_dims.x + gap, 48+padding_y + gap*2), v2f_make(health_bar_width, helf_label_dims.y), 1.0f, 1.0f, health_bar_border, rgba(113,29,56,1));
      ui_add_stringf(&renderer->ui_quads, &renderer->font, (v2f){padding_x + left_container_dims.x + gap*2, 48+padding_y + 2*gap}, (v4f){1,1,1,1}, str8("%u / %u"), (u32)player->current_hp, (u32)player->max_hp);
    }
    
    {
      v2f label_dims = ui_add_stringf(&renderer->ui_quads, &renderer->font, (v2f){padding_x,72+padding_y + gap*3}, (v4f){1,1,1,1}, str8("Experience: "));
      f32 exp_bar_border = 2.0f;
      f32 exp_fill_percent = (f32)player->player.current_experience / (f32)player->player.max_experience;
      f32 exp_bar_width = 180;
      v4f exp_bar_c = rgba(85,108,224,1.0f);
      ui_add_quad_per_vertex_colours(&renderer->ui_quads, v2f_make(padding_x + left_container_dims.x + gap, 72.0f+padding_y + gap*3), v2f_make(exp_bar_width*exp_fill_percent, label_dims.y), 1.0f, 1.0f, exp_bar_c,exp_bar_c,exp_bar_c,exp_bar_c, 0.0f, rgba(0,0,0,0));
      ui_add_quad_border(&renderer->ui_quads, v2f_make(padding_x + left_container_dims.x + gap, 72+padding_y + gap*3), v2f_make(exp_bar_width, label_dims.y), 1.0f, 1.0f, exp_bar_border, rgba(64,29,112,1));
      ui_add_stringf(&renderer->ui_quads, &renderer->font, (v2f){padding_x + left_container_dims.x + gap*2, 72+padding_y + 3*gap}, (v4f){1,1,1,1}, str8("%u / %u"), (u32)player->player.current_experience, (u32)player->player.max_experience);
    }
  }
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmd, int nShowCmd)
{
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmd;
  (void)nShowCmd;
  
  w32_prevent_dpi_scaling();
  
  timeBeginPeriod(1);
  
  LARGE_INTEGER w32_perf_frequency;
  QueryPerformanceFrequency(&w32_perf_frequency);
  
  DEVMODE dev_mode;
  EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dev_mode);
  u64 refresh_rate = dev_mode.dmDisplayFrequency;
  f32 seconds_per_frame = 1.0f / (f32)refresh_rate;
  
  OS_Window window;
  w32_create_window(&window, "Game", 1280, 720);
  
  Game_Memory memory = {0};
  memory.arena = m_arena_reserve(MB(2));
  R_State renderer;
  r_init(&renderer, window, memory.arena);
  memory.renderer = &renderer.input_for_rendering;
  
  Game_State game = {0};
  game_init(&game);
  
  LARGE_INTEGER perf_counter_begin;
  while (1)
  {
    QueryPerformanceCounter(&perf_counter_begin);
    w32_fill_input(&window);
    OS_Input *input = &window.input;
    if (OS_KeyReleased(input, OS_Input_KeyType_Escape))
    {
      ExitProcess(0);
    }
    
    game_update_and_render(&game, input, &memory, seconds_per_frame);
    
#if defined(DR_DEBUG)
    if (OS_KeyReleased(input, OS_Input_KeyType_P))
    {
      game.dbg_draw_entity_wires = !game.dbg_draw_entity_wires;
    }
    if (game.dbg_draw_entity_wires)
    {
      for (u64 entity_idx = 0;
           entity_idx < game.entity_count;
           ++entity_idx)
      {
        Entity *entity = game.entities + entity_idx;
        game_add_rect(&renderer.input_for_rendering.wire_quads, entity->p, entity->dims, (v4f){ 0, 0, 1, 1 });
      }
    }
#endif
    
    r_submit_and_reset(&renderer, game.entities->p);
    
    LARGE_INTEGER perf_counter_end;
    QueryPerformanceCounter(&perf_counter_end);
    
    f32 seconds_used_for_work = (f32)(perf_counter_end.QuadPart - perf_counter_begin.QuadPart) / (f32)(w32_perf_frequency.QuadPart);
    
    if (seconds_used_for_work < seconds_per_frame)
    {
      Sleep((u32)((seconds_per_frame - seconds_used_for_work) * 1000.0f));
    }
  }
  
  ExitProcess(0);
}