Texture2D tex : register(t0, space2);
SamplerState smp : register(s0, space2);

float4 main(float2 uv : TEXCOORD0) : SV_Target {
	return tex.Sample(smp, uv);
}
