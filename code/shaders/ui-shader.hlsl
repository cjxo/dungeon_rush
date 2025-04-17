cbuffer VShaderCBuffer0 : register(b0)
{
  float4x4 proj;
};

struct UI_Quad
{
	float2 p;
  float2 dims;
	float rotation_radians;
	float smoothness;
  
  float4 vertex_colours[4];
  float vertex_roundness;

	float4 border_colour;
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

	nointerpolation float2 rect_c         : Rect_C;
	nointerpolation float2 rect_half_dims : Rect_HalfDims;

	float smoothness : Smoothness;

  float4 vertex_colour                  : Colour_Mod;
	float vertex_roundness                : Vertex_Roundness;

	float4 border_colour   : Border_Colour;
	float border_thickness : Border_Thickness;

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

	result.rect_c = quad.p + quad.dims * 0.5f;
	result.rect_half_dims = quad.dims * 0.5f;

	result.smoothness = quad.smoothness;
	
	result.vertex_colour = quad.vertex_colours[vid];
	result.vertex_roundness = quad.vertex_roundness;

	result.border_colour = quad.border_colour;
	result.border_thickness = quad.border_thickness;
	
	result.uv = quad.uvs[vid];
	result.tex_id = quad.tex_id;
	return(result);
}

float sdf_rect(float2 p, float2 rect_c, float2 rect_half_dims, float radius)
{
	float2 p_in_c = abs(p - rect_c);
	float x_dist = p_in_c.x - rect_half_dims.x + radius;
	float y_dist = p_in_c.y - rect_half_dims.y + radius;
	float c_dist = length(p_in_c - rect_half_dims + radius);
	if ((x_dist > 0) && (y_dist > 0))
	{
		return c_dist - radius;
	}
	else
	{
		return max(x_dist, y_dist) - radius;
	}
}

float get_fill_fact(float dist, float smoothness)
{
	if (smoothness == 0)
	{
		return step(0, -dist);
	}
	else
	{
		return smoothstep(-smoothness, smoothness, -dist);
	}
}

float4 apply_mask(float4 a, float mask)
{
	return float4(a.rgb, a.a*mask);
}

float4 put_a_ontop_of_b(float4 a, float4 b)
{
	return a.a*a + (1.0f-a.a)*b;
}

float4 ps_main(VertexShader_Output ps_inp) : SV_Target
{
	float4 final_colour = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float4 rect_colour = ps_inp.vertex_colour;
	switch (ps_inp.tex_id)
	{
		case 1:
		{
			rect_colour *= g_texture1.Sample(g_sampler0, ps_inp.uv);
		} break;

		case 2:
		{
			rect_colour *= g_texture2.Sample(g_sampler0, ps_inp.uv);
		} break;
	}
	
	float2 sample_p = ps_inp.p.xy;
	float2 rect_c = ps_inp.rect_c;
	float2 rect_half_dims = ps_inp.rect_half_dims;
	float vertex_roundness = ps_inp.vertex_roundness;
	
	float4 border_colour = ps_inp.border_colour;
	float border_thickness = ps_inp.border_thickness;

	float smoothness = ps_inp.smoothness;
	
	float border_length = 0.0f;
	float2 adjusted_half_dims = rect_half_dims;
	if (border_thickness > 0)
	{
		float rect_border_dist = sdf_rect(sample_p, rect_c, rect_half_dims, vertex_roundness);
		float rect_border_alpha = get_fill_fact(rect_border_dist, smoothness);
		final_colour = put_a_ontop_of_b(apply_mask(border_colour, rect_border_alpha), final_colour);

		border_length = length(float2(border_thickness, border_thickness));
		adjusted_half_dims = adjusted_half_dims - border_length;
	}
	
	float rect_dist = sdf_rect(sample_p, rect_c, adjusted_half_dims, vertex_roundness - border_length);
	float rect_alpha = get_fill_fact(rect_dist, smoothness);
	final_colour = put_a_ontop_of_b(apply_mask(rect_colour, rect_alpha), final_colour);

	if (final_colour.a == 0)
	{
		discard;
	}
	
	return(final_colour);
}