cbuffer perFrame : register(b0)
{
    matrix view;
    matrix projection;
}

cbuffer perObject : register(b1)
{
    matrix world;
};

struct VertexShaderInput
{
    float3 localPosition : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VertexToPixelShadow
{
    float4 screenPosition : SV_POSITION;
};


VertexToPixelShadow main(VertexShaderInput input)
{
    VertexToPixelShadow output;

    matrix wvp = mul(projection, mul(view, world));
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));

    return output;
}