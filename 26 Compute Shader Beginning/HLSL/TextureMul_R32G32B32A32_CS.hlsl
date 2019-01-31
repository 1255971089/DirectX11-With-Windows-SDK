Texture2D gTexA : register(t0);
Texture2D gTexB : register(t1);

RWTexture2D<float4> gOutput : register(u0);

// һ���߳����е��߳���Ŀ���߳̿���1άչ����Ҳ����
// 2ά��3ά�Ų�
[numthreads(16, 16, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    gOutput[DTid.xy] = gTexA[DTid.xy] * gTexB[DTid.xy];
}
