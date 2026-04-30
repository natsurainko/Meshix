#ifndef CSM_Techniques
#define CSM_Techniques

#ifndef CASCADE_NUM
#define CASCADE_NUM 4
#endif

#include "CSM.h"

void FastSelectCascadeLayer(
    float           depth,
    CascadeData     cascadeDatas[CASCADE_NUM],
    out uint        layer,
    out CascadeData cascadeData)
{
    for (layer = 0; layer < CASCADE_NUM; ++layer) {
        cascadeData = cascadeDatas[layer];
        if (depth < cascadeData.PlaneDistance) {
            break;
        }
    }
}

void SelectCascadeLayer(
    float4          positionDepth,
    CascadeData     cascadeDatas[CASCADE_NUM],
    inout uint      layer,
    out   float4    shadowCoords,
    out CascadeData cascadeData)
{
    float  depth   = positionDepth.w;
    float3 postion = positionDepth.xyz;

    for (; layer < CASCADE_NUM; ++layer) {
        cascadeData = cascadeDatas[layer];
        if (depth > cascadeData.PlaneDistance && layer != CASCADE_NUM - 1) {
            continue;
        }

        shadowCoords   = mul(cascadeData.LightViewProjection, float4(postion, 1.0));
        shadowCoords.x = mad(shadowCoords.x, 0.5, 0.5);
        shadowCoords.y = mad(shadowCoords.y, -0.5, 0.5);

        if (/*all(shadowCoords.xy > cascadeData.ShadowMapTexelSize) &&
            all(shadowCoords.xy < 1.0 - cascadeData.ShadowMapTexelSize) && */
            shadowCoords.z > 0.0 &&
            shadowCoords.z < 1.0)
        {
            break;
        }
    }
}

#endif // CSM_Techniques
