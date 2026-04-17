Texture2D<float4> tex : register(t0, space2);
SamplerState samp : register(s0, space2);

struct Input {
	float4 color : TEXCOORD0;
	float2 tex_coord : TEXCOORD1;
	float outline_width : TEXCOORD2;
	float4 outline_color : TEXCOORD3;
};

struct Output {
	float4 color : SV_Target;
};

float4 main(Input input) : SV_Target {
	float dist = tex.Sample(samp, input.tex_coord).a;

	float smoothing = fwidth(dist) * 0.7;

	// fill (внутри текста)
	float fill = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);

	// --- outline параметры ---
	float outline_soft  = smoothing;

	// outline зона (чуть снаружи границы)
	float outline = smoothstep(
		0.35 - input.outline_width - outline_soft,
		0.35 - input.outline_width + outline_soft,
		dist
	);

	// вычитаем fill чтобы не перекрывать текст
	outline = outline - fill;

	float4 result = float4(input.color.rgb, input.color.a * fill);;
	if (input.outline_width > 0.0) {
		result = input.color * fill + input.outline_color * outline;
		result.a = saturate(result.a);
	}

	return result;
}
