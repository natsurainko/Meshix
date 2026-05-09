#include "../structures.h"
#include "../noise.hlsli"
#include "../depth.hlsli"
#include "../hbao.hlsli"

// ====================================================
//                    Vertex Shader
// ====================================================

struct VSInput {
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

VSOutput VSMain(VSInput vsInput) {
    VSOutput output;
    output.Position = float4(vsInput.Position, 1.0);
    output.TexCoord = vsInput.TexCoord;
    return output;
}

// ====================================================
//                    Pixel Shader
// ====================================================

ConstantBuffer<FrameConstants> frameConstants : register(b0);

SamplerState PointSampler : register(s0);

Texture2D<float>  gDepth  : register(t0);
Texture2D<float4> gNormal : register(t1);

float4 PSMain(VSOutput input) : SV_TARGET {
    float depth = gDepth.SampleLevel(PointSampler, input.TexCoord, 0).r;
    if (depth == 1.0f) return float4(1.0f, 1.0f, 1.0f, 1.0f);

    float  linearDepth;
    float3 viewPos    = ReconstructViewPosition(input.TexCoord, depth, frameConstants.NearFarProjScale, linearDepth);
    float3 fragNormal = gNormal.SampleLevel(PointSampler, input.TexCoord, 0).xyz;
    float3 viewNormal = normalize(mul((float3x3)frameConstants.View, fragNormal));

    float radiusPixels = GetRadiusPixels(linearDepth, frameConstants.NearFarProjScale.z, frameConstants.FrameResolution.x);
    if (radiusPixels < 1.0f) return float4(1.0f, 1.0f, 1.0f, 1.0f);

    float ao = HBAO(viewPos,
        viewNormal,
        input.TexCoord,
        radiusPixels,
        gDepth,
        PointSampler,
        frameConstants.NearFarProjScale,
        frameConstants.FrameResolutionInverse,
        input.Position.xy);

    return float4(ao, 1.0f, 1.0f, 1.0f);
}
