#include "Basic.hlsli"

// ������ɫ��(3D)
VertexPosHWNormalTex VS_3D(VertexPosNormalTex pIn)
{
    VertexPosHWNormalTex pOut;
    
    matrix viewProj = mul(gView, gProj);
    float4 posW = mul(float4(pIn.PosL, 1.0f), gWorld);
    // ����ǰ�ڻ��Ʒ������壬�Ƚ��з������
    [flatten]
    if (gIsReflection)
    {
        posW = mul(posW, gReflection);
    }
    // ����ǰ�ڻ�����Ӱ���Ƚ���ͶӰ����
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