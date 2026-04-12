cbuffer uniforms : register(b0, space1) {
	row_major float4x4 proj_view : packoffset(c0);
};

struct Input {
	float3 position : TEXCOORD0;
	float4 color : TEXCOORD1;
	float2 tex_coord : TEXCOORD2;
	float outline_width : TEXCOORD3;
	float4 outline_color : TEXCOORD4;
};

struct Output {
	float4 color : TEXCOORD0;
	float2 tex_coord : TEXCOORD1;
	float4 position : SV_Position;
	float outline_width : TEXCOORD3;
	float4 outline_color : TEXCOORD4;
};

Output main(Input input) {
	Output output;
	output.color = input.color;
	output.tex_coord = input.tex_coord;
	output.position = mul(float4(input.position, 1.0f), proj_view);
	output.outline_width = input.outline_width;
	output.outline_color = input.outline_color;
	return output;
}
