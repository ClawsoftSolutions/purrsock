struct VSInput {
  [[vk::location(0)]] float2 Position : POSITION0;
  [[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct VSOutput {
	float4 Position : SV_POSITION;
  [[vk::location(0)]] float2 UV : TEXCOORD0;
};

cbuffer cameraBuffer : register(b0) {
  float4x4 projection;
  float4x4 view;
};

StructuredBuffer<float4x4> models : register(t0, space1);

struct push_constant {
  uint modelIndex;
};

[[vk::push_constant]] push_constant pc;

VSOutput main(VSInput input) {
  float4x4 model = models[pc.modelIndex];
  float4 worldPosition = mul(model, float4(input.Position, 0.0f, 1.0f));
  float4 viewPosition = mul(view, worldPosition);

	VSOutput output = (VSOutput)0;
	output.Position = mul(projection, viewPosition);
	output.UV       = input.UV;
	return output;
}