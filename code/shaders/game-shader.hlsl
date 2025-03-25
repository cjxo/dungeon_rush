cbuffer ConstantStore0 : register(b0)
{
  float4x4 proj;
};

struct Quad
{
  float3 p;
  float3 dims;  
  float4 colour;
  float2 uvs[4];
};

struct VertexShader_Output
{
  float4 p          : SV_Position;
	float3 world_p    : World_P;
  float4 colour     : Colour_Mod;
  float2 uv         : Tex_UV;
};

// texture registers
StructuredBuffer<Quad>       g_quad_instances           : register(t0);
Texture2D<float4>            g_sheet_diffuse            : register(t1);

SamplerState                 g_pointsampler             : register(s0);

static float3 g_quad_vertices[] =
{
  float3(-0.5f, -0.5f, 0.0f),
  float3(-0.5f, +0.5f, 0.0f),
  float3(+0.5f, -0.5f, 0.0f),
  float3(+0.5f, +0.5f, 0.0f),
};

static float2 g_quad_uv[] =
{
  float2(0, 1),
  float2(0, 0),
  float2(1, 1),
  float2(1, 0),
};

VertexShader_Output
vs_main(uint iid : SV_InstanceID, uint vid : SV_VertexID)
{
	VertexShader_Output result = (VertexShader_Output)0;

  Quad instance   = g_quad_instances[iid];
  float3 vertex   = (g_quad_vertices[vid] * instance.dims + instance.p);

	result.p        = mul(proj, float4(vertex, 1.0f));
	result.world_p  = vertex;
  result.colour   = instance.colour;
	result.uv       = instance.uvs[vid];
	return(result);
}

float4
ps_main(VertexShader_Output inp) : SV_Target
{
	float4 sample = inp.colour;
  float4 texel = g_sheet_diffuse.Sample(g_pointsampler, inp.uv);
	sample *= texel;
	return(sample);
}