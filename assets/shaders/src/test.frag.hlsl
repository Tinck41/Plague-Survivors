cbuffer CircleData : register(b0, space1)
{
	float radius;
	float softness;
	float2 center;
};

struct Input {
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

Texture2D<float4> tex : register(t0, space2);
SamplerState smp : register(s0, space2);

float4 main(Input input) : SV_Target0 {
	float2 p = (input.uv - center) * 2.0;

	float dist = length(p);

	// мягкий край
	float alpha = smoothstep(radius, radius - softness, dist);

	float4 color = tex.Sample(smp, input.uv) * input.color;
	color.a *= alpha;

	return color;
}

