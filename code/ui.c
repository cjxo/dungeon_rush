
inline function R_UI_Quad *
ui_acquire_quad(R_UI_QuadArray *quads)
{
  Assert(quads->count < quads->capacity);
  R_UI_Quad *result = quads->quads + quads->count++;
  ClearStructP(result);
  return(result);
}

function R_UI_Quad *
ui_add_quad_per_vertex_colours(R_UI_QuadArray *quads, v2f p, v2f dims,
                               f32 smoothness,
                               f32 vertex_roundness, v4f vertex_top_left_c, v4f vertex_bottom_left_c,
                               v4f vertex_top_right_c, v4f vertex_bottom_right_c,
                               f32 border_thickness, v4f border_colour)
{
  R_UI_Quad *result = ui_acquire_quad(quads);
  result->p = p;
  result->dims = dims;
  result->smoothness = smoothness;
  result->vertex_colours[0] = vertex_top_left_c;
  result->vertex_colours[1] = vertex_bottom_left_c;
  result->vertex_colours[2] = vertex_top_right_c;
  result->vertex_colours[3] = vertex_bottom_right_c;
  result->vertex_roundness = vertex_roundness;
  
  result->border_colour = border_colour;
  result->border_thickness = border_thickness;
  
  result->tex_id = 0;
  return(result);
}

function R_UI_Quad *
ui_add_quad(R_UI_QuadArray *quads, v2f p, v2f dims,
            f32 smoothness, f32 vertex_roundness, v4f vertex_colours,
            f32 border_thickness, v4f border_colour)
{
  return ui_add_quad_per_vertex_colours(quads, p, dims, smoothness, vertex_roundness,
                                        vertex_colours, vertex_colours, vertex_colours, 
                                        vertex_colours, border_thickness, border_colour);
}

function R_UI_Quad *
ui_add_quad_shadowed(R_UI_QuadArray *quads, v2f p, v2f dims,
                     f32 smoothness,
                     f32 vertex_roundness, v4f colour_per_vertex,
                     f32 border_thickness, v4f border_colour,
                     v2f shadow_offset, v2f shadow_dims_offset,
                     v4f shadow_colour_per_vertex,
                     f32 shadow_smoothness)
{
  R_UI_Quad *result = ui_add_quad_per_vertex_colours(quads, p, dims, smoothness, vertex_roundness, colour_per_vertex, colour_per_vertex,
                                                     colour_per_vertex, colour_per_vertex,
                                                     border_thickness, border_colour);
  if (shadow_dims_offset.x < 0.0f) shadow_dims_offset.x = 0.0f;
  if (shadow_dims_offset.y < 0.0f) shadow_dims_offset.y = 0.0f;
  
  result->shadow_offset = shadow_offset;
  result->shadow_dims_offset = shadow_dims_offset;
  result->shadow_colours[0] = shadow_colour_per_vertex;
  result->shadow_colours[1] = shadow_colour_per_vertex;
  result->shadow_colours[2] = shadow_colour_per_vertex;
  result->shadow_colours[3] = shadow_colour_per_vertex;
  result->shadow_smoothness = shadow_smoothness;
  return(result);
}

inline function R_UI_Quad *
ui_add_tex(R_UI_QuadArray *quads, R_Texture2D tex, v2f p, v2f dims, v4f colour)
{
  R_UI_Quad *result = ui_add_quad_per_vertex_colours(quads, p, dims, 0.0f, 0.0f, colour, colour, colour, colour, 0.0f, (v4f){0,0,0,0});
  result->uvs[0] = (v2f){ 0.0f, 0.0f };
  result->uvs[1] = (v2f){ 0.0f, 1.0f };
  result->uvs[2] = (v2f){ 1.0f, 0.0f };
  result->uvs[3] = (v2f){ 1.0f, 1.0f };
  result->tex_id = tex.id;
  return(result);
}

inline function R_UI_Quad *
ui_add_tex_clipped(R_UI_QuadArray *quads, R_Texture2D tex, v2f p, v2f dims, v2f clip_p, v2f clip_dims, v4f colour)
{
  R_UI_Quad *result = ui_add_quad_per_vertex_colours(quads, p, dims, 0.0f, 0.0f, colour, colour, colour, colour, 0.0f, (v4f){0,0,0,0});
  
  f32 x_start = clip_p.x / (f32)tex.width;
  f32 x_end = (clip_p.x + clip_dims.x) / (f32)tex.width;
  f32 y_start = clip_p.y / (f32)tex.height;
  f32 y_end = (clip_p.y + clip_dims.y) / (f32)tex.height;
  
  result->uvs[0] = (v2f){ x_start, y_start };
  result->uvs[1] = (v2f){ x_start, y_end };
  result->uvs[2] = (v2f){ x_end, y_start };
  result->uvs[3] = (v2f){ x_end, y_end };
  
  result->tex_id = tex.id;
  return(result);
}

function R_UI_Quad *
ui_add_quad_border(R_UI_QuadArray *quads, v2f p, v2f dims,
                   f32 smoothness, f32 vertex_roundness,
                   f32 border_thickness, v4f border_colour)
{
  R_UI_Quad *result = ui_acquire_quad(quads);
  result->p = p;
  result->dims = dims;
  result->smoothness = smoothness;
  result->vertex_roundness = vertex_roundness;
  
  result->border_only = 1;
  result->border_colour = border_colour;
  result->border_thickness = border_thickness;
  
  result->tex_id = 0;
  return(result);
}

function v2f
ui_query_string_dimsf(R_Font *font, String_U8_Const str, ...)
{
  M_Arena *temp_arena = get_transient_arena(0, 0);
  Temporary_Memory temp = begin_temporary_memory(temp_arena);
  
  va_list args;
  va_start(args, str);
  String_U8_Const format = str8_format_va(temp_arena, str, args);
  va_end(args);
  
  v2f final_dims = {0};
  ForLoopU64(char_idx, format.cap)
  {
    u8 char_val = format.s[char_idx];
    Assert((char_val >= 32) && (char_val < 128));
    
    R_GlyphData glyph = font->glyphs[char_val];
    if (char_val != ' ')
    {
      if (glyph.clip_height > final_dims.y)
      {
        final_dims.y = glyph.clip_height;
      }
      final_dims.x += glyph.advance;
    }
  }
  
  end_temporary_memory(temp);
  return(final_dims);
}

function v2f
ui_add_stringf(R_UI_QuadArray *quads, R_Font *font, v2f p, v4f colour, String_U8_Const str, ...)
{
  M_Arena *temp_arena = get_transient_arena(0, 0);
  Temporary_Memory temp = begin_temporary_memory(temp_arena);
  
  va_list args;
  va_start(args, str);
  String_U8_Const format = str8_format_va(temp_arena, str, args);
  va_end(args);
  
  v2f final_dims = {0};
  v2f pen_p = p;
  ForLoopU64(char_idx, format.cap)
  {
    u8 char_val = format.s[char_idx];
    Assert((char_val >= 32) && (char_val < 128));
    
    R_GlyphData glyph = font->glyphs[char_val];
    if (char_val != ' ')
    {
      if (glyph.clip_height > final_dims.y)
      {
        final_dims.y = glyph.clip_height;
      }
      
      final_dims.x += glyph.advance;
      v2f glyph_p = { pen_p.x, pen_p.y };
      v2f glyph_dims = { glyph.clip_width, glyph.clip_height, };
      v2f glyph_clip_p = { glyph.clip_x, glyph.clip_y };
      ui_add_tex_clipped(quads, font->sheet, glyph_p,
                         glyph_dims, glyph_clip_p,
                         glyph_dims, colour);
    }
    
    pen_p.x += glyph.advance;
  }
  
  end_temporary_memory(temp);
  return(final_dims);
}

function UI_Widget *
ui_push_widget(UI_Context *ctx, String_U8_Const identifier, UI_Widget_Flag flags)
{
  UI_Widget *result = 0;
  Assert(result);
  return(result);
}