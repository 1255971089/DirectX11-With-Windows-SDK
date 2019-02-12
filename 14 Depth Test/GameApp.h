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

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// ��ɫ��ˢ
	ComPtr<IDWriteFont> mFont;								// ����
	ComPtr<IDWriteTextFormat> mTextFormat;					// �ı���ʽ

	GameObject mBoltAnim;									// ���綯��
	GameObject mWoodCrate;									// ľ��
	GameObject mFloor;										// �ذ�
	std::vector<GameObject> mWalls;							// ǽ��
	GameObject mMirror;										// ����

	std::vector<ComPtr<ID3D11ShaderResourceView>> mBoltSRVs;// ���綯������

	Material mShadowMat;									// ��Ӱ����
	Material mWoodCrateMat;									// ľ�в���

	BasicEffect mBasicEffect;							// ������Ⱦ��Ч����

	std::shared_ptr<Camera> mCamera;						// �����
	CameraMode mCameraMode;									// �����ģʽ

};


#endif