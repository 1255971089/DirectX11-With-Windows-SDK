#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// ����ȳ�ʼ��������Ⱦ״̬���Թ��������Чʹ��
	RenderStates::InitAll(md3dDevice);

	if (!mBasicEffect.InitAll(md3dDevice))
		return false;

	if (!mSkyEffect.InitAll(md3dDevice))
		return false;

	if (!InitResource())
		return false;

	// ��ʼ����꣬���̲���Ҫ
	mMouse->SetWindow(mhMainWnd);
	mMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

	return true;
}

void GameApp::OnResize()
{
	assert(md2dFactory);
	assert(mdwriteFactory);
	// �ͷ�D2D�������Դ
	mColorBrush.Reset();
	md2dRenderTarget.Reset();

	D3DApp::OnResize();

	// ΪD2D����DXGI������ȾĿ��
	ComPtr<IDXGISurface> surface;
	HR(mSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = md2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, md2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE)
	{
		OutputDebugString(L"\n���棺Direct2D��Direct3D�������Թ������ޣ��㽫�޷������ı���Ϣ�����ṩ������ѡ������\n"
			"1. ����Win7ϵͳ����Ҫ������Win7 SP1������װKB2670838������֧��Direct2D��ʾ��\n"
			"2. �������Direct3D 10.1��Direct2D�Ľ�����������ģ�"
			"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			"3. ʹ�ñ������⣬����FreeType��\n\n");
	}
	else if (hr == S_OK)
	{
		// �����̶���ɫˢ���ı���ʽ
		HR(md2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			mColorBrush.GetAddressOf()));
		HR(mdwriteFactory->CreateTextFormat(L"����", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
			mTextFormat.GetAddressOf()));
	}
	else
	{
		// �����쳣����
		assert(md2dRenderTarget);
	}

	// ����������ʾ
	if (mCamera != nullptr)
	{
		mCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		mCamera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
		mBasicEffect.SetProjMatrix(mCamera->GetProjXM());
	}
}

void GameApp::UpdateScene(float dt)
{

	// ��������¼�����ȡ���ƫ����
	Mouse::State mouseState = mMouse->GetState();
	Mouse::State lastMouseState = mMouseTracker.GetLastState();
	mMouseTracker.Update(mouseState);

	Keyboard::State keyState = mKeyboard->GetState();
	mKeyboardTracker.Update(keyState);

	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(mCamera);

	// ********************
	// ����������Ĳ���
	//

	// �����ƶ�
	if (keyState.IsKeyDown(Keyboard::W))
		cam1st->MoveForward(dt * 3.0f);
	if (keyState.IsKeyDown(Keyboard::S))
		cam1st->MoveForward(dt * -3.0f);
	if (keyState.IsKeyDown(Keyboard::A))
		cam1st->Strafe(dt * -3.0f);
	if (keyState.IsKeyDown(Keyboard::D))
		cam1st->Strafe(dt * 3.0f);

	// ��Ұ��ת����ֹ��ʼ�Ĳ�ֵ�����µ�ͻȻ��ת
	cam1st->Pitch(mouseState.y * dt * 1.25f);
	cam1st->RotateY(mouseState.x * dt * 1.25f);

	// ���¹۲����
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());
	mBasicEffect.SetEyePos(mCamera->GetPositionXM());

	// ������ͼ����
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
		mEnableNormalMap = !mEnableNormalMap;
		
	// �л���������
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D2) && mGroundMode != GroundMode::Floor)
	{
		mGroundMode = GroundMode::Floor;
		mGroundModel.modelParts[0].texDiffuse = mFloorDiffuse;
		mGroundTModel.modelParts[0].texDiffuse = mFloorDiffuse;
		mGround.SetModel(mGroundModel);
		mGroundT.SetModel(mGroundTModel);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::D3) && mGroundMode != GroundMode::Stones)
	{
		mGroundMode = GroundMode::Stones;
		mGroundModel.modelParts[0].texDiffuse = mStonesDiffuse;
		mGroundTModel.modelParts[0].texDiffuse = mStonesDiffuse;
		mGround.SetModel(mGroundModel);
		mGroundT.SetModel(mGroundTModel);
	}

	// ���ù���ֵ
	mMouse->ResetScrollWheelValue();

	// �˳���������Ӧ�򴰿ڷ���������Ϣ
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	// ******************
	// ���ɶ�̬��պ�
	//

	// ������ǰ���Ƶ���ȾĿ����ͼ�����ģ����ͼ
	mDaylight->Cache(md3dImmediateContext, mBasicEffect);

	// ���ƶ�̬��պе�ÿ���棨������Ϊ���ģ�
	for (int i = 0; i < 6; ++i)
	{
		mDaylight->BeginCapture(
			md3dImmediateContext, mBasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i));

		// ������������
		DrawScene(false);
	}

	// �ָ�֮ǰ�Ļ����趨
	mDaylight->Restore(md3dImmediateContext, mBasicEffect, *mCamera);
	
	// ******************
	// ���Ƴ���
	//

	// Ԥ�����
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ����������
	DrawScene(true);
	

	// ******************
	// ����Direct2D����
	//
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();
		std::wstring text = L"��ǰ�����ģʽ: �����ӽ�  Esc�˳�\n"
			"����ƶ�������Ұ W/S/A/D�ƶ�\n"
			"1-������ͼ: ";
		text += mEnableNormalMap ? L"����\n" : L"�ر�\n";
		text += L"�л�����: 2-�ذ�  3-����ʯ��\n"
			"��ǰ����: ";
		text += (mGroundMode == GroundMode::Floor ? L"�ذ�" : L"����ʯ��");

		md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
		HR(md2dRenderTarget->EndDraw());
	}

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// ��ʼ��������ͼ���
	//
	mEnableNormalMap = true;

	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\bricks_nmap.dds", nullptr, mBricksNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\floor_nmap.dds", nullptr, mFloorNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\stones_nmap.dds", nullptr, mStonesNormalMap.GetAddressOf()));

	// ******************
	// ��ʼ����պ����

	mDaylight = std::make_unique<DynamicSkyRender>(
		md3dDevice, md3dImmediateContext, 
		L"Texture\\daylight.jpg", 
		5000.0f, 256);

	mBasicEffect.SetTextureCube(mDaylight->GetDynamicTextureCube());

	// ******************
	// ��ʼ����Ϸ����
	//

	mGroundMode = GroundMode::Floor;
	
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\floor.dds", nullptr, mFloorDiffuse.GetAddressOf()));
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\stones.dds", nullptr, mStonesDiffuse.GetAddressOf()));
	// ����
	mGroundModel.SetMesh(md3dDevice,
		Geometry::CreatePlane(XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	mGroundModel.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mGroundModel.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mGroundModel.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	mGroundModel.modelParts[0].material.Reflect = XMFLOAT4();
	mGroundModel.modelParts[0].texDiffuse = mFloorDiffuse;
	mGround.SetModel(mGroundModel);

	// �����������ĵ���
	mGroundTModel.SetMesh(md3dDevice, Geometry::CreatePlane<VertexPosNormalTangentTex>(
		XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	mGroundTModel.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mGroundTModel.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mGroundTModel.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	mGroundTModel.modelParts[0].material.Reflect = XMFLOAT4();
	mGroundTModel.modelParts[0].texDiffuse = mFloorDiffuse;
	mGroundT.SetModel(mGroundTModel);

	// ����
	Model model;
	ComPtr<ID3D11ShaderResourceView> texDiffuse;

	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Texture\\stone.dds",
		nullptr,
		texDiffuse.GetAddressOf()));

	model.SetMesh(md3dDevice, Geometry::CreateSphere(1.0f, 30, 30));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].texDiffuse = texDiffuse;
	mSphere.SetModel(std::move(model));

	// ����
	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Texture\\bricks.dds",
		nullptr,
		texDiffuse.ReleaseAndGetAddressOf()));

	model.SetMesh(md3dDevice,
		Geometry::CreateCylinder(0.5f, 2.0f));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	model.modelParts[0].texDiffuse = texDiffuse;
	mCylinder.SetModel(std::move(model));

	// ����������������
	model.SetMesh(md3dDevice,
		Geometry::CreateCylinder<VertexPosNormalTangentTex>(0.5f, 2.0f));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	model.modelParts[0].texDiffuse = texDiffuse;
	mCylinderT.SetModel(std::move(model));

	// ******************
	// ��ʼ�������
	//
	mCameraMode = CameraMode::Free;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	// ��ʼ�������¹۲����ͶӰ����(����������̶�)
	camera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(camera->GetViewXM());
	mBasicEffect.SetProjMatrix(camera->GetProjXM());


	// ******************
	// ��ʼ������仯��ֵ
	//

	// �����
	DirectionalLight dirLight[4];
	dirLight[0].Ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
	dirLight[0].Diffuse = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	dirLight[0].Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].Direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	dirLight[1] = dirLight[0];
	dirLight[1].Direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
	dirLight[2] = dirLight[0];
	dirLight[2].Direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
	dirLight[3] = dirLight[0];
	dirLight[3].Direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
	for (int i = 0; i < 4; ++i)
		mBasicEffect.SetDirLight(i, dirLight[i]);

	return true;
}

void GameApp::DrawScene(bool drawCenterSphere)
{
	// ����ģ��
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
	mBasicEffect.SetTextureUsed(true);
	
	// ֻ��������ķ���Ч��
	if (drawCenterSphere)
	{
		mBasicEffect.SetReflectionEnabled(true);
		mBasicEffect.SetRefractionEnabled(false);
		mSphere.Draw(md3dImmediateContext, mBasicEffect);
	}
	
	// ���Ƶ���
	mBasicEffect.SetReflectionEnabled(false);
	mBasicEffect.SetRefractionEnabled(false);
	
	if (mEnableNormalMap)
	{
		mBasicEffect.SetRenderWithNormalMap(md3dImmediateContext, BasicEffect::RenderObject);
		if (mGroundMode == GroundMode::Floor)
			mBasicEffect.SetTextureNormalMap(mFloorNormalMap);
		else
			mBasicEffect.SetTextureNormalMap(mStonesNormalMap);
		mGroundT.Draw(md3dImmediateContext, mBasicEffect);
	}
	else
	{
		mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
		mGround.Draw(md3dImmediateContext, mBasicEffect);
	}

	// �������Բ��
	// ��Ҫ�̶�λ��
	static std::vector<XMMATRIX> cyliderWorlds = {
		XMMatrixTranslation(0.0f, -1.99f, 0.0f),
		XMMatrixTranslation(4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, -4.5f),
		XMMatrixTranslation(4.5f, -1.99f, -4.5f)
	};

	if (mEnableNormalMap)
	{
		mBasicEffect.SetRenderWithNormalMap(md3dImmediateContext, BasicEffect::RenderInstance);
		mBasicEffect.SetTextureNormalMap(mBricksNormalMap);
		mCylinderT.DrawInstanced(md3dImmediateContext, mBasicEffect, cyliderWorlds);
	}
	else
	{
		mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
		mCylinder.DrawInstanced(md3dImmediateContext, mBasicEffect, cyliderWorlds);
	}
	
	// �������Բ��
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
	static float rad = 0.0f;
	rad += 0.001f;
	// ��Ҫ��̬λ�ã���ʹ��static
	std::vector<XMMATRIX> sphereWorlds = {
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(4.5f, 0.5f * XMScalarSin(rad), 4.5f),
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(-4.5f, 0.5f * XMScalarSin(rad), 4.5f),
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(-4.5f, 0.5f * XMScalarSin(rad), -4.5f),
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(4.5f, 0.5f * XMScalarSin(rad), -4.5f),
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(2.5f * XMScalarCos(rad), 0.0f, 2.5f * XMScalarSin(rad))
	};
	mSphere.DrawInstanced(md3dImmediateContext, mBasicEffect, sphereWorlds);

	// ������պ�
	mSkyEffect.SetRenderDefault(md3dImmediateContext);

	if (drawCenterSphere)
		mDaylight->Draw(md3dImmediateContext, mSkyEffect, *mCamera);
	else
		mDaylight->Draw(md3dImmediateContext, mSkyEffect, mDaylight->GetCamera());
	
}

