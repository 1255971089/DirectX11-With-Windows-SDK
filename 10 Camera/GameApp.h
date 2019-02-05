#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Geometry.h"
#include "LightHelper.h"
#include "Camera.h"

class GameApp : public D3DApp
{
public:

	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT4 eyePos;
	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};

	struct CBChangesRarely
	{
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		Material material;
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;		// �����֤16�ֽڶ���
	};

	// һ��������С����Ϸ������
	class GameObject
	{
	public:
		GameObject();

		// ��ȡλ��
		DirectX::XMFLOAT3 GetPosition() const;
		// ���û�����
		template<class VertexType, class IndexType>
		void SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData);
		// ��������
		void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);
		// ���þ���
		void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
		void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX world);
		// ����
		void Draw(ComPtr<ID3D11DeviceContext> deviceContext);
	private:
		DirectX::XMFLOAT4X4 mWorldMatrix;				// �������
		ComPtr<ID3D11ShaderResourceView> mTexture;		// ����
		ComPtr<ID3D11Buffer> mVertexBuffer;				// ���㻺����
		ComPtr<ID3D11Buffer> mIndexBuffer;				// ����������
		UINT mVertexStride;								// �����ֽڴ�С
		UINT mIndexCount;								// ������Ŀ	
	};

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
	bool InitEffect();
	bool InitResource();

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// ��ɫ��ˢ
	ComPtr<IDWriteFont> mFont;								// ����
	ComPtr<IDWriteTextFormat> mTextFormat;					// �ı���ʽ

	ComPtr<ID3D11InputLayout> mVertexLayout2D;				// ����2D�Ķ������벼��
	ComPtr<ID3D11InputLayout> mVertexLayout3D;				// ����3D�Ķ������벼��
	ComPtr<ID3D11Buffer> mConstantBuffers[4];				// ����������

	GameObject mWoodCrate;									// ľ��
	GameObject mFloor;										// �ذ�
	std::vector<GameObject> mWalls;							// ǽ��

	ComPtr<ID3D11VertexShader> mVertexShader3D;				// ����3D�Ķ�����ɫ��
	ComPtr<ID3D11PixelShader> mPixelShader3D;				// ����3D��������ɫ��
	ComPtr<ID3D11VertexShader> mVertexShader2D;				// ����2D�Ķ�����ɫ��
	ComPtr<ID3D11PixelShader> mPixelShader2D;				// ����2D��������ɫ��

	CBChangesEveryFrame mCBFrame;							// �û�������Ž���ÿһ֡���и��µı���
	CBChangesOnResize mCBOnResize;							// �û�������Ž��ڴ��ڴ�С�仯ʱ���µı���
	CBChangesRarely mCBRarely;								// �û�������Ų����ٽ����޸ĵı���

	ComPtr<ID3D11SamplerState> mSamplerState;				// ������״̬

	std::shared_ptr<Camera> mCamera;						// �����
	CameraMode mCameraMode;									// �����ģʽ

};


#endif