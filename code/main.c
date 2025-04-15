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
#include "prng.h"
#include "mathematical_objects.h"
#include "game.h"

#include "mathematical_objects.c"
#include "prng.c"

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
  m44 proj;
} DX11_UI_CBuffer0;

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
  
  s32 game_diffse_sheet_width;
  s32 game_diffse_sheet_height;
  ID3D11ShaderResourceView *game_diffuse_sheet_view;
  s32 font_atlas_sheet_width;
  s32 font_atlas_sheet_height;
  ID3D11ShaderResourceView *font_atlas_sheet_view;
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
  }
}

function void
dx11_create_ui_main_state(Application_State *app)
{
  dx11_create_vertex_and_pixel_shader(app->device, 
                                      L"..\\code\\shaders\\ui-shader.hlsl", "vs_main", "ps_main",
                                      &app->vertex_shader_ui, &app->pixel_shader_ui);
  
  D3D11_BUFFER_DESC cbuf_desc =
  {
    .ByteWidth = sizeof(DX11_UI_CBuffer0),
    .Usage = D3D11_USAGE_DYNAMIC,
    .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags = 0,
    .StructureByteStride = 0,
  };
  
  if (SUCCEEDED(ID3D11Device_CreateBuffer(app->device, &cbuf_desc, 0, &app->cbuffer0_ui)))
  {
    D3D11_BUFFER_DESC sbuffer_desc =
    {
      .ByteWidth = sizeof(UI_Quad) * UI_MaxQuads,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = sizeof(UI_Quad),
    };
    
    if (SUCCEEDED(ID3D11Device_CreateBuffer(app->device, &sbuffer_desc, 0, &app->sbuffer_ui)))
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC sbuffer_srv_desc =
      {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
        .Buffer = { .NumElements = UI_MaxQuads }
      };
      
      ID3D11Device_CreateShaderResourceView(app->device, (ID3D11Resource *)app->sbuffer_ui, &sbuffer_srv_desc, &app->sbuffer_view_ui);
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
  
  raster_desc.FillMode = D3D11_FILL_WIREFRAME;
  ID3D11Device_CreateRasterizerState(app->device, &raster_desc, &app->rasterizer_wire_no_cull_ccw);
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

function void
dx11_create_font_atlas(Application_State *app, Game_Memory *memory)
{
  Renderer_State *renderer = &memory->renderer;
  // Questions to answer:
  // 1. Font
  //   - a font is a collection of characters and symbols that share a common design, including look, style, and serifs. But,
  //     we need to distinguish Font Family and Font Faces and be precise on our definitions.
  //  
  //   a. Font Face (or Type Face) - is a collection of characters and symbols that share a common design, including look, style, and serifs.
  //      > For style, it is the inclination (think of it as a skew) to the vertical axis. (Regular Vs. Italic)
  //      > For weight, it defines the boldness of the character.
  //      > For serif, it is the short cross line at the ends of an unconnected stroke
  //
  //   b. Font Family - A group of Font Faces that share basic design characteristics.
  // 
  //   Hence, we see that if the Font Family name is "Palatino.ttf", then "Palatino Regular" and "Palatino Italic" are Font Faces of the
  //   Font Family "Palatino". A font file containing more than one font face is called "Font Collection".
  //
  //   Character Images in a Font Face are called "Glyphs". Each glyph is stored as a bitmap, a vector representation, or SDFs.
  //   These glyphs are typically accessed as a "Glyph Index" in the Font Face. Font Faces one or more tables called "Character Maps". 
  //   These charmaps are used to map codepoint for a given encoding to glyph indices.
  //
  //   Ways to display text:
  //   1. Raster Font: created by a sequence of pixels (pels or dots) that form a character called "Raster Pattern". The number of dots per inch(DPI)
  //      a printer generates is the print resolution. For instance, if the DPI is 240, this means every physical inch is equivalent to 240 pixels.
  //      This implies that the higher the DPI is, the more resolute the font is.
  //
  //   2. Vectorial Representation: A glyph is creating with a collection of line vertices that define the line segments that the system uses
  //      to draw a character or symbol in the font. Hence, we can give this vertex information to a triangulator to draw the glyph.
  //      Note that glyphs in this representation are scalable; That is, they're device independent.
  //
  //   3. TrueType/OpenType: a glyph is a collection of line and curve commands as well as a collection of hints. Said another way,
  //      glyphs in this representation are created through Mathematical Formulas rather the pixels. These formulas are then used by
  //      a Glyph Rasterizer to create bitmap characters that depends on two variables: Resolution (in DPI) and Point Size (we discuss sizing soon).
  //      This implies that fonts can be rasterized at MANY RESOLUTIONS and POINT SIZES. "Hints" are contained in TrueType/OpenType to
  //      make sure that typographic characteristics of the typeface are maintained in a consistent manner throughout all printed characters. That is,
  //      it adjusts the length of lines and shapes of curves to draw glyphs.
  //
  //   "Sizes":
  //     Output devices have varying resolutions. Hence, it is common to describe output device's characteristics with two numbers expressed in Dots Per Inch (DPI).
  //     All fonts are measured in points (the vertical size of the font). One inch is equivalent to 72 points. In other words, 1 point(pt) is equivalent to 1/72 inch.
  //     Measuring text size is a problem because because pixels are not all the same size. Hence, the size of the pixel depends on the RESOLUTION (in DPI) and
  //     the PHYSICAL SIZE OF THE MONITOR. Hence, fonts instead are measured in "Logical Units". Hence, a 72 pt (1 in) font is one logical inch tall. This logical inch
  //     is then converted to pixels. !!IMPORTANT!! for windows: one logical inch is 96 DPI. Hence, given the POINT SIZE of the font, the pixel size is:
  //                                                                     pixel_size = pt_size * 96/72.
  //     
  //     When designing a glyph, the designer uses an imaginary square called the EM square (design units maybe?). It can be thought as a tablet on which characters are drawn.
  //     It is used to scale outlines to a given text dimension. Another acronym used for the pixel size is ppem (pixel per EM).
  // 
  // 2. ClearType
  //   - Created by Microsoft to improve "readability" of text on LCDs.
  //     Words on computer screen looks almost as sharp and clear as those printed on a piece of paper.
  //     LCD monitor pixels have "subpixels" that have three vertical stripes of colour. Because of this,
  //     ClearType can now display features of text as small as a fraction of a pixel in width.
  //               
  //     Hence, a pixel of an LCD screen has three subpixel stripes: red, green, and blue. Zooming out,
  //     the human brain interprets these combinations as a single coloured pixel. For instance, if you
  //     see a white pixel, zooming in, you will actually see a fully lit red, green, and blue subpixels.
  //
  //     ClearType is a form of sub-pixel font rendering that draws text using a pixel's red-green-blue (RGB) components separately
  //     instead of using the entire pixel. When the pixel is used in this way, horizontal resolution theoretically increases 300 percent.
  //
  // TODO(cj): We might want NONANTIALIASED_QUALITY.
  
  // TODO(cj): Checkout
  // - https://learn.microsoft.com/en-us/windows/win32/gdi/about-text-output
  // - https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gettextmetrics
  // - https://learn.microsoft.com/en-us/windows/win32/gdi/using-the-font-and-text-output-functions
  s32 point_size = 24;
  AddFontResourceExA("..\\res\\fonts\\Pixelify_Sans\\PixelifySans-VariableFont_wght.ttf", FR_PRIVATE, 0);
  app->font = CreateFontA(-(point_size * 96)/72, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                          DEFAULT_PITCH | FF_DONTCARE, "Pixelify Sans");
  
  if (app->font)
  {
#define MaxBitmapSize 512
    s32 bitmap_width = MaxBitmapSize;
    s32 bitmap_height = MaxBitmapSize;
    
    HDC dc = CreateCompatibleDC(0);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, bitmap_width, bitmap_height);
    
    RECT r;
    r.top = 0;
    r.left = 0;
    r.right = bitmap_width;
    r.bottom = bitmap_height;
    SelectObject(dc, bitmap);
    FillRect(dc, &r, GetStockObject(BLACK_BRUSH));
    SelectObject(dc, app->font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255,255,255));
    
    TEXTMETRIC text_metrics;
    GetTextMetrics(dc, &text_metrics);
    
    s32 gap = 4;
    s32 pen_x = gap;
    s32 pen_y = gap;
    SetTextAlign(dc, TA_TOP|TA_LEFT);
    for (u8 codepoint = 32; codepoint < 128; ++codepoint)
    {
      SIZE glyph_dims;
      char c = (char)codepoint;
      GetTextExtentPoint32A(dc, &c, 1, &glyph_dims);
      glyph_dims.cx = glyph_dims.cx;
      
      if ((pen_x + glyph_dims.cx + gap) >= bitmap_width)
      {
        pen_x = gap;
        pen_y += text_metrics.tmHeight + gap;
      }
      
      // https://learn.microsoft.com/en-us/windows/win32/gdi/character-widths
      ABCFLOAT abc;
      GetCharABCWidthsFloatA(dc, codepoint, codepoint, &abc);
      f32 advance_x = (abc.abcfA + abc.abcfB + abc.abcfC);
      
#if 0
      GLYPHMETRICS metrics = {0};
      MAT2 mat =
      {
        { 0, 1 }, { 0, 0 },
        { 0, 0 }, { 0, 1 },
      };
      GetGlyphOutlineA(dc, codepoint, GGO_METRICS, &metrics, 0, 0, &mat);
#endif
      // is this correcT???????????????? I am not convinced............................
      //f32 advance_x;
      //GetCharWidthFloatA(dc, codepoint, codepoint, &advance_x);
      //advance_x = (advance_x);
      
      TextOut(dc, pen_x, pen_y, &c, 1);
      
      renderer->font.glyphs[codepoint].advance = advance_x;
      renderer->font.glyphs[codepoint].clip_x = (f32)pen_x;
      renderer->font.glyphs[codepoint].clip_y = (f32)pen_y;
      renderer->font.glyphs[codepoint].clip_width = (f32)(glyph_dims.cx);
      renderer->font.glyphs[codepoint].clip_height = (f32)(glyph_dims.cy);
      renderer->font.glyphs[codepoint].x_offset = 0;
      
      pen_x += glyph_dims.cx + gap;
    }
    
    BITMAPINFO bitmap_info = {0};
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = bitmap_width;
    bitmap_info.bmiHeader.biHeight = -bitmap_height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    Temporary_Memory temp_mem = begin_temporary_memory(memory->temporary);
    u64 bitmap_area = bitmap_width*bitmap_height*4;
    u8 *source_buffer = m_arena_push(temp_mem.arena, bitmap_area);
    u8 *dest_buffer = m_arena_push(temp_mem.arena, bitmap_area);
    GetDIBits(dc, bitmap, 0, bitmap_height, source_buffer, &bitmap_info, DIB_RGB_COLORS);
    
    u8 *src = source_buffer;
    u8 *dest = dest_buffer;
    for (s32 y_pix = 0; y_pix < bitmap_height; ++y_pix)
    {
      u8 *dest_row = dest;
      u8 *src_row = src;
      for (s32 x_pix = 0; x_pix < bitmap_width; ++x_pix)
      {
        *dest_row++ = src_row[0];
        *dest_row++ = src_row[1];
        *dest_row++ = src_row[2];
        *dest_row++ = src_row[0];
        src_row += 4;
      }
      
      dest += bitmap_width * 4;
      src += bitmap_width * 4;
    }
    
    D3D11_TEXTURE2D_DESC atlas_desc =
    {
      .Width = bitmap_width,
      .Height = bitmap_height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc = { 1, 0 },
      .Usage = D3D11_USAGE_IMMUTABLE,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = 0,
      .MiscFlags = 0,
    };
    
    D3D11_SUBRESOURCE_DATA atlas_subrec =
    {
      .pSysMem = dest_buffer,
      .SysMemPitch = bitmap_width*4,
    };
    
    ID3D11Texture2D *atlas_font_tex;
    
    if (SUCCEEDED(ID3D11Device_CreateTexture2D(app->device, &atlas_desc, &atlas_subrec, &atlas_font_tex)))
    {
      ID3D11Device_CreateShaderResourceView(app->device, (ID3D11Resource *)atlas_font_tex, 0, &app->font_atlas_sheet_view);
      app->font_atlas_sheet_width = bitmap_width;
      app->font_atlas_sheet_height = bitmap_height;
      ID3D11Texture2D_Release(atlas_font_tex);
    }
    else
    {
      Assert(!"Log Soon");
    }
    
    end_temporary_memory(temp_mem);
    DeleteObject(bitmap);
    DeleteDC(dc);
  }
  else
  {
    Assert(!"Log Soon");
  }
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

inline function Temporary_Memory
begin_temporary_memory(M_Arena *arena)
{
  Assert(!!arena);
  Temporary_Memory result;
  result.arena = arena;
  result.start_stack_ptr = arena->stack_ptr;
  return(result);
}

inline function void
end_temporary_memory(Temporary_Memory temp)
{
  Assert(!!temp.arena);
  Assert(temp.arena->stack_ptr >= temp.start_stack_ptr);
  m_arena_pop(temp.arena, temp.arena->stack_ptr - temp.start_stack_ptr);
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

inline function UI_Quad *
ui_acquire_quad(UI_QuadArray *quads)
{
  Assert(quads->count < quads->capacity);
  UI_Quad *result = quads->quads + quads->count++;
  return(result);
}

function UI_Quad *
ui_add_quad_per_vertex_colours(UI_QuadArray *quads, v2f p, v2f dims,
                               v4f top_left_c, v4f bottom_left_c,
                               v4f top_right_c, v4f bottom_right_c)
{
  UI_Quad *result = ui_acquire_quad(quads);
  result->p = p;
  result->dims = dims;
  result->vertex_colours[0] = top_left_c;
  result->vertex_colours[1] = bottom_left_c;
  result->vertex_colours[2] = top_right_c;
  result->vertex_colours[3] = bottom_right_c;
  result->tex_id = 0;
  return(result);
}

inline function UI_Quad *
ui_add_tex(UI_QuadArray *quads, u32 tex_id, v2f p, v2f dims, v4f colour)
{
  UI_Quad *result = ui_add_quad_per_vertex_colours(quads, p, dims, colour, colour, colour, colour);
  result->uvs[0] = (v2f){ 0.0f, 0.0f };
  result->uvs[1] = (v2f){ 0.0f, 1.0f };
  result->uvs[2] = (v2f){ 1.0f, 0.0f };
  result->uvs[3] = (v2f){ 1.0f, 1.0f };
  result->tex_id = tex_id;
  return(result);
}

#if 0
// TODO(cj): Remove this soon. This is temporary. This should only be in the 
// UI render pass (which we havent implemented yet).
function void
ui_draw_textf(Renderer_State *renderer, v2f p, String_U8_Const str, v4f colour)
{
  v2f pen_p = p;
  ForLoopU64(char_idx, str.cap)
  {
    u8 char_val = str.s[char_idx];
    Assert((char_val >= 32) && (char_val < 128));
    
    Glyph_Data glyph = renderer->glyphs[char_val];
    if (char_val != ' ')
    {
      // TODO(cj): In the UI shader, the Rectangles will have Top Left as an origin.
      // Hence, remove this offset. I was pulling my hair, assuming that the RECT WAS
      // TOPLEFT ORIGIN, BUT CENTER ORIGIN IS THE GAME RECT. REMOVING THIS OFFSET WILL
      // MESS UP THE TEXT RENDERER. TOOK ME 3 HOURS TO FIGURE THIS OUT. THIS SINGLE LINE.
      // SO REMOVE THIS AND GO TO TOP LEFT FOR UI!!!!!!!!!!!!!!!!!!!!!
      v3f glyph_p = { pen_p.x + glyph.clip_width*0.5f, pen_p.y + glyph.clip_height*0.5f, 0.0f };
      v3f glyph_dims = { glyph.clip_width, glyph.clip_height, 0.0f };
      v2f glyph_clip_p = { glyph.clip_x, glyph.clip_y };
      ui_add_tex_clipped(&renderer->glyph_quads, glyph_p,
                         glyph_dims, glyph_clip_p,
                         glyph_dims.xy, colour, 0);
    }
    
    pen_p.x += glyph.advance;
  }
}
#endif

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
draw_health_bar(Game_QuadArray *quads, Entity *entity)
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
  Renderer_State *renderer = &memory->renderer;
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
        
        f32 camera_width_half = (f32)g_application.reso_width * 0.5f;
        f32 camera_height_half = (f32)g_application.reso_height * 0.5f;
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
  
  //ui_draw_textf(renderer->ui_quads, &renderer->font, (v2f){0,0}, str8("Hello, World! Ready for text rendering!"), (v4f){1,1,1,1});
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
    
    Game_Memory memory = {0};
    memory.arena = m_arena_reserve(MB(2));
    memory.temporary = m_arena_reserve(MB(8));
    
    dx11_create_swap_chain(&g_application);
    dx11_create_game_main_state(&g_application);
    dx11_create_ui_main_state(&g_application);
    dx11_create_rasterizer_states(&g_application);
    dx11_create_sampler_states(&g_application);
    dx11_create_font_atlas(&g_application, &memory);
    
    memory.renderer.filled_quads = (Game_QuadArray)
    {
      .quads = M_Arena_PushArray(memory.arena, Game_Quad, Game_MaxQuads),
      .capacity = Game_MaxQuads,
      .count = 0,
      .tex_width = g_application.game_diffse_sheet_width,
      .tex_height = g_application.game_diffse_sheet_height,
    };
    
    //
    // NOTE(cj): init debug stuff
    //
#if defined(DR_DEBUG)
    memory.renderer.wire_quads = (Game_QuadArray)
    {
      .quads = M_Arena_PushArray(memory.arena, Game_Quad, Game_MaxQuads),
      .capacity = Game_MaxQuads,
      .count = 0,
      .tex_width = g_application.game_diffse_sheet_width,
      .tex_height = g_application.game_diffse_sheet_height,
    };
#endif
    
    memory.renderer.ui_quads = (UI_QuadArray)
    {
      .quads = M_Arena_PushArray(memory.arena, UI_Quad, UI_MaxQuads),
      .capacity = UI_MaxQuads,
      .count = 0,
    };
    
    if (IsWindow(g_application.window))
    {
      ShowWindow(g_application.window, SW_SHOW);
      
      Game_State game = {0};
      game_init(&game);
      
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
        
        game_update_and_render(&game, input, &memory, seconds_per_frame);
        
        ui_add_quad_per_vertex_colours(&memory.renderer.ui_quads, (v2f){0,0},
                                       (v2f){256, 256}, (v4f){1,0,0,1},
                                       (v4f){0,1,0,1}, (v4f){0,0,1,1},
                                       (v4f){1,0,1,1});
        
        ui_add_tex(&memory.renderer.ui_quads, 2, (v2f){ 700, 0 }, (v2f){512,512}, (v4f){1,1,1,1});
        
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
            game_add_rect(&memory.renderer.wire_quads, entity->p, entity->dims, (v4f){ 0, 0, 1, 1 });
          }
        }
#endif
        
        D3D11_VIEWPORT viewport =
        {
          .Width = (f32)g_application.reso_width,
          .Height = (f32)g_application.reso_height,
          .TopLeftX = 0,
          .TopLeftY = 0,
          .MinDepth = 0,
          .MaxDepth = 1,
        };
        
        f32 clear_colour[4] = {0}; 
        ID3D11DeviceContext_ClearRenderTargetView(g_application.device_context, g_application.render_target, clear_colour);
        
        // Game Pass
        {
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
          
          ID3D11DeviceContext_IASetPrimitiveTopology(g_application.device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
          
          D3D11_MAPPED_SUBRESOURCE mapped_subresource;
          ID3D11DeviceContext_Map(g_application.device_context, (ID3D11Resource *)g_application.cbuffer0_main, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
          CopyMemory(mapped_subresource.pData, &new_cbuf0, sizeof(new_cbuf0));
          ID3D11DeviceContext_Unmap(g_application.device_context, (ID3D11Resource *)g_application.cbuffer0_main, 0);
          
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
          
          Game_QuadArray game_quads = memory.renderer.filled_quads;
          if (game_quads.count)
          {
            ID3D11DeviceContext_RSSetState(g_application.device_context, g_application.rasterizer_fill_no_cull_ccw);
            ID3D11DeviceContext_Map(g_application.device_context, (ID3D11Resource *)g_application.sbuffer_main, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
            CopyMemory(mapped_subresource.pData, game_quads.quads, sizeof(Game_Quad) * Game_MaxQuads);
            ID3D11DeviceContext_Unmap(g_application.device_context, (ID3D11Resource *)g_application.sbuffer_main, 0);
            ID3D11DeviceContext_DrawInstanced(g_application.device_context, 4, (UINT)game_quads.count, 0, 0);
          }
          memory.renderer.filled_quads.count = 0;
          
#if defined(DR_DEBUG)
          game_quads = memory.renderer.wire_quads;
          if (game_quads.count)
          {
            ID3D11DeviceContext_RSSetState(g_application.device_context, g_application.rasterizer_wire_no_cull_ccw);
            ID3D11DeviceContext_Map(g_application.device_context, (ID3D11Resource *)g_application.sbuffer_main, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
            CopyMemory(mapped_subresource.pData, game_quads.quads, sizeof(Game_Quad) * Game_MaxQuads);
            ID3D11DeviceContext_Unmap(g_application.device_context, (ID3D11Resource *)g_application.sbuffer_main, 0);
            ID3D11DeviceContext_DrawInstanced(g_application.device_context, 4, (UINT)game_quads.count, 0, 0);
          }
          
          memory.renderer.wire_quads.count = 0;
#endif
          ID3D11DeviceContext_ClearState(g_application.device_context);
        }
        
        // UI Pass
        {
          DX11_UI_CBuffer0 new_cbuf0 =
          {
            .proj = m44_make_orthographic_z01(0, (f32)g_application.reso_width, 0, (f32)g_application.reso_height, 0.0f, 1.0f),
          };
          
          ID3D11DeviceContext_IASetPrimitiveTopology(g_application.device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
          D3D11_MAPPED_SUBRESOURCE mapped_subresource;
          ID3D11DeviceContext_Map(g_application.device_context, (ID3D11Resource *)g_application.cbuffer0_ui, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
          CopyMemory(mapped_subresource.pData, &new_cbuf0, sizeof(new_cbuf0));
          ID3D11DeviceContext_Unmap(g_application.device_context, (ID3D11Resource *)g_application.cbuffer0_ui, 0);
          
          ID3D11DeviceContext_VSSetShader(g_application.device_context, g_application.vertex_shader_ui, 0, 0);
          ID3D11DeviceContext_VSSetConstantBuffers(g_application.device_context, 0, 1, &g_application.cbuffer0_ui);
          ID3D11DeviceContext_VSSetShaderResources(g_application.device_context, 0, 1, &g_application.sbuffer_view_ui);
          
          ID3D11DeviceContext_RSSetViewports(g_application.device_context, 1, &viewport);
          
          ID3D11DeviceContext_PSSetShader(g_application.device_context, g_application.pixel_shader_ui, 0, 0);
          ID3D11DeviceContext_PSSetSamplers(g_application.device_context, 0, 1, &g_application.sampler_point_all);
          ID3D11DeviceContext_PSSetShaderResources(g_application.device_context, 1, 1, &g_application.game_diffuse_sheet_view);
          ID3D11DeviceContext_PSSetShaderResources(g_application.device_context, 2, 1, &g_application.font_atlas_sheet_view);
          
          ID3D11DeviceContext_OMSetBlendState(g_application.device_context, g_application.blend_blend, 0, 0xFFFFFFFF);
          ID3D11DeviceContext_OMSetRenderTargets(g_application.device_context, 1, &g_application.render_target, 0);
          
          if (memory.renderer.ui_quads.count)
          {
            ID3D11DeviceContext_RSSetState(g_application.device_context, g_application.rasterizer_fill_no_cull_ccw);
            ID3D11DeviceContext_Map(g_application.device_context, (ID3D11Resource *)g_application.sbuffer_ui, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
            CopyMemory(mapped_subresource.pData, memory.renderer.ui_quads.quads, sizeof(UI_Quad) * UI_MaxQuads);
            ID3D11DeviceContext_Unmap(g_application.device_context, (ID3D11Resource *)g_application.sbuffer_ui, 0);
            ID3D11DeviceContext_DrawInstanced(g_application.device_context, 4, (UINT)memory.renderer.ui_quads.count, 0, 0);
          }
          
          memory.renderer.ui_quads.count = 0;
          ID3D11DeviceContext_ClearState(g_application.device_context);
        }
        
        IDXGISwapChain1_Present(g_application.swap_chain, 1, 0);
        
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