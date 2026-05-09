#ifndef DEPTH_Techniques
#define DEPTH_Techniques

float LinearizeDepth(float depth, float nearZ, float farZ) {
    return nearZ * farZ / (farZ - depth * (farZ - nearZ));
}

float LinearizeDepth(float depth, float4 nearFarProjScale) {
    return LinearizeDepth(depth, nearFarProjScale.x, nearFarProjScale.y);
}

float3 ReconstructWorldPosition(
    float2   texCoord,
    float    depth,
    float4x4 invViewProj)
{
    float  ndcX  = mad(texCoord.x,  2.0, -1.0);
    float  ndcY  = mad(texCoord.y, -2.0,  1.0);
    float4 world = mul(invViewProj, float4(ndcX, ndcY, depth, 1.0));
    world.xyz /= world.w;
    return world.xyz;
}

float3 ReconstructViewPosition(
    float2 texCoord,
    float  depth,
    float4 nearFarProjScale)
{
    float linearDepth = LinearizeDepth(depth, nearFarProjScale.x, nearFarProjScale.y);
    float ndcX = mad(texCoord.x,  2.0, -1.0);
    float ndcY = mad(texCoord.y, -2.0,  1.0);
    return float3(
        ndcX / nearFarProjScale.z * linearDepth,
        ndcY / nearFarProjScale.w * linearDepth,
        -linearDepth
    );
}

float3 ReconstructViewPosition(
    float2 texCoord,
    float  depth,
    float4 nearFarProjScale,
    out float linearDepth)
{
    linearDepth = LinearizeDepth(depth, nearFarProjScale.x, nearFarProjScale.y);
    float ndcX = mad(texCoord.x,  2.0, -1.0);
    float ndcY = mad(texCoord.y, -2.0,  1.0);
    return float3(
        ndcX / nearFarProjScale.z * linearDepth,
        ndcY / nearFarProjScale.w * linearDepth,
        -linearDepth
    );
}

#endif // DEPTH_Techniques
