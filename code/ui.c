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
ui_add_tex_clipped(R_UI_QuadArray *quads, R_Texture2D tex, v2f p, v2f dims, v2f clip_p, v2f clip_dims, v4f colour)
{
  R_UI_Quad *result = ui_add_quad_per_vertex_colours(quads, p, dims, 0.0f, 0.0f, colour, colour, colour, colour, 0.0f);
  
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
      final_dims.x += glyph.advance;
    }
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

inline function UI_Key
ui_zero_key(void)
{
  UI_Key result;
  result.key = 0;
  return(result);
}

function UI_Context *
ui_create_context(OS_Input *input, R_UI_QuadArray *quads, R_Font font)
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
ui_push_widget(UI_Context *ctx, String_U8_Const identifier, UI_Widget_Flag flags, UI_AxisType children_layout)
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
    
    MemoryClear(result->individual_size, sizeof(result->individual_size));
    if (flags & UI_Widget_Flag_StringContent)
    {
      v2f text_dims = ui_query_string_dims(ctx->font, result->str8_content);
      result->individual_size[UI_Axis_X].type = UI_Widget_IndividualSizing_Pixels;
      result->individual_size[UI_Axis_X].value = text_dims.x;
      result->individual_size[UI_Axis_Y].type = UI_Widget_IndividualSizing_Pixels;
      result->individual_size[UI_Axis_Y].value = text_dims.y;
      if ((text_dims.x == 0) || (text_dims.y == 0))
      {
        result->individual_size[UI_Axis_Y].value = ctx->font.ascent + ctx->font.descent;
      }
    }
    else if ((children_layout == UI_Axis_X) || ((children_layout == UI_Axis_Y)))
    {
      result->individual_size[UI_Axis_X].type = UI_Widget_IndividualSizing_ChildrenSum;
      result->individual_size[UI_Axis_X].value = 0.0f;
      result->individual_size[UI_Axis_Y].type = UI_Widget_IndividualSizing_ChildrenSum;
      result->individual_size[UI_Axis_Y].value = 0.0f;
      result->children_layout_axis = children_layout;
    }
    
    if (ctx->parent_ptr > 0)
    {
      UI_Widget *top_parent = ui_parent_peek(ctx);
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
  ctx->padding_x_ptr = 0;
  ctx->padding_y_ptr = 0;
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
  
  UI_Widget *root = ui_push_vlayout(ctx, str8("__main_container__"));
  ctx->root = root;
}

function UI_Widget *
ui_push_vlayout(UI_Context *ctx, String_U8_Const name)
{
  UI_Widget *result = ui_push_widget(ctx, name,
                                     UI_Widget_Flag_BackgroundColour |
                                     UI_Widget_Flag_BorderColour,
                                     UI_Axis_Y);
  ui_parent_push(ctx, result);
  return(result);
}

function UI_Widget *
ui_push_hlayout(UI_Context *ctx, String_U8_Const name)
{
  UI_Widget *result = ui_push_widget(ctx, name,
                                     UI_Widget_Flag_BackgroundColour |
                                     UI_Widget_Flag_BorderColour,
                                     UI_Axis_X);
  ui_parent_push(ctx, result);
  return(result);
}

function UI_Widget *
ui_push_label(UI_Context *ctx, String_U8_Const str)
{
  UI_Widget *result = ui_push_widget(ctx, str,
                                     UI_Widget_Flag_StringContent |
                                     UI_Widget_Flag_BackgroundColour |
                                     UI_Widget_Flag_BorderColour,
                                     UI_Axis_Count);
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
  f32 accumulated_size = 0.0f;
  if (indie_size.type == UI_Widget_IndividualSizing_ChildrenSum)
  {
    if (root->children_layout_axis == axis)
    {
      for (UI_Widget *child = root->leftmost_child;
           child;
           child = child->next_sibling)
      {
        ui_calculate_downwards_dependent_sizes(child, axis);
        if (child->next_sibling)
        {
          accumulated_size += child->final_dims.v[axis] + root->gap[axis];
        }
        else
        {
          accumulated_size += child->final_dims.v[axis];
        }
      }
    }
    else
    {
      //
      // TODO(cj): Consider padding and gap
      //
      for (UI_Widget *child = root->leftmost_child;
           child;
           child = child->next_sibling)
      {
        ui_calculate_downwards_dependent_sizes(child, axis);
        f32 child_size = child->final_dims.v[axis];
        if (child_size > accumulated_size)
        {
          accumulated_size = child_size;
        }
      }
    }
    
    root->final_dims.v[axis] += accumulated_size + root->padding[axis] * 2.0f;
  }
}

function void
ui_calculate_final_positions(UI_Widget *root, UI_AxisType axis)
{
  UI_Widget_IndividualSize indie_size = root->individual_size[axis];
  f32 accumulated_p = 0.0f;
  if (indie_size.type == UI_Widget_IndividualSizing_ChildrenSum)
  {
    if (root->children_layout_axis == axis)
    {
      //
      // TODO(cj): Consider padding and gap
      //
      for (UI_Widget *child = root->leftmost_child;
           child;
           child = child->next_sibling)
      {
        child->rel_parent_p.v[axis] += accumulated_p + root->border_thickness + root->padding[axis];
        accumulated_p += child->final_dims.v[axis] + root->gap[axis];
        
        child->final_p.v[axis] = root->final_p.v[axis] + child->rel_parent_p.v[axis];
        ui_calculate_final_positions(child, axis);
      }
    }
    else
    {
      //
      // TODO(cj): Consider padding and gap
      //
      for (UI_Widget *child = root->leftmost_child;
           child;
           child = child->next_sibling)
      {
        child->final_p.v[axis] = root->final_p.v[axis] + root->border_thickness + root->padding[axis];
        ui_calculate_final_positions(child, axis);
      }
    }
  }
}

function void
ui_render(UI_Context *ctx, UI_Widget *root)
{
  if (root->flags & (UI_Widget_Flag_BackgroundColour))
  {
    ui_add_quad_per_vertex_colours(ctx->quads, root->final_p, root->final_dims,
                                   root->smoothness, root->vertex_roundness,
                                   root->tl_bg_colour, root->bl_bg_colour,
                                   root->tr_bg_colour, root->br_bg_colour,
                                   0.0f);
  }
  
  if (root->flags & (UI_Widget_Flag_BorderColour))
  {
    ui_add_quad_per_vertex_colours(ctx->quads, root->final_p, root->final_dims,
                                   root->smoothness, root->vertex_roundness,
                                   root->tl_border_colour, root->bl_border_colour,
                                   root->tr_border_colour, root->br_border_colour,
                                   root->border_thickness);
  }
  
  if (root->flags & UI_Widget_Flag_StringContent)
  {
    ui_add_string(ctx->quads, ctx->font, root->final_p, root->text_colour, root->str8_content);
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
  ui_calculate_final_positions(ctx->root, UI_Axis_X);
  
  ui_calculate_standalone_sizes(ctx->root, UI_Axis_Y);
  ui_calculate_downwards_dependent_sizes(ctx->root, UI_Axis_Y);
  ui_calculate_final_positions(ctx->root, UI_Axis_Y);
  
  // SOON:
  // ui_animate(ctx);
  
  // NOTE(cj): rendering
  ui_render(ctx, ctx->root);
  m_arena_clear(ctx->front_util_arena);
  Swap(M_Arena *, ctx->front_util_arena, ctx->back_util_arena);
}