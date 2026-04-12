struct Input {
	uint  vertex_id : SV_VertexID;

	float4 position : TEXCOORD0;
	float4 rotation : TEXCOORD1;
	float4 scale : TEXCOORD2;
	float4 color : TEXCOORD3;
	float2 uv : TEXCOORD4;
	float2 size : TEXCOORD5;
};

struct Output {
	float4 position : SV_Position;
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

cbuffer UniformBlock : register(b0, space1)
{
	float4x4 view_proj : packoffset(c0);
};

Output main(Input input) {
	Output output;

	float2 vertex_pos = float2(input.vertex_id & 1, input.vertex_id >> 1);

	float2 uv = input.uv + vertex_pos * input.size;

	float c = cos(input.rotation.z);
	float s = sin(input.rotation.z);
	float2x2 rot = { c, s, -s, c };

	float2 local = mul(vertex_pos * input.scale.xy, rot);
	float3 world = float3(local + input.position.xy, input.position.z);

	output.position = mul(view_proj, float4(world, 1.0));
	output.color = input.color;
	output.uv = uv;

	return output;
}
