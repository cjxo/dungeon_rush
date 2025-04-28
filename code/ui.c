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
                               f32 border_thickness)
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
  
  result->border_thickness = border_thickness;
  
  result->tex_id = 0;
  return(result);
}

function R_UI_Quad *
ui_add_quad(R_UI_QuadArray *quads, v2f p, v2f dims,
            f32 smoothness, f32 vertex_roundness, v4f vertex_colours,
            f32 border_thickness)
{
  return ui_add_quad_per_vertex_colours(quads, p, dims, smoothness, vertex_roundness,
                                        vertex_colours, vertex_colours, vertex_colours, 
                                        vertex_colours, border_thickness);
}

function R_UI_Quad *
ui_add_quad_shadowed(R_UI_QuadArray *quads, v2f p, v2f dims,
                     f32 smoothness,
                     f32 vertex_roundness, v4f colour_per_vertex,
                     f32 border_thickness,
                     v2f shadow_offset, v2f shadow_dims_offset,
                     v4f shadow_colour_per_vertex,
                     f32 shadow_smoothness)
{
  R_UI_Quad *result = ui_add_quad_per_vertex_colours(quads, p, dims, smoothness, vertex_roundness, colour_per_vertex, colour_per_vertex,
                                                     colour_per_vertex, colour_per_vertex,
                                                     border_thickness);
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
  R_UI_Quad *result = ui_add_quad_per_vertex_colours(quads, p, dims, 0.0f, 0.0f, colour, colour, colour, colour, 0.0f);
  result->uvs[0] = (v2f){ 0.0f, 0.0f };
  result->uvs[1] = (v2f){ 0.0f, 1.0f };
  result->uvs[2] = (v2f){ 1.0f, 0.0f };
  result->uvs[3] = (v2f){ 1.0f, 1.0f };
  result->tex_id = tex.id;
  return(result);
}

inline function R_UI_Quad *
ui_add_tex_clipped_per_vertex_colours(R_UI_QuadArray *quads, R_Texture2D tex, v2f p, v2f dims, v2f clip_p, v2f clip_dims,
                                      v4f vertex_top_left_c, v4f vertex_bottom_left_c,
                                      v4f vertex_top_right_c, v4f vertex_bottom_right_c)
{
  R_UI_Quad *result = ui_add_quad_per_vertex_colours(quads, p, dims, 0.0f, 0.0f, vertex_top_left_c, vertex_bottom_left_c, vertex_top_right_c, vertex_bottom_right_c, 0.0f);
  
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

inline function R_UI_Quad *
ui_add_tex_clipped(R_UI_QuadArray *quads, R_Texture2D tex, v2f p, v2f dims, v2f clip_p, v2f clip_dims, v4f colour)
{
  R_UI_Quad *result = ui_add_tex_clipped_per_vertex_colours(quads, tex, p, dims, clip_p, clip_dims,
                                                            colour, colour, colour, colour);
  return(result);
}

function v2f
ui_query_string_dims(R_Font font, String_U8_Const str)
{
  v2f final_dims = {0};
  ForLoopU64(char_idx, str.count)
  {
    u8 char_val = str.s[char_idx];
    Assert((char_val >= 32) && (char_val < 128));
    
    R_GlyphData glyph = font.glyphs[char_val];
    if (char_val != ' ')
    {
      if (glyph.clip_height > final_dims.y)
      {
        final_dims.y = glyph.clip_height;
      }
    }
    final_dims.x += glyph.advance;
  }
  return(final_dims);
}

function v2f
ui_query_string_dimsf(R_Font font, String_U8_Const str, ...)
{
  M_Arena *temp_arena = get_transient_arena(0, 0);
  Temporary_Memory temp = begin_temporary_memory(temp_arena);
  
  va_list args;
  va_start(args, str);
  String_U8_Const format = str8_format_va(temp_arena, str, args);
  va_end(args);
  
  v2f final_dims = {0};
  ForLoopU64(char_idx, format.count)
  {
    u8 char_val = format.s[char_idx];
    Assert((char_val >= 32) && (char_val < 128));
    
    R_GlyphData glyph = font.glyphs[char_val];
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
  ForLoopU64(char_idx, format.count)
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

function v2f
ui_add_string(R_UI_QuadArray *quads, R_Font font, v2f p, v4f colour, String_U8_Const str)
{
  v2f final_dims = {0};
  v2f pen_p = p;
  ForLoopU64(char_idx, str.count)
  {
    u8 char_val = str.s[char_idx];
    Assert((char_val >= 32) && (char_val < 128));
    
    R_GlyphData glyph = font.glyphs[char_val];
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
      ui_add_tex_clipped(quads, font.sheet, glyph_p,
                         glyph_dims, glyph_clip_p,
                         glyph_dims, colour);
    }
    
    pen_p.x += glyph.advance;
  }
  
  return(final_dims);
}

inline function UI_TextureClipped
ui_texture(v2f clip_p, v2f clip_dims)
{
  UI_TextureClipped result;
  result.clip_p = clip_p;
  result.clip_dims = clip_dims;
  return(result);
}

inline function UI_Key
ui_zero_key(void)
{
  UI_Key result;
  result.key = 0;
  return(result);
}

inline function UI_Widget_IndividualSize
ui_null_size(void)
{
  UI_Widget_IndividualSize result;
  result.type = UI_Widget_IndividualSizing_Null;
  result.value = 0.0f;
  return(result);
}

inline function UI_Widget_IndividualSize
ui_pixel_size(f32 value)
{
  UI_Widget_IndividualSize result;
  result.type = UI_Widget_IndividualSizing_Pixels;
  result.value = value;
  return(result);
}

inline function UI_Widget_IndividualSize
ui_children_sum_size(f32 initial_size)
{
  UI_Widget_IndividualSize result;
  result.type = UI_Widget_IndividualSizing_ChildrenSum;
  result.value = initial_size;
  return(result);
}

inline function f32
ensure_normalized(f32 x)
{
  f32 result;
  if (x > 1.0f)
  {
    result = 1.0f;
  }
  else if (x < 0.0f)
  {
    result = 0.0f;
  }
  else
  {
    result = x;
  }
  
  return(result);
}

inline function UI_Widget_IndividualSize
ui_percent_of_parent_size(f32 percent)
{
  UI_Widget_IndividualSize result;
  result.type = UI_Widget_IndividualSizing_PercentOfParent;
  result.value = ensure_normalized(percent);
  
  return(result);
}

inline function UI_Absolute_Position
ui_absolute_pixels(f32 v)
{
  UI_Absolute_Position result;
  result.metric = UI_Metric_Pixels;
  result.value = v;
  return(result);
}

inline function UI_Absolute_Position
ui_absolute_percent(f32 v)
{
  UI_Absolute_Position result;
  result.metric = UI_Metric_Percent;
  result.value = ensure_normalized(v);
  return(result);
}

function UI_Context *
ui_create_context(OS_Input *input, R_UI_QuadArray *quads, R_Font font, R_Texture2D sprite_sheet)
{
  M_Arena *arena = m_arena_reserve(MB(2));
  UI_Context *result = M_Arena_PushStruct(arena, UI_Context);
  result->arena = arena;
  result->front_util_arena = m_arena_reserve(KB(64));
  result->back_util_arena = m_arena_reserve(KB(64));
  
  result->root = 0;
  result->free_widgets = 0;
  result->current_build_index = 0;
  result->input = input;
  result->quads = quads;
  result->font = font;
  result->sprite_sheet = sprite_sheet;
  result->dt_step_secs = 0;
  result->hot = ui_zero_key();
  result->active = ui_zero_key();
  return(result);
}

function UI_Key
ui_calculate_key_from_identifier(String_U8_Const identifier)
{
  u64 key = str8_calculate_hash(identifier, 0);
  UI_Key result;
  result.key = key;
  return(result);
}

function String_U8_Const
ui_extract_content_from_identifier(String_U8_Const identifier)
{
  u64 triple_pound_idx = str8_find_first_string(identifier, str8("###"), 0);
  if (triple_pound_idx != InvalidIndexU64)
  {
    identifier.s += triple_pound_idx + 3;
    identifier.count -= triple_pound_idx + 3;
  }
  
  return(identifier);
}

function UI_Widget *
ui_push_widget(UI_Context *ctx, String_U8_Const identifier, UI_Widget_Flag flags)
{
  UI_Key key = ui_calculate_key_from_identifier(identifier);
  u64 cache_idx = key.key & (UI_CacheSize - 1);
  
  UI_Widget *result = ctx->cache[cache_idx];
  while (result && !str8_equal_strings(result->str8_identifier, identifier))
  {
    result = result->next_in_hash;
  }
  
  if (!result)
  {
    if (ctx->free_widgets)
    {
      result = ctx->free_widgets;
      ctx->free_widgets = ctx->free_widgets->next_in_free;
    }
    else
    {
      result = M_Arena_PushStruct(ctx->arena, UI_Widget);
    }
    
    result->build_last_touch_index = ctx->current_build_index - 1;
    result->hot_t = result->active_t = 0.0f;
    SLLPushFrontN(ctx->cache[cache_idx], result, next_in_hash);
  }
  
  if (result->build_last_touch_index != ctx->current_build_index)
  {
    result->parent = 0;
    result->next_sibling = 0;
    result->prev_sibling = 0;
    result->rightmost_child = 0;
    result->leftmost_child = 0;
    
    // NOTE(cj): else, in the cache, meaning, cached stuff that is useful for 
    // information such as user interaction, animation, etc.
    result->build_last_touch_index = ctx->current_build_index;
    result->key = key;
    result->flags = flags;
    result->str8_identifier = str8_copy(ctx->back_util_arena, identifier);
    result->str8_content = ui_extract_content_from_identifier(result->str8_identifier);
    result->rel_parent_p = v2f_make(0, 0);
    
    // TODO(cj): Should we instead let the me specify the dimensions of this
    // widget with respect to the string? I mean look at this code.... 
    if (flags & UI_Widget_Flag_StringContent)
    {
      UI_Widget_IndividualSize size_x = ui_size_x_peek_or_auto_pop(ctx);
      UI_Widget_IndividualSize size_y = ui_size_y_peek_or_auto_pop(ctx);
      
      v2f text_dims = ui_query_string_dims(ctx->font, result->str8_content);
      result->individual_size[UI_Axis_X].type = UI_Widget_IndividualSizing_Pixels;
      result->individual_size[UI_Axis_X].value = text_dims.x;
      result->individual_size[UI_Axis_Y].type = UI_Widget_IndividualSizing_Pixels;
      result->individual_size[UI_Axis_Y].value = text_dims.y;
      if ((text_dims.x == 0) || (text_dims.y == 0))
      {
        result->individual_size[UI_Axis_Y].value = ctx->font.ascent + ctx->font.descent;
      }
      
      if (size_x.type != UI_Widget_IndividualSizing_Null)
      {
        result->individual_size[UI_Axis_X] = size_x;
      }
      if (size_y.type != UI_Widget_IndividualSizing_Null)
      {
        result->individual_size[UI_Axis_Y] = size_y;
      }
      result->text_dims = text_dims;
    }
    else
    {
      result->individual_size[UI_Axis_X] = ui_size_x_peek_or_auto_pop(ctx);
      result->individual_size[UI_Axis_Y] = ui_size_y_peek_or_auto_pop(ctx);
    }
    
    if (ctx->parent_ptr > 0)
    {
      UI_Widget *top_parent = ui_parent_peek_or_auto_pop(ctx);
      result->parent = top_parent;
      if (top_parent->leftmost_child == 0)
      {
        top_parent->leftmost_child = top_parent->rightmost_child = result;
      }
      else
      {
        Assert(top_parent->rightmost_child != 0);
        top_parent->rightmost_child->next_sibling = result;
        result->prev_sibling = top_parent->rightmost_child;
        top_parent->rightmost_child = result;
      }
    }
    else
    {
      result->parent = 0;
    }
  }
  
  result->padding[UI_Axis_X] = ui_padding_x_peek_or_auto_pop(ctx);
  result->padding[UI_Axis_Y] = ui_padding_y_peek_or_auto_pop(ctx);
  result->gap[UI_Axis_X] = ui_gap_x_peek_or_auto_pop(ctx);
  result->gap[UI_Axis_Y] = ui_gap_y_peek_or_auto_pop(ctx);
  
  result->position = ui_position_peek_or_auto_pop(ctx);
  result->absolute_p[UI_Axis_X] = ui_absolute_x_peek_or_auto_pop(ctx);
  result->absolute_p[UI_Axis_Y] = ui_absolute_y_peek_or_auto_pop(ctx);
  
  result->texture = ui_texture_peek_or_auto_pop(ctx);
  
  result->vertex_roundness = ui_vertex_roundness_peek_or_auto_pop(ctx);
  result->tl_bg_colour = ui_tl_bg_colour_peek_or_auto_pop(ctx);
  result->bl_bg_colour = ui_bl_bg_colour_peek_or_auto_pop(ctx);
  result->tr_bg_colour = ui_tr_bg_colour_peek_or_auto_pop(ctx);
  result->br_bg_colour = ui_br_bg_colour_peek_or_auto_pop(ctx);
  
  result->border_thickness = ui_border_thickness_peek_or_auto_pop(ctx);
  result->tl_border_colour = ui_tl_border_colour_peek_or_auto_pop(ctx);
  result->bl_border_colour = ui_bl_border_colour_peek_or_auto_pop(ctx);
  result->tr_border_colour = ui_tr_border_colour_peek_or_auto_pop(ctx);
  result->br_border_colour = ui_br_border_colour_peek_or_auto_pop(ctx);
  
  result->smoothness = ui_smoothness_peek_or_auto_pop(ctx);
  
  result->text_colour = ui_text_colour_peek_or_auto_pop(ctx);
  result->text_centering[UI_Axis_X] = ui_text_centering_x_peek_or_auto_pop(ctx);
  result->text_centering[UI_Axis_Y] = ui_text_centering_y_peek_or_auto_pop(ctx);
  
  Assert(result);
  return(result);
}

function void
ui_begin(UI_Context *ctx, u32 reso_width, u32 reso_height, f32 dt_step_secs)
{
  f32 max_width = (f32)reso_width;
  f32 max_height = (f32)reso_height;
  ctx->max_width = max_width;
  ctx->max_height = max_height;
  
  ctx->dt_step_secs = dt_step_secs;
  
  //
  // NOTE(cj): Evict stuff in cache that are not used in the last frame 
  //
  for (u64 idx = 0; idx < UI_CacheSize; ++idx)
  {
    UI_Widget **start = ctx->cache + idx;
    while (*start)
    {
      if ((*start)->build_last_touch_index != ctx->current_build_index)
      {
        UI_Widget *temp = *start;
        (*start) = (*start)->next_in_hash;
        temp->next_in_free = ctx->free_widgets;
        ctx->free_widgets = temp;
      }
      else
      {
        start = &((*start)->next_in_hash);
      }
    }
  }
  
  // NOTE(cj): Increment the current frame
  ++ctx->current_build_index;
  
  //
  // NOTE(cj): reset stacks to defaults
  //
  ctx->parent_ptr = 0;
  ctx->size_x_ptr = 0;
  ctx->size_y_ptr = 0;
  ctx->position_ptr = 0;
  ctx->absolute_x_ptr = 0;
  ctx->absolute_y_ptr = 0;
  ctx->padding_x_ptr = 0;
  ctx->padding_y_ptr = 0;
  ctx->texture_ptr = 0;
  ctx->gap_x_ptr = 0;
  ctx->gap_y_ptr = 0;
  ctx->vertex_roundness_ptr = 0;
  ctx->tl_bg_colour_ptr = 0;
  ctx->bl_bg_colour_ptr = 0;
  ctx->tr_bg_colour_ptr = 0;
  ctx->br_bg_colour_ptr = 0;
  ctx->border_thickness_ptr = 0;
  ctx->tl_border_colour_ptr = 0;
  ctx->bl_border_colour_ptr = 0;
  ctx->tr_border_colour_ptr = 0;
  ctx->br_border_colour_ptr = 0;
  ctx->smoothness_ptr = 0;
  ctx->text_colour_ptr = 0;
  ctx->text_centering_x_ptr = 0;
  ctx->text_centering_y_ptr = 0;
  
  ui_size_x_push(ctx, ui_null_size());
  ui_size_y_push(ctx, ui_null_size());
  ui_position_push(ctx, UI_Widget_Position_Normal);
  ui_absolute_x_push(ctx, ui_absolute_pixels(0.0f));
  ui_absolute_y_push(ctx, ui_absolute_pixels(0.0f));
  
  ui_texture_push(ctx, (UI_TextureClipped){});
  
  ui_padding_x_push(ctx, 0.0f);
  ui_padding_y_push(ctx, 0.0f);
  ui_gap_x_push(ctx, 0.0f);
  ui_gap_y_push(ctx, 0.0f);
  ui_vertex_roundness_push(ctx, 0.0f);
  ui_tl_bg_colour_push(ctx, rgba(0, 0, 0, 0));
  ui_bl_bg_colour_push(ctx, rgba(0, 0, 0, 0));
  ui_tr_bg_colour_push(ctx, rgba(0, 0, 0, 0));
  ui_br_bg_colour_push(ctx, rgba(0, 0, 0, 0));
  
  ui_border_thickness_push(ctx, 0.0f);
  ui_tl_border_colour_push(ctx, rgba(0, 0, 0, 0));
  ui_bl_border_colour_push(ctx, rgba(0, 0, 0, 0));
  ui_tr_border_colour_push(ctx, rgba(0, 0, 0, 0));
  ui_br_border_colour_push(ctx, rgba(0, 0, 0, 0));
  
  ui_smoothness_push(ctx, 0.0f);
  
  ui_text_colour_push(ctx, v4f_make(1, 1, 1, 1));
  
  ui_text_centering_x_push(ctx, UI_Widget_TextCentering_Begin);
  ui_text_centering_y_push(ctx, UI_Widget_TextCentering_Begin);
  
  ui_size_x_next(ctx, ui_pixel_size(max_width));
  ui_size_y_next(ctx, ui_children_sum_size(max_height));
  UI_Widget *root = ui_push_widget(ctx, str8("__main_container__"), 0);
  ui_parent_push(ctx, root);
  ctx->root = root;
}

function UI_Widget *
ui_push_vlayout(UI_Context *ctx, f32 init_height, v2f padding, v2f gap, String_U8_Const name)
{
  //ui_size_x_next(ctx, ui_null_size());
  ui_padding_x_next(ctx, padding.x);
  ui_padding_y_next(ctx, padding.y);
  ui_gap_x_next(ctx, gap.x);
  ui_gap_y_next(ctx, gap.y);
  ui_size_y_next(ctx, ui_children_sum_size(init_height));
  UI_Widget *result = ui_push_widget(ctx, name,
                                     UI_Widget_Flag_BackgroundColour |
                                     UI_Widget_Flag_BorderColour);
  ui_parent_push(ctx, result);
  return(result);
}

function UI_Widget *
ui_push_hlayout(UI_Context *ctx, f32 init_width, v2f padding, v2f gap, String_U8_Const name)
{
  ui_padding_x_next(ctx, padding.x);
  ui_padding_y_next(ctx, padding.y);
  ui_gap_x_next(ctx, gap.x);
  ui_gap_y_next(ctx, gap.y);
  ui_size_x_next(ctx, ui_children_sum_size(init_width));
  //ui_size_y_next(ctx, ui_null_size());
  UI_Widget *result = ui_push_widget(ctx, name,
                                     UI_Widget_Flag_BackgroundColour |
                                     UI_Widget_Flag_BorderColour);
  ui_parent_push(ctx, result);
  return(result);
}

function UI_Widget *
ui_push_label(UI_Context *ctx, String_U8_Const str)
{
  UI_Widget *result = ui_push_widget(ctx, str,
                                     UI_Widget_Flag_StringContent |
                                     UI_Widget_Flag_BackgroundColour |
                                     UI_Widget_Flag_BorderColour);
  return(result);
}

function UI_Widget *
ui_push_labelf(UI_Context *ctx, String_U8_Const str, ...)
{
  M_Arena *temp_arena = get_transient_arena(0, 0);
  Temporary_Memory temp = begin_temporary_memory(temp_arena);
  
  va_list args;
  va_start(args, str);
  String_U8_Const format = str8_format_va(temp_arena, str, args);
  va_end(args);
  
  UI_Widget *result = ui_push_label(ctx, format);
  
  end_temporary_memory(temp);
  return(result);
}

function UI_Widget *
ui_push_texture(UI_Context *ctx, UI_TextureClipped texture, f32 width, f32 height, String_U8_Const str)
{
  ui_size_x_next(ctx, ui_pixel_size(width));
  ui_size_y_next(ctx, ui_pixel_size(height));
  ui_texture_next(ctx, texture);
  UI_Widget *result = ui_push_widget(ctx, str,
                                     UI_Widget_Flag_BackgroundColour |
                                     UI_Widget_Flag_Texture);
  return(result);
}

function void
ui_calculate_standalone_sizes(UI_Widget *root, UI_AxisType axis)
{
  UI_Widget_IndividualSize indie_size = root->individual_size[axis];
  f32 indie = 0.0f;
  switch (indie_size.type)
  {
    case UI_Widget_IndividualSizing_Pixels:
    case UI_Widget_IndividualSizing_ChildrenSum:
    {
      indie = indie_size.value;
    } break;
  }
  
  root->final_dims.v[axis] = indie + root->border_thickness * 2;
  
  for (UI_Widget *widget = root->leftmost_child;
       widget;
       widget = widget->next_sibling)
  {
    ui_calculate_standalone_sizes(widget, axis);
  }
}

function void
ui_calculate_downwards_dependent_sizes(UI_Widget *root, UI_AxisType axis)
{
  UI_Widget_IndividualSize indie_size = root->individual_size[axis];
  f32 accumulated_size = root->final_dims.v[axis];
  if (indie_size.type == UI_Widget_IndividualSizing_ChildrenSum)
  {
    f32 child_size = 0.0f;
    for (UI_Widget *child = root->leftmost_child;
         child;
         child = child->next_sibling)
    {
      ui_calculate_downwards_dependent_sizes(child, axis);
      if (child->position == UI_Widget_Position_Normal)
      {
        if (child->next_sibling)
        {
          child_size += child->final_dims.v[axis] + root->gap[axis] + child->border_thickness*2;
          //accumulated_size += child->final_dims.v[axis] + root->gap[axis];
        }
        else
        {
          child_size += child->final_dims.v[axis] + child->border_thickness;
          //accumulated_size += child->final_dims.v[axis];
        }
      }
    }
    
    // only increase the size of the container if the children
    // overflows it
    if (child_size > accumulated_size)
    {
      accumulated_size = child_size;
    }
  }
  else
  {
    for (UI_Widget *child = root->leftmost_child;
         child;
         child = child->next_sibling)
    {
      ui_calculate_downwards_dependent_sizes(child, axis);
      if (child->position == UI_Widget_Position_Normal)
      {
        f32 child_size = child->final_dims.v[axis] + child->border_thickness;
        if (child_size > accumulated_size)
        {
          accumulated_size = child_size;
        }
      }
    }
  }
  
  root->final_dims.v[axis] = accumulated_size + root->padding[axis] * 2.0f;
}

function void
ui_calculate_upwards_dependent_sizes(UI_Widget *root, UI_AxisType axis)
{
  UI_Widget_IndividualSize indie_size = root->individual_size[axis];
  f32 indie = 0.0f;
  switch (indie_size.type)
  {
    case UI_Widget_IndividualSizing_PercentOfParent:
    {
      Assert(root->parent);
      // TODO(cj): Figure out why the heck is the times three?
      indie = indie_size.value * (root->parent->final_dims.v[axis] - root->parent->padding[axis] * 3);
    } break;
  }
  
  root->final_dims.v[axis] += indie;
  
  for (UI_Widget *widget = root->leftmost_child;
       widget;
       widget = widget->next_sibling)
  {
    ui_calculate_upwards_dependent_sizes(widget, axis);
  }
}

function void
ui_calculate_absolute_p_if_absolute(UI_Widget *widget, UI_AxisType axis)
{
  if (widget->position == UI_Widget_Position_Absolute)
  {
    UI_Absolute_Position pos = widget->absolute_p[axis];
    switch (pos.metric)
    {
      case UI_Metric_Pixels:
      {
        widget->rel_parent_p.v[axis] += pos.value;
      } break;
      
      case UI_Metric_Percent:
      {
        Assert(widget->parent);
        f32 value = pos.value * (widget->parent->final_dims.v[axis] - widget->parent->padding[axis]*2);
        widget->rel_parent_p.v[axis] += value;
      } break;
      
      InvalidDefaultCase();
    }
  }
}

function void
ui_calculate_final_positions(UI_Widget *root, UI_AxisType axis)
{
  if (root->flags & UI_Widget_Flag_StringContent)
  {
    root->rel_text_p.v[axis] = root->padding[axis];
    switch (root->text_centering[axis])
    {
      case UI_Widget_TextCentering_Center:
      {
        f32 size = root->final_dims.v[axis] - root->padding[axis]*2;
        root->rel_text_p.v[axis] += (size - root->text_dims.v[axis])*0.5f;
      } break;
    }
    root->final_text_p.v[axis] = root->final_p.v[axis] + root->rel_text_p.v[axis];
  }
  
  UI_Widget_IndividualSize indie_size = root->individual_size[axis];
  f32 accumulated_p = 0.0f;
  if (indie_size.type == UI_Widget_IndividualSizing_ChildrenSum)
  {
    for (UI_Widget *child = root->leftmost_child;
         child;
         child = child->next_sibling)
    {
      if (child->position == UI_Widget_Position_Normal)
      {
        child->rel_parent_p.v[axis] += accumulated_p + root->border_thickness + root->padding[axis];
        accumulated_p += child->final_dims.v[axis] + root->gap[axis];
      }
      else
      {
        ui_calculate_absolute_p_if_absolute(child, axis);
        child->rel_parent_p.v[axis] += root->border_thickness + root->padding[axis];
      }
      
      child->final_p.v[axis] = root->final_p.v[axis] + child->rel_parent_p.v[axis];
      ui_calculate_final_positions(child, axis);
    }
  }
  else
  {
    for (UI_Widget *child = root->leftmost_child;
         child;
         child = child->next_sibling)
    {
      ui_calculate_absolute_p_if_absolute(child, axis);
      child->rel_parent_p.v[axis] += root->border_thickness + root->padding[axis];
      child->final_p.v[axis] = root->final_p.v[axis] + child->rel_parent_p.v[axis];
      ui_calculate_final_positions(child, axis);
    }
  }
}

function void
ui_render(UI_Context *ctx, UI_Widget *root)
{
  v2f final_dims = root->final_dims;
  if (final_dims.x >= ctx->max_width)
  {
    final_dims.x = ctx->max_width;
  }
  if (final_dims.y >= ctx->max_height)
  {
    final_dims.y = ctx->max_height;
  }
  
  if (root->flags & (UI_Widget_Flag_BackgroundColour))
  {
    ui_add_quad_per_vertex_colours(ctx->quads, root->final_p, final_dims,
                                   root->smoothness, root->vertex_roundness,
                                   root->tl_bg_colour, root->bl_bg_colour,
                                   root->tr_bg_colour, root->br_bg_colour,
                                   0.0f);
  }
  
  if ((root->flags & (UI_Widget_Flag_BorderColour)) && (root->border_thickness > 0.0f))
  {
    ui_add_quad_per_vertex_colours(ctx->quads, root->final_p, final_dims,
                                   root->smoothness, root->vertex_roundness,
                                   root->tl_border_colour, root->bl_border_colour,
                                   root->tr_border_colour, root->br_border_colour,
                                   root->border_thickness);
  }
  
  if (root->flags & UI_Widget_Flag_Texture)
  {
    UI_TextureClipped tex = root->texture;
    ui_add_tex_clipped(ctx->quads, ctx->sprite_sheet, root->final_p, final_dims,
                       tex.clip_p, tex.clip_dims, v4f_make(1,1,1,1));
  }
  
  if ((root->flags & UI_Widget_Flag_StringContent) && (root->text_colour.w != 0.0f))
  {
    ui_add_string(ctx->quads, ctx->font, root->final_text_p, root->text_colour, root->str8_content);
  }
  
  for (UI_Widget *child = root->leftmost_child;
       child;
       child = child->next_sibling)
  {
    ui_render(ctx, child);
  }
}

function void
ui_end(UI_Context *ctx)
{
  Assert(ctx->root);
  
  // NOTE(cj): layout
  ui_calculate_standalone_sizes(ctx->root, UI_Axis_X);
  ui_calculate_downwards_dependent_sizes(ctx->root, UI_Axis_X);
  ui_calculate_upwards_dependent_sizes(ctx->root, UI_Axis_X);
  ui_calculate_final_positions(ctx->root, UI_Axis_X);
  
  ui_calculate_standalone_sizes(ctx->root, UI_Axis_Y);
  ui_calculate_downwards_dependent_sizes(ctx->root, UI_Axis_Y);
  ui_calculate_upwards_dependent_sizes(ctx->root, UI_Axis_Y);
  ui_calculate_final_positions(ctx->root, UI_Axis_Y);
  
  // SOON:
  // ui_animate(ctx);
  
  // NOTE(cj): rendering
  ui_render(ctx, ctx->root);
  m_arena_clear(ctx->front_util_arena);
  Swap(M_Arena *, ctx->front_util_arena, ctx->back_util_arena);
}