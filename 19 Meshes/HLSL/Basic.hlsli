#include "LightHelper.hlsli"

Texture2D gDiffuseMap : register(t0);
SamplerState gSam : register(s0);


cbuffer CBChangesEveryDrawing : register(b0)
{
    matrix gWorld;
    matrix gWorldInvTranspose;
    Material gMaterial;
}

cbuffer CBChangesEveryFrame : register(b1)
{
    matrix gView;
    float3 gEyePosW;
}

cbuffer CBChangesOnResize : register(b2)
{
    matrix gProj;
}

cbuffer CBChangesRarely : register(b3)
{
    DirectionalLight gDirLight[5];
    PointLight gPointLight[5];
    SpotLight gSpotLight[5];
}



struct VertexPosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

struct VertexPosHWNormalTex
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION; // 在世界中的位置
    float3 NormalW : NORMAL; // 法向量在世界中的方向
    float2 Tex : TEXCOORD;
};






