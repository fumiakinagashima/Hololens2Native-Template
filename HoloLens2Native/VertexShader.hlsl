cbuffer TransformBuffer : register(b0) {
    float4x4 world;
    float4x4 viewproj;
};
struct VSInput {
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
};
struct VSOutput {
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(float4(input.pos.xyz, 1), world);
    output.pos = mul(output.pos, viewproj);

    float3 normal = normalize(mul(float4(input.norm, 0), world).xyz);

    output.color = saturate(dot(normal, float3(0, 1, 0))).xxx;
    return output;
}