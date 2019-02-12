#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Effects.h"
#include "Vertex.h"

class GameApp : public D3DApp
{
public:
	enum class Mode { SplitedTriangle, SplitedSnow, SplitedSphere };
	
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

	void ResetSplitedTriangle();
	void ResetSplitedSnow();
	void ResetSplitedSphere();


private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// ��ɫ��ˢ
	ComPtr<IDWriteFont> mFont;								// ����
	ComPtr<IDWriteTextFormat> mTextFormat;					// �ı���ʽ

	ComPtr<ID3D11Buffer> mVertexBuffers[7];					// ���㻺��������
	int mVertexCounts[7];									// ������Ŀ
	int mCurrIndex;											// ��ǰ����
	Mode mShowMode;											// ��ǰ��ʾģʽ
	bool mIsWireFrame;										// �Ƿ�Ϊ�߿�ģʽ
	bool mShowNormal;										// �Ƿ���ʾ������

	BasicEffect mBasicEffect;							// ���������Ч������

};


#endif