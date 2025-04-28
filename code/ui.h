#ifndef UI_H
#define UI_H

// NOTE(cj): the "type" of the widget
typedef u64 UI_Widget_Flag;
enum
{
  UI_Widget_Flag_Clickable = 0x1,
  UI_Widget_Flag_StringContent = 0x2,
  UI_Widget_Flag_BackgroundColour = 0x4,
  UI_Widget_Flag_BorderColour = 0x8,
  UI_Widget_Flag_Texture = 0x10,
};

typedef u64 UI_Widget_IndividualSizingType;
enum
{
  UI_Widget_IndividualSizing_Null,
  UI_Widget_IndividualSizing_ChildrenSum,
  UI_Widget_IndividualSizing_Pixels,
  UI_Widget_IndividualSizing_PercentOfParent,
  UI_Widget_IndividualSizing_Count,
};

typedef u64 UI_Metric;
enum
{
  UI_Metric_Pixels,
  UI_Metric_Percent,
  UI_Metric_Count,
};

typedef u64 UI_Anchor;
enum
{
  UI_Anchor_Begin,
  UI_Anchor_Center,
  UI_Anchor_End,
  UI_Anchor_Count,
};

typedef struct
{
  UI_Metric metric;
  f32 value;
} UI_Absolute_Position;

typedef u64 UI_Widget_PositionType;
enum
{
  UI_Widget_Position_Normal,
  UI_Widget_Position_Absolute,
  UI_Widget_Position_Count,
};

typedef struct
{
  UI_Widget_IndividualSizingType type;
  f32 value;
} UI_Widget_IndividualSize;

typedef struct
{
  v2f clip_p;
  v2f clip_dims;
} UI_TextureClipped;

typedef u64 UI_Widget_TextCenteringType;
enum
{
  UI_Widget_TextCentering_Begin,
  UI_Widget_TextCentering_Center,
  UI_Widget_TextCentering_Count,
};

typedef u64 UI_AxisType;
enum
{
  UI_Axis_X,
  UI_Axis_Y,
  UI_Axis_Count,
};

typedef union
{
  u64 key;
} UI_Key;

typedef struct UI_Widget UI_Widget;
struct UI_Widget
{
  UI_Widget *parent, *next_sibling, *prev_sibling;
  UI_Widget *rightmost_child, *leftmost_child;
  union
  {
    UI_Widget *next_in_free, *next_in_hash;
  };
  
  // NOTE(cj): used for identifying/validating widgets
  u64 build_last_touch_index;
  UI_Key key;
  UI_Widget_Flag flags;
  String_U8_Const str8_identifier;
  
  // NOTE(cj): String context
  String_U8_Const str8_content;
  UI_TextureClipped texture;
  
  // NOTE(cj): used for computing layout
  v2f rel_text_p;
  v2f rel_parent_p;
  f32 padding[UI_Axis_Count];
  f32 gap[UI_Axis_Count];
  UI_Widget_IndividualSize individual_size[UI_Axis_Count];
  UI_Widget_TextCenteringType text_centering[UI_Axis_Count];
  UI_Widget_PositionType position;
  UI_Absolute_Position absolute_p[UI_Axis_Count]; // only used when position = Absolute
  v2f text_dims;
  
  // NOTE(cj): final calculation
  v2f final_text_p;
  v2f final_p;
  v2f final_dims;
  
  // NOTE(cj): properties
  v4f tl_bg_colour;
  v4f bl_bg_colour;
  v4f tr_bg_colour;
  v4f br_bg_colour;
  f32 vertex_roundness;
  
  v4f tl_border_colour;
  v4f bl_border_colour;
  v4f tr_border_colour;
  v4f br_border_colour;
  f32 border_thickness;
  
  v4f text_colour;
  
  f32 hot_t, active_t;
  f32 smoothness;
};

#define UI_DefineStack(type,name) \
type name##_stack[UI_MaxStackSize];\
u64 name##_ptr;\
b32 name##_auto_pop

#define UI_DefineStackFN(type,name) \
inline function type ui_##name##_peek(UI_Context *ctx) \
{\
Assert(ctx->name##_ptr > 0);\
return (ctx->name##_stack[ctx->name##_ptr-1]);\
}\
inline function type ui_##name##_pop(UI_Context *ctx) \
{\
Assert(ctx->name##_ptr>0);\
return (ctx->name##_stack[--ctx->name##_ptr]);\
}\
inline function type ui_##name##_push(UI_Context *ctx, type t) \
{\
Assert(ctx->name##_ptr < UI_MaxStackSize);\
ctx->name##_stack[ctx->name##_ptr++] = t;\
return t;\
}\
inline function type ui_##name##_next(UI_Context *ctx, type t) \
{\
type result = ui_##name##_push(ctx,t);\
ctx->name##_auto_pop=1;\
return(result);\
}\
inline function type ui_##name##_peek_or_auto_pop(UI_Context *ctx) \
{\
if (ctx->name##_auto_pop)\
{\
ctx->name##_auto_pop=0;\
return ui_##name##_pop(ctx);\
}\
else\
{\
return ui_##name##_peek(ctx);\
}\
}

// TODO(cj): UI Improvements:
// - BORDER BOX (LIKE CSS) ONLY SIZING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define UI_MaxStackSize 64
#define UI_CacheSize 128
typedef struct
{
  M_Arena *arena, *front_util_arena, *back_util_arena;
  
  UI_Widget *root;
  f32 max_width;
  f32 max_height;
  
  // NOTE(cj): The purpose of the cache is for saving
  // ui layout calculation for user interaction.
  // Because, when the user first called the create_widget function,
  // the widget does not have any size or position yet, and hence we 
  // will recieve one frame delay the first time we call the create_widget function.
  // The layout calculation algorithm happens on ui_end. We will cache this result
  // on this right below me.
  UI_Widget *cache[UI_CacheSize];
  UI_Widget *free_widgets;
  
  u64 current_build_index;
  
  OS_Input *input;
  R_UI_QuadArray *quads;
  R_Font font;
  R_Texture2D sprite_sheet;
  
  f32 dt_step_secs;
  
  UI_Key hot, active;
  
  // stacks
  UI_DefineStack(UI_Widget *, parent);
  
  UI_DefineStack(UI_Widget_IndividualSize, size_x);
  UI_DefineStack(UI_Widget_IndividualSize, size_y);
  
  UI_DefineStack(UI_Widget_PositionType, position);
  UI_DefineStack(UI_Absolute_Position, absolute_x);
  UI_DefineStack(UI_Absolute_Position, absolute_y);
  
  UI_DefineStack(UI_TextureClipped, texture);
  
  UI_DefineStack(f32, padding_x);
  UI_DefineStack(f32, padding_y);
  UI_DefineStack(f32, gap_x);
  UI_DefineStack(f32, gap_y);
  
  UI_DefineStack(f32, vertex_roundness);
  UI_DefineStack(v4f, tl_bg_colour);
  UI_DefineStack(v4f, bl_bg_colour);
  UI_DefineStack(v4f, tr_bg_colour);
  UI_DefineStack(v4f, br_bg_colour);
  
  UI_DefineStack(f32, border_thickness);
  UI_DefineStack(v4f, tl_border_colour);
  UI_DefineStack(v4f, bl_border_colour);
  UI_DefineStack(v4f, tr_border_colour);
  UI_DefineStack(v4f, br_border_colour);
  
  UI_DefineStack(f32, smoothness);
  
  UI_DefineStack(UI_Widget_TextCenteringType, text_centering_x);
  UI_DefineStack(UI_Widget_TextCenteringType, text_centering_y);
  UI_DefineStack(v4f, text_colour);
} UI_Context;

UI_DefineStackFN(UI_Widget *, parent);

UI_DefineStackFN(UI_Widget_IndividualSize, size_x);
UI_DefineStackFN(UI_Widget_IndividualSize, size_y);

UI_DefineStackFN(UI_Widget_PositionType, position);
UI_DefineStackFN(UI_Absolute_Position, absolute_x);
UI_DefineStackFN(UI_Absolute_Position, absolute_y);

UI_DefineStackFN(UI_TextureClipped, texture);

UI_DefineStackFN(f32, padding_x);
UI_DefineStackFN(f32, padding_y);
UI_DefineStackFN(f32, gap_x);
UI_DefineStackFN(f32, gap_y);

UI_DefineStackFN(f32, vertex_roundness);
UI_DefineStackFN(v4f, tl_bg_colour);
UI_DefineStackFN(v4f, bl_bg_colour);
UI_DefineStackFN(v4f, tr_bg_colour);
UI_DefineStackFN(v4f, br_bg_colour);

UI_DefineStackFN(f32, border_thickness);
UI_DefineStackFN(v4f, tl_border_colour);
UI_DefineStackFN(v4f, bl_border_colour);
UI_DefineStackFN(v4f, tr_border_colour);
UI_DefineStackFN(v4f, br_border_colour);

UI_DefineStackFN(f32, smoothness);

UI_DefineStackFN(UI_Widget_TextCenteringType, text_centering_x);
UI_DefineStackFN(UI_Widget_TextCenteringType, text_centering_y);
UI_DefineStackFN(v4f, text_colour);

inline function void
ui_size_push(UI_Context *ctx, UI_Widget_IndividualSize x, UI_Widget_IndividualSize y)
{
  ui_size_x_push(ctx, x);
  ui_size_y_push(ctx, y);
}

inline function void
ui_size_pop(UI_Context *ctx)
{
  ui_size_x_pop(ctx);
  ui_size_y_pop(ctx);
}

inline function v2f
ui_gap_push(UI_Context *ctx, f32 x, f32 y)
{
  v2f result = {x, y};
  ui_gap_x_push(ctx, x);
  ui_gap_y_push(ctx, y);
  return(result);
}

inline function v2f
ui_padding_push(UI_Context *ctx, f32 x, f32 y)
{
  v2f result = {x, y};
  ui_padding_x_push(ctx, x);
  ui_padding_y_push(ctx, y);
  return(result);
}

inline function v2f
ui_padding_pop(UI_Context *ctx)
{
  v2f result = {ui_padding_x_pop(ctx), ui_padding_y_pop(ctx)};
  return(result);
}

#define ui_bg_transparent_next(ctx) ui_bg_colour_next((ctx),rgba(0,0,0,0))
inline function v4f
ui_bg_colour_next(UI_Context *ctx, v4f colour)
{
  ui_tl_bg_colour_next(ctx, colour);
  ui_bl_bg_colour_next(ctx, colour);
  ui_tr_bg_colour_next(ctx, colour);
  ui_br_bg_colour_next(ctx, colour);
  return colour;
}

inline function v4f
ui_border_colour_next(UI_Context *ctx, v4f colour)
{
  ui_tl_border_colour_next(ctx, colour);
  ui_bl_border_colour_next(ctx, colour);
  ui_tr_border_colour_next(ctx, colour);
  ui_br_border_colour_next(ctx, colour);
  return colour;
}

#define ui_vlayout_pop(ctx) ui_parent_pop(ctx)
#define ui_hlayout_pop(ctx) ui_parent_pop(ctx)

// UI Helpers
inline function UI_Widget_IndividualSize ui_null_size(void);
inline function UI_Widget_IndividualSize ui_pixel_size(f32 value);
inline function UI_Widget_IndividualSize ui_children_sum_size(f32 initial_size);
inline function UI_Widget_IndividualSize ui_percent_of_parent_size(f32 percent);
inline function UI_Absolute_Position ui_absolute_pixels(f32 v);
inline function UI_Absolute_Position ui_absolute_percent(f32 v);
inline function UI_TextureClipped ui_texture(v2f clip_p, v2f clip_dims);

// UI init
function UI_Context *ui_create_context(OS_Input *input, R_UI_QuadArray *quads, R_Font font, R_Texture2D sprite_sheet);
function void        ui_begin(UI_Context *ctx, u32 reso_width, u32 reso_height, f32 dt_step_secs);
function void        ui_end(UI_Context *ctx);

// UI Widgets
function UI_Widget *ui_push_vlayout(UI_Context *ctx, f32 init_height, v2f padding, v2f gap, String_U8_Const name);
function UI_Widget *ui_push_hlayout(UI_Context *ctx, f32 init_width, v2f padding, v2f gap, String_U8_Const name);
function UI_Widget *ui_push_labelf(UI_Context *ctx, String_U8_Const str, ...);
function UI_Widget *ui_push_label(UI_Context *ctx, String_U8_Const str);
function UI_Widget *ui_push_texture(UI_Context *ctx, UI_TextureClipped texture, f32 width, f32 height, String_U8_Const str);
//function UI_Widget *ui_push_buttonf(UI_Context *ctx, String_U8_Const str, ...);
//function UI_Widget *ui_push_progress_bar_with_stringf(UI_Context *ctx, f32 current_progress, v4f progress_colour, f32 max_progress, v4f border_colour, String_U8_Const str, ...);
//function UI_Widget *ui_push_text_input(UI_Context *ctx, String_U8_Const name, String_U8 *result);

#endif //UI_H