#include "../structures.h"
#include "../noise.hlsli"
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

Texture2D<float4> gPositionDepth   : register(t0);
Texture2D<float4> gNormalRoughness : register(t1);

float PSMain(VSOutput input) : SV_TARGET0 {
    float linearDepth = gPositionDepth.SampleLevel(PointSampler, input.TexCoord, 0).w;
    if (linearDepth <= 0.0f) return 1.0f;

    float3 viewPos    = ReconstructViewPosition(input.TexCoord, linearDepth, frameConstants.Projection);
    float3 fragNormal = gNormalRoughness.SampleLevel(PointSampler, input.TexCoord, 0).xyz;
    float3 viewNormal = normalize(mul((float3x3)frameConstants.View, fragNormal));

    float radiusPixels = GetRadiusPixels(linearDepth, frameConstants.Projection, frameConstants.FrameResolution.x);
    if (radiusPixels < 1.0f) return 1.0f;

    return HBAO(viewPos,
                viewNormal,
                input.TexCoord,
                radiusPixels,
                gPositionDepth,
                PointSampler,
                frameConstants.Projection,
                frameConstants.FrameResolutionInverse,
                input.Position.xy);
}
