#ifndef UI_H
#define UI_H

// NOTE(cj): the "type" of the widget
typedef u64 UI_Widget_Flag;
enum
{
  UI_Widget_Flag_Clickable = 0x1,
  UI_Widget_Flag_StringContent = 0x2,
};

typedef u64 UI_Widget_IndividualSizingType;
enum
{
  UI_Widget_IndividualSizing_Null,
  UI_Widget_IndividualSizing_ChildrenSum,
  UI_Widget_IndividualSizing_Pixels,
  UI_Widget_IndividualSizing_Count,
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
  u64 h[1];
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
  u64 build_id;
  UI_Key key;
  UI_Widget_Flag flags;
  String_U8_Const str8_identifier;
  
  // NOTE(cj): String context
  String_U8_Const str8_content;
  
  // NOTE(cj): used for computing layout
  v2f rel_parent_p;
  v2f text_dims;
  UI_Widget_IndividualSizingType individual_size_type;
  UI_AxisType children_layout_axis;
  
  // NOTE(cj): final calculation
  v2f final_p;
  v2f final_dims;
  
  // NOTE(cj): properties
  f32 padding[UI_Axis_Count];
  f32 gap[UI_Axis_Count];
  
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

#define UI_MaxStackSize 64
#define UI_CacheSize 128
typedef struct
{
  UI_Widget *root;
  UI_Widget *cache[UI_CacheSize];
  UI_Widget *free_widgets;
  
  u64 current_build_id;
  
  OS_Input *input;
  R_UI_QuadArray *ui_quads;
  R_Font font;
  
  f32 dt_step_secs;
  
  // stacks
  UI_DefineStack(f32, padding_x);
  UI_DefineStack(f32, padding_y);
  UI_DefineStack(f32, gap_x);
  UI_DefineStack(f32, gap_y);
  
  UI_DefineStack(v4f, vertex_roundness);
  UI_DefineStack(v4f, tl_bg_colour);
  UI_DefineStack(v4f, bl_bg_colour);
  UI_DefineStack(v4f, tr_bg_colour);
  UI_DefineStack(v4f, br_bg_colour);
  
  UI_DefineStack(v4f, border_thickness);
  UI_DefineStack(v4f, tl_border_colour);
  UI_DefineStack(v4f, bl_border_colour);
  UI_DefineStack(v4f, tr_border_colour);
  UI_DefineStack(v4f, br_border_colour);
} UI_Context;

UI_DefineStackFN(f32, padding_x);
UI_DefineStackFN(f32, padding_y);
UI_DefineStackFN(f32, gap_x);
UI_DefineStackFN(f32, gap_y);

UI_DefineStackFN(v4f, vertex_roundness);
UI_DefineStackFN(v4f, tl_bg_colour);
UI_DefineStackFN(v4f, bl_bg_colour);
UI_DefineStackFN(v4f, tr_bg_colour);
UI_DefineStackFN(v4f, br_bg_colour);

UI_DefineStackFN(v4f, border_thickness);
UI_DefineStackFN(v4f, tl_border_colour);
UI_DefineStackFN(v4f, bl_border_colour);
UI_DefineStackFN(v4f, tr_border_colour);
UI_DefineStackFN(v4f, br_border_colour);

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

function UI_Context *ui_create_context(OS_Input *input, R_UI_QuadArray *quads, R_Font font);
function void        ui_begin(UI_Context *ctx, u32 reso_width, u32 reso_height, f32 dt_step_secs);
function void        ui_end(UI_Context *ctx);

function UI_Widget *ui_push_vlayout(UI_Context *ctx, String_U8_Const name);
function UI_Widget *ui_push_hlayout(UI_Context *ctx, String_U8_Const name);
function UI_Widget *ui_push_labelf(UI_Context *ctx, String_U8_Const str, ...);
function UI_Widget *ui_push_buttonf(UI_Context *ctx, String_U8_Const str, ...);
//function UI_Widget *ui_push_text_input(UI_Context *ctx, String_U8_Const name, String_U8 *result);

#endif //UI_H