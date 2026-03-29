static const float2 positions[6] = {
	float2(-1,-1), float2(1,-1), float2(1, 1),
	float2(-1,-1), float2(1, 1), float2(-1, 1),
};
static const float2 uvs[6] = {
	float2(0,1), float2(1,1), float2(1,0),
	float2(0,1), float2(1,0), float2(0,0),
};

struct Output {
	float2 uv : TEXCOORD0;
	float4 position : SV_Position;
};

Output main(uint id : SV_VertexID) {
	Output o;
	o.position = float4(positions[id], 0, 1);
	o.uv = uvs[id];
	return o;
}
