
Texture2D gTex : register(t0);
SamplerState gSam : register(s0);

cbuffer CBChangesEveryFrame : register(b0)
{
    float3 gEyePosW;            // �����λ��
    float gPad;
}

cbuffer CBDrawingStates : register(b1)
{
    int gFogEnabled;            // �Ƿ�Χ����
    float gVisibleRange;        // 3D������ӷ�Χ
    float2 gPad2;
    float4 gRectW;              // С��ͼxOzƽ���Ӧ3D�����������(Left, Front, Right, Back)
    float4 gInvisibleColor;     // ����������µ���ɫ
}


struct VertexPosTex
{
    float3 PosL : POSITION;
    float2 Tex : TEXCOORD;
};

struct VertexPosHTex
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};





