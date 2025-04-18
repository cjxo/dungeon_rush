#ifndef RENDERER_H
#define RENDERER_H

#define R_Game_MaxQuads 1024
#define R_UI_MaxQuads 512

typedef struct
{
  u32 id;
  s32 width, height;
} R_Texture2D;

typedef struct
{
  v3f p;
  v3f dims;
  v4f colour;
  v2f uvs[4];
  u32 tex_id;
} R_Game_Quad;

typedef struct
{
  R_Game_Quad *quads;
  u64 capacity;
  u64 count;
  
  R_Texture2D tex;
} R_Game_QuadArray;

typedef struct
{
  v2f p;
  v2f dims;
  f32 rotation_radians;
  f32 smoothness;
  
  v4f vertex_colours[4];
  f32 vertex_roundness;
  
  v4f border_colour;
  f32 border_thickness;
  
  v2f shadow_offset;
  v2f shadow_dims_offset;
  v4f shadow_colours[4];
  f32 shadow_smoothness;
  
  v2f uvs[4];
  // 1 -> diffuse sheet
  // 2 -> small font 
  // anything else -> no texture
  u32 tex_id;
} R_UI_Quad;

typedef struct
{
  R_UI_Quad *quads;
  u64 capacity;
  u64 count;
} R_UI_QuadArray;

typedef struct
{
  f32 advance;
  f32 clip_x, clip_y;
  f32 clip_width, clip_height;
  f32 x_offset;
} R_GlyphData;

typedef struct
{
  // f32 ascent, descent;
  R_GlyphData glyphs[128];
  R_Texture2D sheet;
} R_Font;

typedef struct
{
  // TODO(cj): Should we remove QuadArrays, and pass this struct instead?
  
  // NOTE(cj): "Game Stuff" must be drawn first
  R_Game_QuadArray filled_quads;
  R_Game_QuadArray wire_quads;
  
  // NOTE(cj): "UI Stuff" must be drawn last
  R_UI_QuadArray ui_quads;
  
  s32 reso_width, reso_height;
  // TODO(cj): This should be manually created by the front end
  // and input to renderer
  R_Font font;
  R_Texture2D game_sheet;
  R_Texture2D font_sheet;
} R_InputForRendering;

typedef struct
{
  // D3D11 STUFF
  s32 reso_width, reso_height;
  ID3D11Device *device;
  ID3D11DeviceContext *device_context;
  IDXGISwapChain1 *swap_chain;
  ID3D11RenderTargetView *render_target;
  
  // the game's main rendering state
  ID3D11VertexShader *vertex_shader_main;
  ID3D11PixelShader *pixel_shader_main;
  ID3D11Buffer *cbuffer0_main;
  ID3D11Buffer *cbuffer1_main;
  ID3D11Buffer *sbuffer_main;
  ID3D11ShaderResourceView *sbuffer_view_main;
  
  // UI main rendering state
  ID3D11VertexShader *vertex_shader_ui;
  ID3D11PixelShader *pixel_shader_ui;
  ID3D11Buffer *cbuffer0_ui;
  ID3D11Buffer *sbuffer_ui;
  ID3D11ShaderResourceView *sbuffer_view_ui;
  
  ID3D11RasterizerState *rasterizer_fill_no_cull_ccw;
  ID3D11RasterizerState *rasterizer_wire_no_cull_ccw;
  
  ID3D11SamplerState *sampler_point_all;
  
  ID3D11BlendState *blend_blend;
  
  HFONT font;
  
  // NOTE(cj): TextureID: 1
  s32 game_diffse_sheet_width;
  s32 game_diffse_sheet_height;
  ID3D11ShaderResourceView *game_diffuse_sheet_view;
  
  // NOTE(cj): TextureID: 2
  s32 font_atlas_sheet_width;
  s32 font_atlas_sheet_height;
  ID3D11ShaderResourceView *font_atlas_sheet_view;
  
  R_InputForRendering input_for_rendering;
} R_State;

function void r_init(R_State *state, OS_Window window, M_Arena *arena);
function void r_submit_and_reset(R_State *state, v3f camera_p);

#endif //RENDERER_H
