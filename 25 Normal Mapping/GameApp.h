#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "SkyRender.h"
#include "ObjReader.h"
#include "Collision.h"
class GameApp : public D3DApp
{
public:
	// �����ģʽ
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
	// ����ģʽ
	enum class GroundMode { Floor, Stones };

public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();
	
	void DrawScene(bool drawCenterSphere);

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// ��ɫ��ˢ
	ComPtr<IDWriteFont> mFont;								// ����
	ComPtr<IDWriteTextFormat> mTextFormat;					// �ı���ʽ

	ComPtr<ID3D11ShaderResourceView> mFloorDiffuse;			// �ذ�����
	ComPtr<ID3D11ShaderResourceView> mStonesDiffuse;		// ����ʯ������

	Model mGroundModel;										// ��������ģ��
	Model mGroundTModel;									// �����ߵĵ�������ģ��

	GameObject mSphere;										// ��
	GameObject mGround;										// ����
	GameObject mGroundT;									// �����������ĵ���
	GameObject mCylinder;									// Բ��
	GameObject mCylinderT;									// ������������Բ��
	GroundMode mGroundMode;									// ����ģʽ

	ComPtr<ID3D11ShaderResourceView> mBricksNormalMap;		// ש�鷨����ͼ
	ComPtr<ID3D11ShaderResourceView> mFloorNormalMap;		// ���淨����ͼ
	ComPtr<ID3D11ShaderResourceView> mStonesNormalMap;		// ʯͷ���淨����ͼ
	bool mEnableNormalMap;									// ����������ͼ

	BasicEffect mBasicEffect;								// ������Ⱦ��Ч����
	SkyEffect mSkyEffect;									// ��պ���Ч����
	

	std::unique_ptr<DynamicSkyRender> mDaylight;			// ��պ�(����)

	std::shared_ptr<Camera> mCamera;						// �����
	CameraMode mCameraMode;									// �����ģʽ

	ObjReader mObjReader;									// ģ�Ͷ�ȡ����
};


#endif