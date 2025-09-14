struct SpriteInstance {
	float4 position;
	float4 rotation;
	float4 scale;
	float4 color;
	float2 uv;
	float2 size;
};

struct Output {
	float4 position : SV_Position;
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

StructuredBuffer<SpriteInstance> InstanceBuffer : register(t0, space0);

cbuffer UniformBlock : register(b0, space1)
{
	float4x4 view_proj : packoffset(c0);
};

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
	SpriteInstance sprite = InstanceBuffer[sprite_id];

	float2 tex_coord[4] = {
		{ sprite.uv.x,                 sprite.uv.y + sprite.size.y },
		{ sprite.uv.x + sprite.size.x, sprite.uv.y + sprite.size.y },
		{ sprite.uv.x + sprite.size.x, sprite.uv.y                 },
		{ sprite.uv.x,                 sprite.uv.y                 }
	};

	float c = cos(sprite.rotation.z);
	float s = sin(sprite.rotation.z);

	float2 coord = vertices[index];
	coord *= sprite.scale.xy;
	float2x2 rotation = {c, s, -s, c};
	coord = mul(coord, rotation);

	float3 coordWithDepth = float3(coord + sprite.position.xy, sprite.position.z);

	Output output;

	output.position = mul(view_proj, float4(coordWithDepth, 1.0f));
	output.color = sprite.color;
	output.uv = tex_coord[index];

	return output;
}

