cbuffer UBO : register(b0, space1) {
	float4x4 mvp;
}

struct InstanceData {
	float4x4 model;
};

struct Input {
	float3 position : TEXCOORD0;
	float4 color : TEXCOORD1;
	float2 uv : TEXCOORD2;
};

struct Output {
	float4 position : SV_Position;
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

StructuredBuffer<InstanceData> DataBuffer : register(t0, space0);

Output main(Input input, uint id : SV_InstanceID) {
	Output output;

	InstanceData instance = DataBuffer[id];

	output.position = mul(mvp, mul(instance.model, float4(input.position, 1)));
	output.color = input.color;
	output.uv = input.uv;

	return output;
}

