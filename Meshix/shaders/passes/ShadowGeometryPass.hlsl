#include "../structures.h"

// ====================================================
//                    Vertex Shader
// ====================================================

cbuffer VSIndirect : register(b0) {
    uint ObjectHandle;
}

cbuffer VS : register(b1) {
    uint CascadeIndex;
}

ConstantBuffer<CascadeShadowConstants> cascadeShadowConstants : register(b2);

StructuredBuffer<ObjectConstants> objectConstants : register(t0);

struct VSInput {
    float3 Position : POSITION;
    float2 TexCoord  : TEXCOORD;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    uint   RTIndex  : SV_RenderTargetArrayIndex;
};

VSOutput VSMain(VSInput vsInput) {
    ObjectConstants object = objectConstants[ObjectHandle];
    float4 fragPos = mul(object.World, float4(vsInput.Position, 1.0f));

    VSOutput output;
    output.RTIndex  = CascadeIndex;
    output.TexCoord = vsInput.TexCoord;
    output.Position = mul(cascadeShadowConstants.CascadeDatas[CascadeIndex].LightViewProjection, fragPos);
    return output;
}

// ====================================================
//                    Pixel Shader
// ====================================================

cbuffer PSIndirect : register(b0) {
    uint MaterialHandle;
}

SamplerState PointSampler : register(s0);

StructuredBuffer<MaterialConstants> materialConstants : register(t0);

// 1-based handle
template<typename T>
T GetTexture(uint handle) {
    return ResourceDescriptorHeap[handle];
}

// 1-based handle
MaterialConstants GetMaterial(uint handle) {
    return materialConstants[handle];
}

void PSMain(VSOutput psInput) {
    if (MaterialHandle) {
        MaterialConstants material = GetMaterial(MaterialHandle);
        if (material.baseColorHandle) {
            Texture2D<float4> tBaseColor = GetTexture<Texture2D<float4> >(material.baseColorHandle);
            float4 baseColor = tBaseColor.Sample(PointSampler, psInput.TexCoord);

            // The alphaMode here can only be 0 or 1. When it is 2, forward rendering will be used directly.
            if (material.alphaMode && baseColor.a <= material.alphaCutoff) discard;
        }
    }
}
