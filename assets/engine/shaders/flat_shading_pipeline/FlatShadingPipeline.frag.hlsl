#include "../ColorUtils.hlsl"

struct PSIn {
    float4 position : SV_POSITION;
    float2 baseColorUV : TEXCOORD0;
    float3 worldNormal : NORMAL;
    int hasColor : TEXCOORD1;
    int hasAo : TEXCOORD2;
    int hasPerlin : TEXCOORD3;
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
    //float3 lightDir = float3(1.0f, 1.0f, 0.0f);
    float aStrength = 0.1f;
    float3 sStrength = float3(0.5f, 0.5f, 0.5f);
    float3 viewDir = float3(mul(input.position.xyz, 1 / input.position.w));
    float spec_pow = pow(2, 2);
    float lightCol = float3(1.0f, 1.0f, 1.0f);
    float aoStrength = 5.0f;
    //float m = 10.0f;

    if (input.hasColor == 0)
    {
        color = material.color.rgb;
        alpha = material.color.a;
    }
    else
    {
        color = baseColorTexture.Sample(textureSampler, input.baseColorUV);
        //float3 ao = aoColorTexture.Sample(textureSampler, input.aoUV).rgb : float3(1.0, 1.0, 1.0);

        //float3 newStrengths = aoStrength * aStrength * aoColorTexture.Sample(textureSampler, input.baseColorUV);
        //color = float3(color.r * newStrengths.r, color.b * newStrengths.b, color.g * newStrengths.g);

        // Combine base color with AO
        //color = baseColor * ao;
        alpha = 1.0;
    }


    // will change
    float tur = 0.0f;
    int m = 24;
    if (input.hasPerlin == 1) {
        for (int i = 0; i < 4; i++) {
            // Noise(2^i * x,2^i * y)
            float rate = pow(2, i);
            tur += proTexture.Sample(textureSampler, float2 (input.baseColorUV.x * rate, input.baseColorUV.y * rate)) / rate;
        }

        float s = (1 + sin(m * 3.14159f * (input.baseColorUV.x + input.baseColorUV.y + tur))) / 2;

        color = (1 - s) * color + s * lightCol;     // blend color and light color
        //float3 light_orange = float3(1.0f, 0.341f, 0.2f);
        //color = lerp(color, light_orange, s);
    }


    
    // float3 lightDir = float3(-1.0f, -1.0f, -1.0f);
    float3 lightDir = float3(1.0f, 1.0f, 1.0f);
    // float ambient = 0.25f;
    float3 ambient = aStrength * color;

    if (input.hasAo == 1) {
        ambient = aoColorTexture.Sample(textureSampler, input.baseColorUV);
        //ambient.y = aoColorTexture.Sample(textureSampler, input.baseColorUV);
        //ambient.z = aoColorTexture.Sample(textureSampler, input.baseColorUV);

        ambient *= aStrength * color;
    }

    float dot = dot(normalize(-lightDir), normalize(input.worldNormal));
    float3 dirLight = max(dot, 0.0f) * color;
    float3 color2 = dirLight + ambient * color;
    // color = ambient * color;

    //color2 = ApplyExposureToneMapping(color2);
    //// Gamma correct
    //color2 = ApplyGammaCorrection(color2); 

    output.color = float4(color2, alpha);
    return output;
}