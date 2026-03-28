struct Input {
	float3 position : POSITION;
	float4 color : TEXCOORD1;
	float2 uv : TEXCOORD0;
};

struct Output {
	float4 position : SV_Position;
	float4 color : TEXCOORD1;
	float2 uv : TEXCOORD0;
};

cbuffer UniformBlock : register(b0, space1) {
	float4x4 mvp : packoffset(c0);
};

Output main(Input input) {
	Output output;

	output.position = mul(mvp, float4(input.position, 1));
	output.color = input.color;
	output.uv = input.uv;

	return output;
}
