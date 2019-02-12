#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"


class GameApp : public D3DApp
{
public:
	// �����ģʽ
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
	
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();
	void InitPointSpritesBuffer();

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// ��ɫ��ˢ
	ComPtr<IDWriteFont> mFont;								// ����
	ComPtr<IDWriteTextFormat> mTextFormat;					// �ı���ʽ

	ComPtr<ID3D11Buffer> mPointSpritesBuffer;				// �㾫�鶥�㻺����
	ComPtr<ID3D11ShaderResourceView> mTreeTexArray;			// ������������
	Material mTreeMat;										// ���Ĳ���

	GameObject mGround;										// ����
	
	BasicEffect mBasicEffect;							// ������Ⱦ��Ч����

	CameraMode mCameraMode;									// �����ģʽ
	std::shared_ptr<Camera> mCamera;						// �����

	bool mFogEnabled;										// �Ƿ�����Ч
	bool mIsNight;											// �Ƿ��ҹ
	bool mEnableAlphaToCoverage;							// �Ƿ���Alpha-To-Coverage

	float mFogRange;										// ��Ч��Χ
};


#endif