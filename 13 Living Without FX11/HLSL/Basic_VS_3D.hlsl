#include "Basic.hlsli"

// 顶点着色器(3D)
VertexPosHWNormalTex VS_3D(VertexPosNormalTex pIn)
{
    VertexPosHWNormalTex pOut;
    
    matrix viewProj = mul(gView, gProj);
    float4 posW = mul(float4(pIn.PosL, 1.0f), gWorld);
    // 若当前在绘制反射物体，先进行反射操作
    [flatten]
    if (gIsReflection)
    {
        posW = mul(posW, gReflection);
    }
    // 若当前在绘制阴影，先进行投影操作
    [flatten]
    if (gIsShadow)
    {
        posW = (gIsReflection ? mul(posW, gRefShadow) : mul(posW, gShadow));
    }

    pOut.PosH = mul(posW, viewProj);
    pOut.PosW = posW.xyz;
    pOut.NormalW = mul(pIn.NormalL, (float3x3) gWorldInvTranspose);
    pOut.Tex = pIn.Tex;
    return pOut;
}