struct Input {
	float3 position     : POSITION;
	float2 uv           : TEXCOORD0;
	float4 color        : TEXCOORD1;
	uint flags          : TEXCOORD2;
	float2 size         : TEXCOORD3;
	float border_radius : TEXCOORD4;
	float4 border_color : TEXCOORD5;
	float border_width  : TEXCOORD6;
	float2 local_pos    : TEXCOORD7;
};

struct Output {
	float4 position     : SV_Position;
	float2 uv           : TEXCOORD0;
	float4 color        : TEXCOORD1;
	uint   flags        : TEXCOORD2;
	float2 size         : TEXCOORD3;
	float border_radius : TEXCOORD4;
	float4 border_color : TEXCOORD5;
	float border_width  : TEXCOORD6;
	float2 local_pos    : TEXCOORD7;
};

cbuffer UniformBlock : register(b0, space1) {
	float4x4 mvp : packoffset(c0);
};

Output main(Input input) {
	Output output;

	output.position = mul(mvp, float4(input.position, 1));
	output.uv = input.uv;
	output.color = input.color;
	output.flags = input.flags;
	output.size = input.size;
	output.border_radius = input.border_radius;
	output.border_color = input.border_color;
	output.border_width = input.border_width;
	output.local_pos = input.local_pos;

	return output;
}
