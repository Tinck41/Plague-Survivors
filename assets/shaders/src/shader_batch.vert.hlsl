struct SpriteData {
	float3 position;
	float rotation;
	float2 scale;
	float2 padding;
	float u, v, w, h;
	float4 color;
};

struct Output {
	float4 position : SV_Position;
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

StructuredBuffer<SpriteData> DataBuffer : register(t0, space0);

cbuffer UBO : register(b0, space1) {
	float4x4 view_proj : packoffset(c0);
}

static const uint indices[6] = { 0, 1, 2, 2, 3, 0 };
static const float2 vertices[4] = {
	{ 0.f, 1.f },
	{ 1.f, 1.f },
	{ 1.f, 0.f },
	{ 0.f, 0.f }
};

Output main(uint vertex_id : SV_VertexID) {
	uint sprite_id = vertex_id / 6;
	uint index = indices[vertex_id % 6];
	SpriteData sprite = DataBuffer[sprite_id];

	float2 tex_coord[4] = {
		{ sprite.u,            sprite.v + sprite.h },
		{ sprite.u + sprite.w, sprite.v + sprite.h },
		{ sprite.u + sprite.w, sprite.v            },
		{ sprite.u,            sprite.v            }
	};

	float c = cos(sprite.rotation);
	float s = sin(sprite.rotation);

	float2 coord = vertices[index];
	coord *= sprite.scale;
	float2x2 rotation = {c, s, -s, c};
	coord = mul(coord, rotation);

	float3 coordWithDepth = float3(coord + sprite.position.xy, sprite.position.z);

	Output output;

	output.position = mul(view_proj, float4(coordWithDepth, 1.0f));
	output.color = sprite.color;
	output.uv = tex_coord[index];

	return output;
}

