#ifndef WINDOWS_STUFF_H
#define WINDOWS_STUFF_H

typedef u16 OS_Input_KeyType;
enum
{
  OS_Input_KeyType_Escape,
  OS_Input_KeyType_Space,
  
  OS_Input_KeyType_0, OS_Input_KeyType_1, OS_Input_KeyType_2, OS_Input_KeyType_3, OS_Input_KeyType_4,
  OS_Input_KeyType_5, OS_Input_KeyType_6, OS_Input_KeyType_7, OS_Input_KeyType_8, OS_Input_KeyType_9,
  
  OS_Input_KeyType_A, OS_Input_KeyType_B, OS_Input_KeyType_C, OS_Input_KeyType_D, OS_Input_KeyType_E,
  OS_Input_KeyType_F, OS_Input_KeyType_G, OS_Input_KeyType_H, OS_Input_KeyType_I, OS_Input_KeyType_J,
  OS_Input_KeyType_K, OS_Input_KeyType_L, OS_Input_KeyType_M, OS_Input_KeyType_N, OS_Input_KeyType_O,
  OS_Input_KeyType_P, OS_Input_KeyType_Q, OS_Input_KeyType_R, OS_Input_KeyType_S, OS_Input_KeyType_T,
  OS_Input_KeyType_U, OS_Input_KeyType_V, OS_Input_KeyType_W, OS_Input_KeyType_X, OS_Input_KeyType_Y,
  OS_Input_KeyType_Z,
  
  OS_Input_KeyType_Count,
};

typedef u16 OS_Input_ButtonType;
enum
{
  OS_Input_ButtonType_Left,
  OS_Input_ButtonType_Right,
  OS_Input_ButtonType_Count,
};

typedef u8 OS_Input_InteractFlag;
enum
{
  OS_Input_InteractFlag_Pressed = 0x1,
  OS_Input_InteractFlag_Released = 0x2,
  OS_Input_InteractFlag_Held = 0x4,
};

#define OS_KeyPressed(inp,keytype) ((inp)->key[keytype]&OS_Input_InteractFlag_Pressed)
#define OS_KeyReleased(inp,keytype) ((inp)->key[keytype]&OS_Input_InteractFlag_Released)
#define OS_KeyHeld(inp,keytype) ((inp)->key[keytype]&OS_Input_InteractFlag_Held)
#define OS_ButtonReleased(inp,btntype) ((inp)->button[btntype]&OS_Input_InteractFlag_Released)
typedef struct
{
  OS_Input_InteractFlag key[OS_Input_KeyType_Count];
  OS_Input_InteractFlag button[OS_Input_ButtonType_Count];
  s32 mouse_x, mouse_y, prev_mouse_x, prev_mouse_y;
} OS_Input;

typedef struct
{
  HWND handle;
  s32 client_width;
  s32 client_height;
  OS_Input input;
} OS_Window;

function void  w32_prevent_dpi_scaling(void);
function void  w32_create_window(OS_Window *window, char *name, s32 client_width, s32 client_height);
function void  w32_fill_input(OS_Window *window);
#endif //WINDOWS_STUFF_H