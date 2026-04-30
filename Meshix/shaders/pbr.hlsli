#ifndef PBR_Techniques
#define PBR_Techniques

float3 fresnelSchlick(float cosTheta, float3 F0) {
    // return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    return mad(1.0 - F0, pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0), F0);
}

float DistributionGGX(float NdotH, float alphaRoughness) {
    float a2     = alphaRoughness * alphaRoughness;
    float NdotH2 = NdotH * NdotH;
    float denom  = mad(NdotH2, a2 - 1.0, 1.0); // (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359f * denom * denom;
    return a2 / denom;
}

float VisibilityGGX(float NdotL, float NdotV, float alphaRoughness) {
    float a2     = alphaRoughness * alphaRoughness;
    float GGXV = NdotL * sqrt(mad(NdotV, NdotV, -NdotV * NdotV * a2) + a2);
    float GGXL = NdotV * sqrt(mad(NdotL, NdotL, -NdotL * NdotL * a2) + a2);
    float GGX  = GGXV + GGXL;
    return (GGX > 0.0) ? 0.5 / GGX : 0.0;
}

float3 PBRLighting(float  NdotV,
                   float3 N,
                   float3 V,
                   float3 L,
                   float3 radiance,
                   float3 baseColor,
                   float  roughness,
                   float  metallic) {
    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float alphaRoughness = roughness * roughness;

    float NDF = DistributionGGX(NdotH, alphaRoughness);
    float Vis = VisibilityGGX(NdotL, NdotV, alphaRoughness);

    float  specularWeight = NDF * Vis;
    float3 diffuse        = baseColor / 3.14159265359f;

    float3 metal_F      = fresnelSchlick(HdotV, baseColor);
    float3 dielectric_F = fresnelSchlick(HdotV, float3(0.04, 0.04, 0.04));

    float3 l_metal_brdf      = metal_F * specularWeight;
    float3 l_dielectric_brdf = lerp(diffuse, specularWeight, dielectric_F);

    float3 l_color = lerp(l_dielectric_brdf, l_metal_brdf, metallic);
    return l_color * radiance * NdotL;
}

#endif // PBR_Techniques
