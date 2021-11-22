cbuffer externalData : register(b0)
{
    matrix view;
    matrix projection;
    float currentTime;
};

struct Particle
{
    float EmitTime;
    float3 StartPosition;
};

StructuredBuffer<Particle> ParticleData : register(t0);

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VertexToPixel main( uint id : SV_VertexID )
{
    VertexToPixel output;
    
    uint particleID = id / 4;
    uint cornerID = id % 4;
    Particle p = ParticleData.Load(particleID);
    float3 pos = p.StartPosition;
    
    float age = currentTime - p.EmitTime;
    
    //Cool effects
    pos += float3(0.1f, 0, 0) * age;
    //
    
    float2 offsets[4];
    offsets[0] = float2(-1.0f, +1.0f);
    offsets[1] = float2(+1.0f, +1.0f);
    offsets[2] = float2(+1.0f, -1.0f);
    offsets[3] = float2(-1.0f, -1.0f);
    
    pos += float3(view._11, view._12, view._13) * offsets[cornerID].x;
    pos += float3(view._21, view._22, view._23) * offsets[cornerID].y;
    
    matrix viewProj = mul(projection, view);
    output.position = mul(view, float4(pos, 1.0f));
    
    float2 uvs[4];
    uvs[0] = float2(0, 0);
    uvs[1] = float2(1, 0);
    uvs[2] = float2(1, 1);
    uvs[3] = float2(0, 1);
    output.uv = uvs[cornerID];
    
    return output;
    
}