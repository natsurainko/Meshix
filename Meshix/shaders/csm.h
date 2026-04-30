#ifndef CSM_H
#define CSM_H

#include "chlsl.h"

struct CascadeData {
    float4x4 LightViewProjection IDENTITY;

    float PlaneDistance      ZERO;
    float ShadowMapTexelSize ZERO;
    float ZStartBiasScale    ZERO;
    float RadiusScale        ZERO;

    float4 DepthConvertToView;
};

#endif // CSM_H
