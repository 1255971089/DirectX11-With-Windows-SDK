#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "TextureRender.h"
#include "ObjReader.h"
#include "Collision.h"

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
	void CreateRandomTrees();

	void DrawScene(bool drawMiniMap);

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// ��ɫ��ˢ
	ComPtr<IDWriteFont> mFont;								// ����
	ComPtr<IDWriteTextFormat> mTextFormat;					// �ı���ʽ

	GameObject mTrees;										// ��
	GameObject mGround;										// ����
	std::vector<DirectX::XMMATRIX> mInstancedData;			// ����ʵ������
	Collision::WireFrameData mTreeBoxData;					// ����Χ���߿�����

	BasicEffect mBasicEffect;								// ������Ⱦ��Ч����
	ScreenFadeEffect mScreenFadeEffect;						// ��Ļ���뵭����Ч����
	MinimapEffect mMinimapEffect;							// С��ͼ��Ч����

	std::unique_ptr<TextureRender> mMinimapRender;			// С��ͼ��������
	std::unique_ptr<TextureRender> mScreenFadeRender;		// ������������


	ComPtr<ID3D11Texture2D> mMinimapTexture;				// С��ͼ����
	ComPtr<ID3D11ShaderResourceView> mMininmapSRV;			// С��ͼ��ɫ����Դ
	Model mMinimap;											// С��ͼ����ģ��
	Model mFullScreenShow;									// ȫ����ʾ����ģ��

	std::shared_ptr<Camera> mCamera;						// �����
	std::unique_ptr<FirstPersonCamera> mMinimapCamera;		// С��ͼ���������
	CameraMode mCameraMode;									// �����ģʽ

	ObjReader mObjReader;									// ģ�Ͷ�ȡ����

	bool mPrintScreenStarted;								// ������ǰ֡
	bool mEscapePressed;									// �˳�������
	bool mFadeUsed;											// �Ƿ�ʹ�õ���/����
	float mFadeAmount;										// ����/����ϵ��
	float mFadeSign;										// 1.0f��ʾ���룬-1.0f��ʾ����
};


#endif