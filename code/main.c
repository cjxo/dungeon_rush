#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define NOMINMAX
#define _NO_CRT_STDIO_INLINE
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
#include "mathematical_objects.h"
#include "game.h"

#include "mathematical_objects.c"

#if defined(GAME_DEBUG)
# define DX11_ShaderCompileFlags (D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR|D3DCOMPILE_SKIP_OPTIMIZATION|D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS)
#else
# define DX11_ShaderCompileFlags (D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR|D3DCOMPILE_OPTIMIZATION_LEVEL3)
#endif

function void *
os_mem_reserve(u64 reserve_size)
{
  return VirtualAlloc(0, reserve_size, MEM_RESERVE, PAGE_NOACCESS);
}

function b32
os_mem_commit(void *base, u64 commit_size)
{
  return VirtualAlloc(base, commit_size, MEM_COMMIT, PAGE_READWRITE) != 0;
}

function b32
os_mem_decommit(void *base, u64 decommit_size)
{
  return VirtualFree(base, decommit_size, MEM_DECOMMIT) != 0;
}

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
  m44 proj;
  m44 world_to_cam;
} DX11_Game_CBuffer0;

typedef struct
{
  // window
  HWND window;
  s32 window_width;
  s32 window_height;
  
  OS_Input input;
  
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
  
  s32 game_diffse_sheet_width;
  s32 game_diffse_sheet_height;
  ID3D11ShaderResourceView *game_diffuse_sheet_view;
  
  ID3D11RasterizerState *rasterizer_fill_no_cull_ccw;
  
  ID3D11SamplerState *sampler_point_all;
  
  ID3D11BlendState *blend_blend;
} Application_State;

function void
prevent_dpi_scaling(void)
{
  typedef BOOL WINAPI T_SetProcessDpiAwarenessContext(void *);
  typedef BOOL WINAPI T_SetProcessDPIAware(void);
  
  HMODULE user32 = LoadLibraryA("User32.dll");
  if (user32)
  {
    T_SetProcessDpiAwarenessContext *fn = (T_SetProcessDpiAwarenessContext *)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    if (fn)
    {
      fn((void *)(-4));
    }
    else
    {
      T_SetProcessDPIAware *fn1 = (T_SetProcessDPIAware *)GetProcAddress(user32, "SetProcessDPIAware");
      if (fn1)
      {
        fn1();
      }
    }
    
    FreeLibrary(user32);
  }
}

function void
dx11_create_device(Application_State *app)
{
  UINT flags = (D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT);
#if defined(DR_DEBUG)
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif        
  D3D_FEATURE_LEVEL feature_levels = D3D_FEATURE_LEVEL_11_0;
  if(SUCCEEDED(D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, &feature_levels, 1,
                                 D3D11_SDK_VERSION, &app->device, 0, &app->device_context)))
  {
#if defined(DR_DEBUG)
    ID3D11InfoQueue *dx11_infoq;
    IDXGIInfoQueue *dxgi_infoq;
    
    if (SUCCEEDED(ID3D11Device_QueryInterface(app->device, &IID_ID3D11InfoQueue, &dx11_infoq)))
    {
      ID3D11InfoQueue_SetBreakOnSeverity(dx11_infoq, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      ID3D11InfoQueue_SetBreakOnSeverity(dx11_infoq, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
      ID3D11InfoQueue_Release(dx11_infoq);
      
      typedef HRESULT (* DXGIGetDebugInterfaceFN)(REFIID, void **);
      
      HMODULE lib = LoadLibraryA("Dxgidebug.dll");
      if (lib)
      {
        DXGIGetDebugInterfaceFN fn = (DXGIGetDebugInterfaceFN)GetProcAddress(lib, "DXGIGetDebugInterface");
        if (fn)
        {
          if (SUCCEEDED(fn(&IID_IDXGIInfoQueue, &dxgi_infoq)))
          {
            IDXGIInfoQueue_SetBreakOnSeverity(dxgi_infoq, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            IDXGIInfoQueue_SetBreakOnSeverity(dxgi_infoq, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
            IDXGIInfoQueue_Release(dxgi_infoq);
          }
        }
        
        FreeLibrary(lib);
      }
    }
#endif
  }
}

function void
dx11_create_swap_chain(Application_State *app)
{
  IDXGIDevice2 *dxgi_device;
  IDXGIAdapter *dxgi_adapter;
  IDXGIFactory2 *dxgi_factory;
  ID3D11Texture2D *backbuffer_tex;
  
  if (SUCCEEDED(ID3D11Device_QueryInterface(app->device, &IID_IDXGIDevice2, &dxgi_device)))
  {
    if (SUCCEEDED(IDXGIDevice2_GetAdapter(dxgi_device, &dxgi_adapter)))
    {
      if (SUCCEEDED(IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, &dxgi_factory)))
      {
        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {0};
        swap_chain_desc.Width              = app->reso_width;
        swap_chain_desc.Height             = app->reso_height;
        swap_chain_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.Stereo             = FALSE;
        swap_chain_desc.SampleDesc.Count   = 1;
        swap_chain_desc.SampleDesc.Quality = 0;
        swap_chain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount        = 2;
        swap_chain_desc.Scaling            = DXGI_SCALING_STRETCH;
        swap_chain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_desc.Flags              = 0;
        
        if (SUCCEEDED(IDXGIFactory2_CreateSwapChainForHwnd(dxgi_factory, (IUnknown *)app->device, app->window,
                                                           &swap_chain_desc, 0, 0,
                                                           &app->swap_chain)))
        {
          IDXGIFactory2_MakeWindowAssociation(dxgi_factory, app->window, DXGI_MWA_NO_ALT_ENTER);
          
          if (SUCCEEDED(IDXGISwapChain1_GetBuffer(app->swap_chain, 0, &IID_ID3D11Texture2D, &backbuffer_tex)))
          {
            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { 0 };
            rtv_desc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtv_desc.ViewDimension       = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice  = 0;
            ID3D11Device_CreateRenderTargetView(app->device, (ID3D11Resource *)backbuffer_tex,
                                                &rtv_desc, &app->render_target);
            
            ID3D11Texture2D_Release(backbuffer_tex);
          }
        }
        
        IDXGIFactory2_Release(dxgi_factory);
      }
      
      IDXGIAdapter_Release(dxgi_adapter);
    }
    
    IDXGIDevice2_Release(dxgi_device);
  }
}

function void
dx11_create_vertex_and_pixel_shader(ID3D11Device *device, LPCWSTR filename, char *vs_entry, char *ps_entry, ID3D11VertexShader **out_vshader, ID3D11PixelShader **out_pshader)
{
  ID3DBlob *bytecode, *errmsg;
  HRESULT hr = D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs_entry, "vs_5_0", DX11_ShaderCompileFlags, 0, &bytecode, &errmsg);
  
  if (hr == 0x80070002)
  {
    Assert(!"File Not Found!");
  }
  
  if (hr)
  {
    OutputDebugStringA(ID3D10Blob_GetBufferPointer(errmsg));
    Assert(0);
  }
  
  if (SUCCEEDED(ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(bytecode),
                                                ID3D10Blob_GetBufferSize(bytecode),
                                                0, out_vshader)))
  {
    ID3D10Blob_Release(bytecode);
    
    D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps_entry, "ps_5_0", DX11_ShaderCompileFlags, 0, &bytecode, &errmsg);
    
    if (errmsg)
    {
      OutputDebugStringA(ID3D10Blob_GetBufferPointer(errmsg));
      Assert(0);
    }
    
    if (SUCCEEDED(ID3D11Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(bytecode),
                                                 ID3D10Blob_GetBufferSize(bytecode),
                                                 0, out_pshader)))
    {
      ID3D10Blob_Release(bytecode);
    }
    else
    {
      OutputDebugStringA("Failed to create pixel shader!\n");
      Assert(0);
    }
  }
  else
  {
    OutputDebugStringA("Failed to create vertex shader!\n");
    Assert(0);
  }
}

function void
dx11_create_game_main_state(Application_State *app)
{
  dx11_create_vertex_and_pixel_shader(app->device, 
                                      L"..\\code\\shaders\\game-shader.hlsl", "vs_main", "ps_main",
                                      &app->vertex_shader_main, &app->pixel_shader_main);
  
  D3D11_BUFFER_DESC cbuf_desc =
  {
    .ByteWidth = sizeof(DX11_Game_CBuffer0),
    .Usage = D3D11_USAGE_DYNAMIC,
    .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags = 0,
    .StructureByteStride = 0,
  };
  
  if (SUCCEEDED(ID3D11Device_CreateBuffer(app->device, &cbuf_desc, 0, &app->cbuffer0_main)))
  {
    D3D11_BUFFER_DESC sbuffer_desc =
    {
      .ByteWidth = sizeof(Game_Quad) * Game_MaxQuads,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = sizeof(Game_Quad),
    };
    
    if (SUCCEEDED(ID3D11Device_CreateBuffer(app->device, &sbuffer_desc, 0, &app->sbuffer_main)))
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC sbuffer_srv_desc =
      {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
        .Buffer = { .NumElements = Game_MaxQuads }
      };
      
      ID3D11Device_CreateShaderResourceView(app->device, (ID3D11Resource *)app->sbuffer_main, &sbuffer_srv_desc, &app->sbuffer_view_main);
      
      s32 tex_width, tex_height, tex_comps;
      u8 *tex_data = stbi_load("../res/textures/sheet.png", &tex_width, &tex_height, &tex_comps, 4);
      Assert(tex_data != 0);
      if (tex_data)
      {
        D3D11_TEXTURE2D_DESC texture_desc =
        {
          .Width = tex_width,
          .Height = tex_height,
          .MipLevels = 1,
          .ArraySize = 1,
          .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
          .SampleDesc = { 1, 0 },
          .Usage = D3D11_USAGE_IMMUTABLE,
          .BindFlags = D3D11_BIND_SHADER_RESOURCE,
          .CPUAccessFlags = 0,
          .MiscFlags = 0,
        };
        
        D3D11_SUBRESOURCE_DATA texture_data =
        {
          .pSysMem = tex_data,
          .SysMemPitch = 4 * tex_width,
          .SysMemSlicePitch = 0,
        };
        
        ID3D11Texture2D *game_diffuse_sheet_tex = 0;
        ID3D11Device_CreateTexture2D(app->device, &texture_desc, &texture_data, &game_diffuse_sheet_tex);
        Assert(game_diffuse_sheet_tex != 0);
        if (game_diffuse_sheet_tex)
        {
          ID3D11Device_CreateShaderResourceView(app->device, (ID3D11Resource *)game_diffuse_sheet_tex, 0, &app->game_diffuse_sheet_view);
          Assert(app->game_diffuse_sheet_view != 0);
          if (app->game_diffuse_sheet_view)
          {
            app->game_diffse_sheet_width = tex_width;
            app->game_diffse_sheet_height = tex_height;
          }
          else
          {
            // NOTE(cj): LOGGING
          }
          
          ID3D11Texture2D_Release(game_diffuse_sheet_tex);
        }
        else
        {
          // NOTE(cj): LOGGING
        }
        
        stbi_image_free(tex_data);
      }
      else
      {
        // NOTE(cj): LOGGING
      }
    }
    else
    {
      // NOTE(cj): LOGGING
    }
#if 0
    cbuf_desc.ByteWidth = sizeof(DX11_Game_CBuffer1);
    if (SUCCEEDED(ID3D11Device1_CreateBuffer(g_dx11_dev, &cbuf_desc, 0, &g_dx11_game_cbuffer1)))
    {
    }
#endif
  }
}

function void
dx11_create_rasterizer_states(Application_State *app)
{
  D3D11_RASTERIZER_DESC raster_desc;
  raster_desc.FillMode = D3D11_FILL_SOLID;
  raster_desc.CullMode = D3D11_CULL_NONE;
  raster_desc.FrontCounterClockwise = TRUE;
  raster_desc.DepthBias = 0;
  raster_desc.DepthBiasClamp = 0.0f;
  raster_desc.SlopeScaledDepthBias = 0.0f;
  raster_desc.DepthClipEnable = TRUE;
  raster_desc.ScissorEnable = FALSE;
  raster_desc.MultisampleEnable = FALSE;
  raster_desc.AntialiasedLineEnable = FALSE;
  
  ID3D11Device_CreateRasterizerState(app->device, &raster_desc, &app->rasterizer_fill_no_cull_ccw);
}

function void
dx11_create_sampler_states(Application_State *app)
{
  D3D11_SAMPLER_DESC sam_desc;
  //sam_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  //sam_desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
  //sam_desc.Filter              = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
  //sam_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sam_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  //sam_desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
  //sam_desc.Filter = D3D11_FILTER_ANISOTROPIC;
  sam_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sam_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sam_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sam_desc.MipLODBias = 0;
  //sam_desc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
  sam_desc.MaxAnisotropy = 1;
  sam_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sam_desc.BorderColor[0] = 1.0f;
  sam_desc.BorderColor[1] = 1.0f;
  sam_desc.BorderColor[2] = 1.0f;
  sam_desc.BorderColor[3] = 1.0f;
  sam_desc.MinLOD = 0;
  sam_desc.MaxLOD = D3D11_FLOAT32_MAX;
  
  ID3D11Device_CreateSamplerState(app->device, &sam_desc, &app->sampler_point_all);
}

function void
dx11_create_blend_states(Application_State *app)
{
  D3D11_BLEND_DESC blend_desc;
  blend_desc.AlphaToCoverageEnable = FALSE;
  blend_desc.IndependentBlendEnable = FALSE;
  blend_desc.RenderTarget[0].BlendEnable = TRUE;
  blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
  //blend_desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_INV_DEST_ALPHA;
  blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  //blend_desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
  blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  
  ID3D11Device_CreateBlendState(app->device, &blend_desc, &app->blend_blend);
  
}

function OS_Input_KeyType
w32_map_wparam_to_keytype(WPARAM wParam)
{
  OS_Input_KeyType key = OS_Input_KeyType_Count;
  switch (wParam)
  {
    case VK_ESCAPE:
    {
      key = OS_Input_KeyType_Escape;
    } break;
    
    case VK_SPACE:
    {
      key = OS_Input_KeyType_Space;
    } break;
    
    default:
    {
      if ((wParam >= 'A') && (wParam <= 'Z'))
      {
        key = (OS_Input_KeyType)(OS_Input_KeyType_A + (wParam - 'A'));
      }
      else if ((wParam >= '0') && (wParam <= '9'))
      {
        key = (OS_Input_KeyType)(OS_Input_KeyType_0 + (wParam - '0'));
      }
    } break;
  }
  
  return(key);
}

function LRESULT
w32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  
  if (message == WM_NCCREATE)
  {
    OutputDebugStringA("Hello Traveller!\n");
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCT *)lparam)->lpCreateParams));
    return DefWindowProc(window, message, wparam, lparam);
  }
  
  Application_State *app = (Application_State *)GetWindowLongPtrA(window, GWLP_USERDATA);
  if (!app)
  {
    return DefWindowProc(window, message, wparam, lparam);
  }
  
  switch (message)
  {
    case WM_CREATE:
    {
      OutputDebugStringA("Hello Creator!\n");
      
      app->reso_width = 1280;
      app->reso_height = 720;
      
    } break;
    
    case WM_CLOSE:
    {
      DestroyWindow(window);
    } break;
    
    case WM_DESTROY:
    {
      PostQuitMessage(0);
    } break;
    
    case WM_KEYDOWN:
    {
      OS_Input_KeyType key = w32_map_wparam_to_keytype(wparam);
      if (key != OS_Input_KeyType_Count)
      {
        app->input.key[key] |= OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Held;
      }
    } break;
    
    case WM_KEYUP:
    {
      OS_Input_KeyType key = w32_map_wparam_to_keytype(wparam);
      if (key != OS_Input_KeyType_Count)
      {
        app->input.key[key] &= ~(OS_Input_InteractFlag_Held);
        app->input.key[key] |= OS_Input_InteractFlag_Released;
      }
    } break;
    
    case WM_LBUTTONDOWN:
    {
      app->input.button[OS_Input_ButtonType_Left] |= OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Held;
    } break;
    
    case WM_LBUTTONUP:
    {
      app->input.button[OS_Input_ButtonType_Left] |= OS_Input_InteractFlag_Released;
      app->input.button[OS_Input_ButtonType_Left] &= ~OS_Input_InteractFlag_Held;
    } break;
    
    
    case WM_RBUTTONDOWN:
    {
      app->input.button[OS_Input_ButtonType_Right] |= OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Held;
    } break;
    
    case WM_RBUTTONUP:
    {
      app->input.button[OS_Input_ButtonType_Right] |= OS_Input_InteractFlag_Released;
      app->input.button[OS_Input_ButtonType_Right] &= ~OS_Input_InteractFlag_Held;
    } break;
    
    default:
    {
      result = DefWindowProc(window, message, wparam, lparam);
    } break;
  }
  
  return(result);
}

function M_Arena *
m_arena_reserve(u64 reserve_size)
{
  M_Arena *result = 0;
  reserve_size = AlignAToB(reserve_size, 16);
  void *block = os_mem_reserve(reserve_size);
  
  if (block)
  {
    u64 new_commit_ptr = AlignAToB(sizeof(M_Arena), M_Arena_DefaultCommit);
    u64 new_commit_ptr_clamped = Min(new_commit_ptr, reserve_size);
    
    os_mem_commit(block, new_commit_ptr_clamped);
    result = block;
    result->base = block;
    result->commit_ptr = new_commit_ptr_clamped;
    result->stack_ptr = sizeof(M_Arena);
    result->capacity = reserve_size;
  }
  
  return(result);
}

function void *
m_arena_push(M_Arena *arena, u64 push_size)
{
  Assert(arena);
  void *result_block = 0;
  push_size = AlignAToB(push_size, 16);
  u64 desired_stack_ptr = arena->stack_ptr + push_size;
  u64 desired_commit_ptr = arena->commit_ptr;
  if (desired_stack_ptr <= arena->capacity)
  {
    if (desired_stack_ptr >= arena->commit_ptr)
    {
      u64 new_commit_ptr = AlignAToB(desired_stack_ptr, M_Arena_DefaultCommit);
      u64 new_commit_ptr_clamped = Min(new_commit_ptr, arena->capacity);
      
      if (new_commit_ptr_clamped > arena->commit_ptr)
      {
        os_mem_commit(arena->base + arena->commit_ptr, new_commit_ptr_clamped - arena->commit_ptr);
        desired_commit_ptr = new_commit_ptr_clamped;
      }
    }
    
    if (desired_commit_ptr > desired_stack_ptr)
    {
      result_block = arena->base + arena->stack_ptr;
      arena->stack_ptr = desired_stack_ptr;
      arena->commit_ptr = desired_commit_ptr;
    }
  }
  
  Assert(result_block);
  return(result_block);
}

function void
m_arena_pop(M_Arena *arena, u64 pop_size)
{
  pop_size = AlignAToB(pop_size, 16);
  Assert(pop_size <= (arena->stack_ptr + sizeof(M_Arena)));
  arena->stack_ptr -= pop_size;
  
  u64 new_commit_ptr = AlignAToB(arena->stack_ptr, M_Arena_DefaultCommit);
  if (new_commit_ptr < arena->commit_ptr)
  {
    os_mem_decommit(arena->base + new_commit_ptr, arena->commit_ptr - new_commit_ptr);
    arena->commit_ptr = new_commit_ptr;
  }
}

function Application_State g_application;

inline function Game_Quad *
game_acquire_quad(Game_QuadArray *quads)
{
  Assert(quads->count < quads->capacity);
  Game_Quad *result = quads->quads + quads->count++;
  return(result);
}

inline function Game_Quad *
game_add_rect(Game_QuadArray *quads, v3f p, v3f dims, v4f colour)
{
  Game_Quad *result = game_acquire_quad(quads);
  result->p = p;
  result->dims = dims;
  result->colour = colour;
  result->tex_id = 0;
  return(result);
}

inline function Game_Quad *
game_add_tex(Game_QuadArray *quads, v3f p, v3f dims, v4f mod)
{
  Game_Quad *result = game_add_rect(quads, p, dims, mod);
  result->uvs[0] = (v2f){ 0.0f, 1.0f };
  result->uvs[1] = (v2f){ 0.0f, 0.0f };
  result->uvs[2] = (v2f){ 1.0f, 1.0f };
  result->uvs[3] = (v2f){ 1.0f, 0.0f };
  result->tex_id = 1;
  return(result);
}

inline function Game_Quad *
game_add_tex_clipped(Game_QuadArray *quads,
                     v3f p, v3f dims,
                     v2f clip_p, v2f clip_dims,
                     v4f mod,
                     b32 flip_horizontal)
{
  Game_Quad *result = game_add_rect(quads, p, dims, mod);
  f32 x_start = clip_p.x / (f32)quads->tex_width;
  f32 x_end = (clip_p.x + clip_dims.x) / (f32)quads->tex_width;
  
  f32 y_start = clip_p.y / (f32)quads->tex_height;
  f32 y_end = (clip_p.y + clip_dims.y) / (f32)quads->tex_height;
  
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
  
  result->tex_id = 1;
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
make_enemy_skull(Game_State *game, v3f p)
{
  Entity *result = make_entity(game, EntityType_Skull, EntityFlag_Hostile);
  result->p = p;
  return(result);
}

function void
game_init(Game_State *game, s32 tex_width, s32 tex_height)
{
  game->arena = m_arena_reserve(MB(2));
  game->quads = (Game_QuadArray)
  {
    .quads = M_Arena_PushArray(game->arena, Game_Quad, Game_MaxQuads),
    .capacity = Game_MaxQuads,
    .count = 0,
    .tex_width = tex_width,
    .tex_height = tex_height,
  };
  
  // player entity
  {
    Entity *player = make_entity(game, EntityType_Player, 0);
    player->flags = 0;
    player->type = EntityType_Player;
    player->last_face_dir = 0;
    
    Animation_Config *anim_config = &(game->walk_animation);
    anim_config->current_secs = 0.0f;
    anim_config->duration_secs = 0.15f;
    anim_config->frame_idx = 0;
    for (u32 idx = 0; idx < ArrayCount(anim_config->frames); ++idx)
    {
      anim_config->frames[idx] = (Animation_Frame)
      {
        (v2f){ 16.0f * (f32)(idx), 16.0f },
        (v2f){ 16.0f, 16.0f },
      };
    }
    
    player->attack_count = 1;
    player->attacks[0] = (Attack)
    {
      .type = AttackType_ShadowSlash,
      .current_secs = 1.0f,
      .interval_secs = 1.0f,
      .damage = 6,
    };
    
    anim_config = &(player->attacks[0].animation);
    anim_config->current_secs = 0.0f;
    anim_config->duration_secs = 0.04f;
    anim_config->frame_idx = 0;
    
    anim_config->frames[0] = (Animation_Frame)
    {
      (v2f){ 0.0f, 32.0f },
      (v2f){ 32.0f, 32.0f },
      (v2f){ -11.0f, -13.0f }
    };
    
    anim_config->frames[1] = (Animation_Frame)
    {
      (v2f){ 32.0f, 32.0f },
      (v2f){ 16.0f, 32.0f },
      (v2f){ -3.0f, -16.0f }
    };
    
    anim_config->frames[2] = (Animation_Frame)
    {
      (v2f){ 48.0f, 32.0f },
      (v2f){ 32.0f, 32.0f },
      (v2f){ 14.0f, 8.0f }
    };
    
    anim_config->frames[3] = (Animation_Frame)
    {
      (v2f){ 80.0f, 32.0f },
      (v2f){ 32.0f, 32.0f },
      (v2f){ 12.0f, 11.0f }
    };
  }
  
  // test entity
  make_enemy_skull(game, (v3f) { 512, 0, 0 });
}

typedef struct
{
  Animation_Frame frame;
  b32 is_full_cycle;
} Animation_Tick_Result;

function Animation_Tick_Result
tick_animation(Animation_Config *anim, f32 seconds_elapsed)
{
  Animation_Tick_Result result;
  result.frame = anim->frames[anim->frame_idx];
  b32 is_full_cycle = 0;
  b32 time_is_up = anim->current_secs >= anim->duration_secs;
  if (time_is_up)
  {
    anim->current_secs = 0.0f;
    anim->frame_idx += 1;
    if (anim->frame_idx == ArrayCount(anim->frames))
    {
      anim->frame_idx = 0;
      is_full_cycle = 1;
    }
  }
  else
  {
    anim->current_secs += seconds_elapsed;
  } 
  
  result.is_full_cycle = is_full_cycle;
  return(result);
}

function void
game_update_and_render(Game_State *game, OS_Input *input, f32 game_update_secs)
{
  // the player is always at 0th idx
  Entity *player = game->entities;
  for (u64 entity_idx = 0;
       entity_idx < game->entity_count;
       ++entity_idx)
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
        // NOTE(cj): Drawing/Animation update
        //
        if (desired_move_x || desired_move_y)
        {
          Animation_Frame walk_frame = tick_animation(&game->walk_animation, game_update_secs).frame;
          game_add_tex_clipped(&game->quads,
                               entity->p, (v3f){64,64,0},
                               walk_frame.clip_p, walk_frame.clip_dims,
                               (v4f){1,1,1,1},
                               entity->last_face_dir);
        }
        else
        {
          game_add_tex_clipped(&game->quads,
                               entity->p, (v3f){64,64,0},
                               (v2f){0,0}, (v2f){16,16},
                               (v4f){1,1,1,1},
                               entity->last_face_dir);
        }
        
        // TODO(cj): Should Attacks be generated entities?
        //
        // NOTE(cj): Update Attacks
        //
        for (u64 attack_idx = 0;
             attack_idx < entity->attack_count;
             ++attack_idx)
        {
          Attack *attack = entity->attacks + attack_idx;
          if (attack->current_secs >= attack->interval_secs)
          {
            Animation_Tick_Result tick_result = tick_animation(&attack->animation, game_update_secs);
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
            
            for (u64 entity_to_collide_idx = 1;
                 entity_to_collide_idx< game->entity_count;
                 ++entity_to_collide_idx)
            {
              Entity *possible_collision = game->entities + entity_to_collide_idx;
              if (!!(possible_collision->flags & EntityFlag_Hostile))
              {
                f32 c_dist_x = absolute_value_f32(p.x - possible_collision->p.x);
                f32 c_dist_y = absolute_value_f32(p.y - possible_collision->p.y);
                f32 c_dist_z = absolute_value_f32(p.z - possible_collision->p.z);
                
                f32 r_add_x = dims.x + possible_collision->half_dims.x;
                f32 r_add_y = dims.y + possible_collision->half_dims.y;
                f32 r_add_z = dims.z + possible_collision->half_dims.z;
                
                b32 x_test = c_dist_x <= r_add_x;
                b32 y_test = c_dist_y <= r_add_y;
                b32 z_test = c_dist_z <= r_add_z;
                z_test;
                
                // TODO(cj): for now, this is delete entity of collided by attack.
                // Add a notion of HP!
                if (x_test && y_test)
                {
                  --game->entity_count;
                  if (entity_to_collide_idx != game->entity_count)
                  {
                    game->entities[game->entity_count] = game->entities[entity_to_collide_idx];
                  }
                }
              }
            }
            
            game_add_tex_clipped(&game->quads, p, dims,
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
      
      case EntityType_Skull:
      {
        f32 follow_speed = 32.0f;
        v3f to_player = v3f_sub_and_normalize_or_zero(player->p, entity->p);
        to_player.x *= follow_speed * game_update_secs;
        to_player.y *= follow_speed * game_update_secs;
        v3f_add_eq(&entity->p, to_player);
        
        game_add_rect(&game->quads,
                      entity->p, (v3f){64,64,0},
                      (v4f){1,0,1,1});
      } break;
      
      InvalidDefaultCase();
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
  
  prevent_dpi_scaling();
  
  timeBeginPeriod(1);
  
  LARGE_INTEGER w32_perf_frequency;
  QueryPerformanceFrequency(&w32_perf_frequency);
  
  DEVMODE dev_mode;
  EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dev_mode);
  u64 refresh_rate = dev_mode.dmDisplayFrequency;
  f32 seconds_per_frame = 1.0f / (f32)refresh_rate;
  
  WNDCLASS wnd_class =
  {
    .style = 0,
    .lpfnWndProc = &w32_window_proc,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = GetModuleHandleA(0),
    .hIcon = LoadIconA(0, IDI_APPLICATION),
    .hCursor = LoadCursorA(0, IDC_ARROW),
    .hbrBackground = GetStockObject(BLACK_BRUSH),
    .lpszMenuName = 0,
    .lpszClassName = "Game Project",
  };
  
  if (RegisterClassA(&wnd_class))
  {
    s32 width = 1280;
    s32 height = 720;
    g_application.window_width = width;
    g_application.window_height = height;
    
    dx11_create_device(&g_application);
    
    RECT w32_client_rect =
    {
      .left = 0,
      .top = 0,
      .right = width,
      .bottom = height,
    };
    
    AdjustWindowRect(&w32_client_rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    g_application.window = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP, wnd_class.lpszClassName, "Game", WS_OVERLAPPEDWINDOW,
                                           0, 0, w32_client_rect.right - w32_client_rect.left, w32_client_rect.bottom - w32_client_rect.top,
                                           0, 0, wnd_class.hInstance, &g_application);
    
    dx11_create_swap_chain(&g_application);
    dx11_create_game_main_state(&g_application);
    dx11_create_rasterizer_states(&g_application);
    dx11_create_sampler_states(&g_application);
    
    if (IsWindow(g_application.window))
    {
      ShowWindow(g_application.window, SW_SHOW);
      
      Game_State game = {0};
      game_init(&game, g_application.game_diffse_sheet_width, g_application.game_diffse_sheet_height);
      LARGE_INTEGER perf_counter_begin;
      while (1)
      {
        QueryPerformanceCounter(&perf_counter_begin);
        for (u32 key = 0; key < OS_Input_KeyType_Count; ++key)
        {
          g_application.input.key[key] &= ~(OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Released);
        }
        
        for (u32 button = 0; button < OS_Input_ButtonType_Count; ++button)
        {
          g_application.input.button[button] &= ~(OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Released);
        }
        
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE) != 0)
        {
          switch (message.message)
          {
            case WM_QUIT:
            {
              ExitProcess(0);
            } break;
            
            default:
            {
              TranslateMessage(&message);
              DispatchMessage(&message);
            } break;
          }
        }
        
        OS_Input *input = &g_application.input;
        if (OS_KeyReleased(input, OS_Input_KeyType_Escape))
        {
          ExitProcess(0);
        }
        
        game_update_and_render(&game, input, seconds_per_frame);
        
        D3D11_VIEWPORT viewport =
        {
          .Width = (f32)g_application.reso_width,
          .Height = (f32)g_application.reso_height,
          .TopLeftX = 0,
          .TopLeftY = 0,
          .MinDepth = 0,
          .MaxDepth = 1,
        };
        
        Entity *player = game.entities;
        //
        // // NOTE(cj): Cbuf0 - proj and cam
        //
        f32 half_reso_x = (f32)g_application.reso_width * 0.5f;
        f32 half_reso_y = (f32)g_application.reso_height * 0.5f;
        DX11_Game_CBuffer0 new_cbuf0 =
        {
          .proj = m44_make_orthographic_z01(-half_reso_x, half_reso_x, half_reso_y, -half_reso_y, -50.0f, 50.0f),
          .world_to_cam = 
          {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            -player->p.x, -player->p.y, -player->p.z, 1,
          }
        };
        
        D3D11_MAPPED_SUBRESOURCE mapped_subresource;
        ID3D11DeviceContext_Map(g_application.device_context, (ID3D11Resource *)g_application.cbuffer0_main, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
        CopyMemory(mapped_subresource.pData, &new_cbuf0, sizeof(new_cbuf0));
        ID3D11DeviceContext_Unmap(g_application.device_context, (ID3D11Resource *)g_application.cbuffer0_main, 0);
        
        f32 clear_colour[4] = {0}; 
        ID3D11DeviceContext_ClearRenderTargetView(g_application.device_context, g_application.render_target, clear_colour);
        
        ID3D11DeviceContext_IASetPrimitiveTopology(g_application.device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        
        ID3D11DeviceContext_VSSetShader(g_application.device_context, g_application.vertex_shader_main, 0, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(g_application.device_context, 0, 1, &g_application.cbuffer0_main);
        ID3D11DeviceContext_VSSetShaderResources(g_application.device_context, 0, 1, &g_application.sbuffer_view_main);
        
        ID3D11DeviceContext_RSSetViewports(g_application.device_context, 1, &viewport);
        
        ID3D11DeviceContext_PSSetShader(g_application.device_context, g_application.pixel_shader_main, 0, 0);
        ID3D11DeviceContext_PSSetSamplers(g_application.device_context, 0, 1, &g_application.sampler_point_all);
        ID3D11DeviceContext_PSSetConstantBuffers(g_application.device_context, 1, 1, &g_application.cbuffer1_main);
        ID3D11DeviceContext_PSSetShaderResources(g_application.device_context, 1, 1, &g_application.game_diffuse_sheet_view);
        
        ID3D11DeviceContext_OMSetBlendState(g_application.device_context, g_application.blend_blend, 0, 0xFFFFFFFF);
        ID3D11DeviceContext_OMSetRenderTargets(g_application.device_context, 1, &g_application.render_target, 0);
        
        Game_QuadArray game_quads = game.quads;
        if (game_quads.count)
        {
          ID3D11DeviceContext_Map(g_application.device_context, (ID3D11Resource *)g_application.sbuffer_main, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
          CopyMemory(mapped_subresource.pData, game_quads.quads, sizeof(Game_Quad) * Game_MaxQuads);
          ID3D11DeviceContext_Unmap(g_application.device_context, (ID3D11Resource *)g_application.sbuffer_main, 0);
          ID3D11DeviceContext_DrawInstanced(g_application.device_context, 4, (UINT)game_quads.count, 0, 0);
        }
        
        ID3D11DeviceContext_ClearState(g_application.device_context);
        IDXGISwapChain1_Present(g_application.swap_chain, 1, 0);
        
        game.quads.count = 0;
        
        LARGE_INTEGER perf_counter_end;
        QueryPerformanceCounter(&perf_counter_end);
        
        f32 seconds_used_for_work = (f32)(perf_counter_end.QuadPart - perf_counter_begin.QuadPart) / (f32)(w32_perf_frequency.QuadPart);
        
        if (seconds_used_for_work < seconds_per_frame)
        {
          Sleep((u32)((seconds_per_frame - seconds_used_for_work) * 1000.0f));
        }
      }
    }
  }
  
  ExitProcess(0);
}
