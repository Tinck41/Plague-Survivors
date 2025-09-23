Texture2D<float4> tex : register(t0, space2);
SamplerState samp : register(s0, space2);

struct Input {
	float4 color : TEXCOORD0;
	float2 tex_coord : TEXCOORD1;
};

struct Output {
	float4 color : SV_Target;
};

Output main(Input input) {
	Output output;
	output.color = input.color * tex.Sample(samp, input.tex_coord);
	return output;
}
