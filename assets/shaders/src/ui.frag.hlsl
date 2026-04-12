struct Input {
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
	uint flags : TEXCOORD2;
};

Texture2D<float4> tex : register(t0, space2);
SamplerState smp : register(s0, space2);

static const uint TEXTURED = 1u << 0;

float4 main(Input input) : SV_Target0 {
	if (input.flags & TEXTURED) {
		return tex.Sample(smp, input.uv) * input.color;
	}

	return input.color;
}

