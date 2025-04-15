cbuffer VShaderCBuffer0 : register(b0)
{
  float4x4 proj;
};

struct UI_Quad
{
	float2 p;
  float2 dims;
  
  float4 vertex_colours[4];
  float vertex_roundness;
  float border_thickness;
  
  float2 shadow_offset;
  float2 shadow_dim_offset;
  float4 shadow_colours[4];
  float shadow_roundness;
  float shadow_smoothness;
  
  float2 uvs[4];
  // 1 -> diffuse sheet
  // 2 -> small font 
  // anything else -> no texture
  uint tex_id;
};

struct VertexShader_Output
{
  float4 p          : SV_Position;
  float4 colour     : Colour_Mod;
  float2 uv         : Tex_UV;
	uint   tex_id     : Tex_ID;
};

StructuredBuffer<UI_Quad> g_instances : register(t0);
Texture2D<float4> g_texture1          : register(t1);
Texture2D<float4> g_texture2          : register(t2);

SamplerState g_sampler0 : register(s0);

static const float2 quad_vertices[] = 
{
	float2(0, 0),
	float2(0, 1),
	float2(1, 0),
	float2(1, 1),
};

VertexShader_Output vs_main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
	VertexShader_Output result = (VertexShader_Output)0;
	UI_Quad quad = g_instances[iid];
	
	float4 p = float4(quad_vertices[vid] * quad.dims + quad.p, 0.0f, 1.0f);
	result.p = mul(proj, p);
	result.colour = quad.vertex_colours[vid];
	result.uv = quad.uvs[vid];
	result.tex_id = quad.tex_id;
	return(result);
}

float4 ps_main(VertexShader_Output ps_inp) : SV_Target
{
	float4 final_colour = ps_inp.colour;
	switch (ps_inp.tex_id)
	{
		case 1:
		{
			final_colour *= g_texture1.Sample(g_sampler0, ps_inp.uv);
		} break;

		case 2:
		{
			final_colour *= g_texture2.Sample(g_sampler0, ps_inp.uv);
		} break;
	}

	if (final_colour.a == 0)
	{
		discard;
	}
	return(final_colour);
}