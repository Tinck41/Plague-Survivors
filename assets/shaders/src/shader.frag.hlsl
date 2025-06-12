struct Input {
	float4 color : TEXCOORD0;
};

Texture2D<float4> tex : register(t0, space2);

float4 main(Input input) : SV_Target0 {
	return input.color;
}
