#include "../ColorUtils.hlsl"

struct PSIn {
    float4 position : SV_POSITION;
    float4 world : POSITION0;
    float2 baseColorUV : TEXCOORD0;
    float3 worldNormal : NORMAL;
    int hasColor : TEXCOORD1;
    int hasAo : TEXCOORD2;
    int hasPerlin : TEXCOORD3;
    int m : TEXCOORD4;
    float3 camPosition : TEXCOORD5;
};

struct PSOut {
    float4 color : SV_Target0;
};

struct Material
{
    float4 color;
    int hasBaseColorTexture;
    int placeholder0;
    int placeholder1;
    int placeholder2;
};

sampler textureSampler : register(s1, space0);

ConstantBuffer <Material> material: register(b0, space1);

Texture2D baseColorTexture : register(t1, space1);
Texture2D aoColorTexture : register(t2, space1);
Texture2D proTexture : register(t3, space1);

PSOut main(PSIn input) {
    PSOut output;

    float3 color;
    float alpha;

    float3 lightDir = float3(1.0f, 1.0f, 1.0f);
    float amIntensity = 0.1f;
    float3 specIntensity = float3(0.5f, 0.5f, 0.5f);
    float spec_pow = pow(2, 2);
    float3 lightCol = float3(0.870588f, 0.929412f, 0.686275f);


    // handle base color texture toggle
    if (input.hasColor == 0)
    {
        color = material.color.rgb;
        alpha = material.color.a;
    }
    else
    {
        color = baseColorTexture.Sample(textureSampler, input.baseColorUV);
        alpha = 1.0;
    }

    // Turbulence function for Perlin Texture
    float tur = 0.0f;
    if (input.hasPerlin == 1) {

        // Noise(2^i * x,2^i * y)  with i 0->4
        for (int i = 0; i < 5; i++) {
            float rate = pow(2, i);
            tur += proTexture.Sample(textureSampler, float2 (input.baseColorUV.x * rate, input.baseColorUV.y * rate)) / rate;
        }

        float s = (1 + sin(input.m * 3.14159f * (input.baseColorUV.x + input.baseColorUV.y + tur))) / 2;

        color = (1 - s) * color + s * lightCol;     // blend color and light color
    }

    // handle AO
    float3 ambient = amIntensity * color;

    if (input.hasAo == 1) {
        ambient = aoColorTexture.Sample(textureSampler, input.baseColorUV);
        ambient *= amIntensity * color;
    }

 
    float3 worldDir = normalize(input.worldNormal);   // normal
    float3 lightNormal = normalize(lightDir);
    float3 viewDir = normalize(input.world.xyz - input.camPosition);      // view direction

    float3 R = reflect(lightNormal, worldDir);   // perfectly reflection direction

    float angleIn = dot(worldDir, lightNormal);
    float angleOut = dot(viewDir, R);

    if (angleIn < 0) {
        angleIn = 0.0f;
        angleOut = 0.0f;
    }
    if (angleOut < 0) {
        angleOut = 0.0f;
    }

    float3 diffuse = angleIn * color;
    float3 spec = pow(angleOut, spec_pow) * specIntensity;

    float3 color2 = (ambient + diffuse + spec) * lightCol;

    output.color = float4(color2, alpha);
    return output;
}