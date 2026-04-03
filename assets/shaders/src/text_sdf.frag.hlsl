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
	float distance = tex.Sample(samp, input.tex_coord).a;
	const float smoothing = fwidth(distance) * 0.7;
	float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
	output.color = float4(input.color.rgb, input.color.a * alpha);
	return output;
}
