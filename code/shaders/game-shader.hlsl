cbuffer ConstantStore0 : register(b0)
{
  float4x4 proj;
};

struct Quad
{
  float3 p;
  float3 dims;  
  float4 colour;
};

struct VertexShader_Output
{
  float4 p          : SV_Position;
	float3 world_p    : World_P;
  float4 colour     : Colour_Mod;
  float2 uv         : Tex_UV;
};

StructuredBuffer<Quad>       g_quad_instances           : register(t0);
SamplerState                 g_pointsampler             : register(s0);

static float3 g_quad_vertices[] =
{
#if 0
  float3(+0.0f, +0.0f, 0.0f),
  float3(+0.0f, +1.0f, 0.0f),
  float3(+1.0f, +0.0f, 0.0f),
  float3(+1.0f, +1.0f, 0.0f),
#else
  float3(-0.5f, -0.5f, 0.0f),
  float3(-0.5f, +0.5f, 0.0f),
  float3(+0.5f, -0.5f, 0.0f),
  float3(+0.5f, +0.5f, 0.0f),
#endif
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
	return(result);
}

float4
ps_main(VertexShader_Output inp) : SV_Target
{
	float4 sample = inp.colour;
	return(sample);
}