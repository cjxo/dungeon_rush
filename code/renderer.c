typedef struct
{
  m44 proj;
  m44 world_to_cam;
} DX11_Game_CBuffer0;

typedef struct
{
  m44 proj;
} DX11_UI_CBuffer0;


#if defined(GAME_DEBUG)
# define DX11_ShaderCompileFlags (D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR|D3DCOMPILE_SKIP_OPTIMIZATION|D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS)
#else
# define DX11_ShaderCompileFlags (D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR|D3DCOMPILE_OPTIMIZATION_LEVEL3)
#endif

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
dx11_create_device(R_State *state)
{
  UINT flags = (D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT);
#if defined(DR_DEBUG)
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif        
  D3D_FEATURE_LEVEL feature_levels = D3D_FEATURE_LEVEL_11_0;
  if(SUCCEEDED(D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, &feature_levels, 1,
                                 D3D11_SDK_VERSION, &state->device, 0, &state->device_context)))
  {
#if defined(DR_DEBUG)
    ID3D11InfoQueue *dx11_infoq;
    IDXGIInfoQueue *dxgi_infoq;
    
    if (SUCCEEDED(ID3D11Device_QueryInterface(state->device, &IID_ID3D11InfoQueue, &dx11_infoq)))
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
  else
  {
    Assert(!"TODO: Logging");
  }
}

function void
dx11_create_swap_chain(R_State *state, OS_Window window)
{
  IDXGIDevice2 *dxgi_device;
  IDXGIAdapter *dxgi_adapter;
  IDXGIFactory2 *dxgi_factory;
  ID3D11Texture2D *backbuffer_tex;
  
  if (SUCCEEDED(ID3D11Device_QueryInterface(state->device, &IID_IDXGIDevice2, &dxgi_device)))
  {
    if (SUCCEEDED(IDXGIDevice2_GetAdapter(dxgi_device, &dxgi_adapter)))
    {
      if (SUCCEEDED(IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, &dxgi_factory)))
      {
        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {0};
        swap_chain_desc.Width              = window.client_width;
        swap_chain_desc.Height             = window.client_height;
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
        
        if (SUCCEEDED(IDXGIFactory2_CreateSwapChainForHwnd(dxgi_factory, (IUnknown *)state->device, window.handle,
                                                           &swap_chain_desc, 0, 0,
                                                           &state->swap_chain)))
        {
          IDXGIFactory2_MakeWindowAssociation(dxgi_factory, window.handle, DXGI_MWA_NO_ALT_ENTER);
          
          if (SUCCEEDED(IDXGISwapChain1_GetBuffer(state->swap_chain, 0, &IID_ID3D11Texture2D, &backbuffer_tex)))
          {
            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { 0 };
            rtv_desc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtv_desc.ViewDimension       = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice  = 0;
            ID3D11Device_CreateRenderTargetView(state->device, (ID3D11Resource *)backbuffer_tex,
                                                &rtv_desc, &state->render_target);
            
            state->reso_width = window.client_width;
            state->reso_height = window.client_height;
            
            ID3D11Texture2D_Release(backbuffer_tex);
          }
          else
          {
            Assert(!"TODO: Logging");
          }
        }
        else
        {
          Assert(!"TODO: Logging");
        }
        
        IDXGIFactory2_Release(dxgi_factory);
      }
      else
      {
        Assert(!"TODO: Logging");
      }
      
      IDXGIAdapter_Release(dxgi_adapter);
    }
    else
    {
      Assert(!"TODO: Logging");
    }
    
    IDXGIDevice2_Release(dxgi_device);
  }
  else
  {
    Assert(!"TODO: Logging");
  }
}


function void
dx11_create_game_main_state(R_State *state)
{
  dx11_create_vertex_and_pixel_shader(state->device, 
                                      L"..\\code\\shaders\\game-shader.hlsl", "vs_main", "ps_main",
                                      &state->vertex_shader_main, &state->pixel_shader_main);
  
  D3D11_BUFFER_DESC cbuf_desc =
  {
    .ByteWidth = sizeof(DX11_Game_CBuffer0),
    .Usage = D3D11_USAGE_DYNAMIC,
    .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags = 0,
    .StructureByteStride = 0,
  };
  
  if (SUCCEEDED(ID3D11Device_CreateBuffer(state->device, &cbuf_desc, 0, &state->cbuffer0_main)))
  {
    D3D11_BUFFER_DESC sbuffer_desc =
    {
      .ByteWidth = sizeof(R_Game_Quad) * R_Game_MaxQuads,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = sizeof(R_Game_Quad),
    };
    
    if (SUCCEEDED(ID3D11Device_CreateBuffer(state->device, &sbuffer_desc, 0, &state->sbuffer_main)))
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC sbuffer_srv_desc =
      {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
        .Buffer = { .NumElements = R_Game_MaxQuads }
      };
      
      ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)state->sbuffer_main, &sbuffer_srv_desc, &state->sbuffer_view_main);
      
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
        ID3D11Device_CreateTexture2D(state->device, &texture_desc, &texture_data, &game_diffuse_sheet_tex);
        Assert(game_diffuse_sheet_tex != 0);
        if (game_diffuse_sheet_tex)
        {
          ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)game_diffuse_sheet_tex, 0, &state->game_diffuse_sheet_view);
          Assert(state->game_diffuse_sheet_view != 0);
          if (state->game_diffuse_sheet_view)
          {
            state->game_diffse_sheet_width = tex_width;
            state->game_diffse_sheet_height = tex_height;
          }
          else
          {
            // NOTE(cj): LOGGING
            Assert(!"TODO: Logging");
          }
          
          ID3D11Texture2D_Release(game_diffuse_sheet_tex);
        }
        else
        {
          // NOTE(cj): LOGGING
          Assert(!"TODO: Logging");
        }
        
        stbi_image_free(tex_data);
      }
      else
      {
        // NOTE(cj): LOGGING
        Assert(!"TODO: Logging");
      }
    }
    else
    {
      // NOTE(cj): LOGGING
      Assert(!"TODO: Logging");
    }
  }
}

function void
dx11_create_ui_main_state(R_State *state)
{
  dx11_create_vertex_and_pixel_shader(state->device, 
                                      L"..\\code\\shaders\\ui-shader.hlsl", "vs_main", "ps_main",
                                      &state->vertex_shader_ui, &state->pixel_shader_ui);
  
  D3D11_BUFFER_DESC cbuf_desc =
  {
    .ByteWidth = sizeof(DX11_UI_CBuffer0),
    .Usage = D3D11_USAGE_DYNAMIC,
    .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags = 0,
    .StructureByteStride = 0,
  };
  
  if (SUCCEEDED(ID3D11Device_CreateBuffer(state->device, &cbuf_desc, 0, &state->cbuffer0_ui)))
  {
    D3D11_BUFFER_DESC sbuffer_desc =
    {
      .ByteWidth = sizeof(R_UI_Quad) * R_UI_MaxQuads,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = sizeof(R_UI_Quad),
    };
    
    if (SUCCEEDED(ID3D11Device_CreateBuffer(state->device, &sbuffer_desc, 0, &state->sbuffer_ui)))
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC sbuffer_srv_desc =
      {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
        .Buffer = { .NumElements = R_UI_MaxQuads }
      };
      
      ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)state->sbuffer_ui, &sbuffer_srv_desc, &state->sbuffer_view_ui);
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
dx11_create_rasterizer_states(R_State *state)
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
  
  ID3D11Device_CreateRasterizerState(state->device, &raster_desc, &state->rasterizer_fill_no_cull_ccw);
  
  raster_desc.FillMode = D3D11_FILL_WIREFRAME;
  ID3D11Device_CreateRasterizerState(state->device, &raster_desc, &state->rasterizer_wire_no_cull_ccw);
}

function void
dx11_create_sampler_states(R_State *state)
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
  
  ID3D11Device_CreateSamplerState(state->device, &sam_desc, &state->sampler_point_all);
}

function void
dx11_create_blend_states(R_State *state)
{
  D3D11_BLEND_DESC blend_desc = {0};
  blend_desc.AlphaToCoverageEnable = FALSE;
  blend_desc.IndependentBlendEnable = FALSE;
  blend_desc.RenderTarget[0].BlendEnable = TRUE;
  blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
  blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  
  ID3D11Device_CreateBlendState(state->device, &blend_desc, &state->blend_blend);
}

function void
dx11_create_font_atlas(R_State *state, R_InputForRendering *renderer)
{
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
  s32 point_size = 16;
  AddFontResourceExA("..\\res\\fonts\\Pixelify_Sans\\PixelifySans-VariableFont_wght.ttf", FR_PRIVATE, 0);
  state->font = CreateFontA(-(point_size * 96)/72, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, "Pixelify Sans");
  
  if (state->font)
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
    SelectObject(dc, state->font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255,255,255));
    
    TEXTMETRIC text_metrics;
    GetTextMetrics(dc, &text_metrics);
    
    renderer->font.ascent = (f32)text_metrics.tmAscent;
    renderer->font.descent = (f32)text_metrics.tmDescent;
    
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
    
    Temporary_Memory temp_mem = begin_temporary_memory(get_transient_arena(0,0));
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
    
    if (SUCCEEDED(ID3D11Device_CreateTexture2D(state->device, &atlas_desc, &atlas_subrec, &atlas_font_tex)))
    {
      ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)atlas_font_tex, 0, &state->font_atlas_sheet_view);
      state->font_atlas_sheet_width = bitmap_width;
      state->font_atlas_sheet_height = bitmap_height;
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

function void
r_init(R_State *state, OS_Window window, M_Arena *arena)
{
  dx11_create_device(state);
  dx11_create_swap_chain(state, window);
  dx11_create_game_main_state(state);
  dx11_create_ui_main_state(state);
  dx11_create_rasterizer_states(state);
  dx11_create_blend_states(state);
  dx11_create_sampler_states(state);
  dx11_create_font_atlas(state, &state->input_for_rendering);
  
  R_InputForRendering *renderer = &state->input_for_rendering;
  renderer->reso_width = state->reso_width;
  renderer->reso_height = state->reso_height;
  renderer->game_sheet = (R_Texture2D)
  {
    1, state->game_diffse_sheet_width, state->game_diffse_sheet_height,
  };
  
  renderer->font_sheet = (R_Texture2D)
  {
    2, state->font_atlas_sheet_width, state->font_atlas_sheet_height,
  };
  
  renderer->font.sheet = renderer->font_sheet;
  renderer->filled_quads = (R_Game_QuadArray)
  {
    .quads = M_Arena_PushArray(arena, R_Game_Quad, R_Game_MaxQuads),
    .capacity = R_Game_MaxQuads,
    .count = 0,
    .tex = renderer->game_sheet
  };
  
  //
  // NOTE(cj): init debug stuff
  //
#if defined(DR_DEBUG)
  renderer->wire_quads = (R_Game_QuadArray)
  {
    .quads = M_Arena_PushArray(arena, R_Game_Quad, R_Game_MaxQuads),
    .capacity = R_Game_MaxQuads,
    .count = 0,
    .tex = renderer->game_sheet,
  };
#endif
  
  renderer->ui_quads = (R_UI_QuadArray)
  {
    .quads = M_Arena_PushArray(arena, R_UI_Quad, R_UI_MaxQuads),
    .capacity = R_UI_MaxQuads,
    .count = 0,
  };
}

function void
r_submit_and_reset(R_State *state, v3f camera_p)
{
  D3D11_VIEWPORT viewport =
  {
    .Width = (f32)state->input_for_rendering.reso_width,
    .Height = (f32)state->input_for_rendering.reso_height,
    .TopLeftX = 0,
    .TopLeftY = 0,
    .MinDepth = 0,
    .MaxDepth = 1,
  };
  
  f32 clear_colour[4] = {0}; 
  ID3D11DeviceContext_ClearRenderTargetView(state->device_context, state->render_target, clear_colour);
  
  // Game Pass
  {
    //
    // // NOTE(cj): Cbuf0 - proj and cam
    //
    f32 half_reso_x = (f32)state->reso_width * 0.5f;
    f32 half_reso_y = (f32)state->reso_height * 0.5f;
    DX11_Game_CBuffer0 new_cbuf0 =
    {
      .proj = m44_make_orthographic_z01(-half_reso_x, half_reso_x, half_reso_y, -half_reso_y, -50.0f, 50.0f),
      .world_to_cam = 
      {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        -camera_p.x, -camera_p.y, -camera_p.z, 1,
      }
    };
    
    ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
    D3D11_MAPPED_SUBRESOURCE mapped_subresource;
    ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->cbuffer0_main, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
    CopyMemory(mapped_subresource.pData, &new_cbuf0, sizeof(new_cbuf0));
    ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->cbuffer0_main, 0);
    
    ID3D11DeviceContext_VSSetShader(state->device_context, state->vertex_shader_main, 0, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(state->device_context, 0, 1, &state->cbuffer0_main);
    ID3D11DeviceContext_VSSetShaderResources(state->device_context, 0, 1, &state->sbuffer_view_main);
    
    ID3D11DeviceContext_RSSetViewports(state->device_context, 1, &viewport);
    
    ID3D11DeviceContext_PSSetShader(state->device_context, state->pixel_shader_main, 0, 0);
    ID3D11DeviceContext_PSSetSamplers(state->device_context, 0, 1, &state->sampler_point_all);
    ID3D11DeviceContext_PSSetConstantBuffers(state->device_context, 1, 1, &state->cbuffer1_main);
    ID3D11DeviceContext_PSSetShaderResources(state->device_context, 1, 1, &state->game_diffuse_sheet_view);
    
    ID3D11DeviceContext_OMSetBlendState(state->device_context, state->blend_blend, 0, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &state->render_target, 0);
    
    R_Game_QuadArray game_quads = state->input_for_rendering.filled_quads;
    if (game_quads.count)
    {
      ID3D11DeviceContext_RSSetState(state->device_context, state->rasterizer_fill_no_cull_ccw);
      ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->sbuffer_main, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
      CopyMemory(mapped_subresource.pData, game_quads.quads, sizeof(R_Game_Quad) * R_Game_MaxQuads);
      ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->sbuffer_main, 0);
      ID3D11DeviceContext_DrawInstanced(state->device_context, 4, (UINT)game_quads.count, 0, 0);
    }
    state->input_for_rendering.filled_quads.count = 0;
    
#if defined(DR_DEBUG)
    game_quads = state->input_for_rendering.wire_quads;
    if (game_quads.count)
    {
      ID3D11DeviceContext_RSSetState(state->device_context, state->rasterizer_wire_no_cull_ccw);
      ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->sbuffer_main, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
      CopyMemory(mapped_subresource.pData, game_quads.quads, sizeof(R_Game_Quad) * R_Game_MaxQuads);
      ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->sbuffer_main, 0);
      ID3D11DeviceContext_DrawInstanced(state->device_context, 4, (UINT)game_quads.count, 0, 0);
    }
    
    state->input_for_rendering.wire_quads.count = 0;
#endif
    ID3D11DeviceContext_ClearState(state->device_context);
  }
  
  // UI Pass
  {
    DX11_UI_CBuffer0 new_cbuf0 =
    {
      .proj = m44_make_orthographic_z01(0, (f32)state->reso_width, 0, (f32)state->reso_height, 0.0f, 1.0f),
    };
    
    ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    D3D11_MAPPED_SUBRESOURCE mapped_subresource;
    ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->cbuffer0_ui, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
    CopyMemory(mapped_subresource.pData, &new_cbuf0, sizeof(new_cbuf0));
    ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->cbuffer0_ui, 0);
    
    ID3D11DeviceContext_VSSetShader(state->device_context, state->vertex_shader_ui, 0, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(state->device_context, 0, 1, &state->cbuffer0_ui);
    ID3D11DeviceContext_VSSetShaderResources(state->device_context, 0, 1, &state->sbuffer_view_ui);
    
    ID3D11DeviceContext_RSSetViewports(state->device_context, 1, &viewport);
    
    ID3D11DeviceContext_PSSetShader(state->device_context, state->pixel_shader_ui, 0, 0);
    ID3D11DeviceContext_PSSetSamplers(state->device_context, 0, 1, &state->sampler_point_all);
    ID3D11DeviceContext_PSSetShaderResources(state->device_context, 1, 1, &state->game_diffuse_sheet_view);
    ID3D11DeviceContext_PSSetShaderResources(state->device_context, 2, 1, &state->font_atlas_sheet_view);
    
    ID3D11DeviceContext_OMSetBlendState(state->device_context, state->blend_blend, 0, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &state->render_target, 0);
    
    if (state->input_for_rendering.ui_quads.count)
    {
      ID3D11DeviceContext_RSSetState(state->device_context, state->rasterizer_fill_no_cull_ccw);
      ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->sbuffer_ui, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
      CopyMemory(mapped_subresource.pData, state->input_for_rendering.ui_quads.quads, sizeof(R_UI_Quad) * R_UI_MaxQuads);
      ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->sbuffer_ui, 0);
      ID3D11DeviceContext_DrawInstanced(state->device_context, 4, (UINT)state->input_for_rendering.ui_quads.count, 0, 0);
    }
    
    state->input_for_rendering.ui_quads.count = 0;
    ID3D11DeviceContext_ClearState(state->device_context);
  }
  
  IDXGISwapChain1_Present(state->swap_chain, 1, 0);
}