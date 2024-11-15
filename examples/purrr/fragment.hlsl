struct FSInput {
  [[vk::location(0)]] float2 UV : TEXCOORD0;
};

struct FSOutput {
  [[vk::location(0)]] float4 Color : SV_Target;
};

FSOutput main(FSInput input) {
	FSOutput output;
	output.Color    = float4(1.0f, 0.0f, 0.0f, 1.0f);
	return output;
}