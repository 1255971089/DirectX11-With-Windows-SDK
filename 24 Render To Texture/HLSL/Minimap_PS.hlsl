#include "Minimap.hlsli"

// ������ɫ��
float4 PS(VertexPosHTex pIn) : SV_Target
{
    // Ҫ��Tex��ȡֵ��Χ����[0.0f, 1.0f], yֵ��Ӧ��������z��
    float2 PosW = pIn.Tex * float2(gRectW.zw - gRectW.xy) + gRectW.xy;
    
    float4 color = gTex.Sample(gSam, pIn.Tex);

    [flatten]
    if (gFogEnabled && length(PosW - gEyePosW.xz) / gVisibleRange > 1.0f)
    {
        return gInvisibleColor;
    }

    return color;
}