#include "../structures.h"

// ====================================================
//                    Vertex Shader
// ====================================================

ConstantBuffer<ObjectConstants>        objectConstants        : register(b0);
ConstantBuffer<CascadeShadowConstants> cascadeShadowConstants : register(b1);

struct VSInput {
    float3 Position : POSITION;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    uint   RTIndex  : SV_RenderTargetArrayIndex;
};

VSOutput VSMain(VSInput vsInput, uint instanceID : SV_InstanceID) {
    float4 fragPos = mul(objectConstants.World, float4(vsInput.Position, 1.0f));

    VSOutput output;
    output.RTIndex  = instanceID;
    output.Position = mul(cascadeShadowConstants.CascadeDatas[instanceID].LightViewProjection, fragPos);
    return output;
}

// ====================================================
//                    Pixel Shader
// ====================================================

void PSMain() {
    // empty shader
}
