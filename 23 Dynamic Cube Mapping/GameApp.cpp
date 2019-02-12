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

	// ѡ����պ�
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		mSkyBoxMode = SkyBoxMode::Daylight;
		mBasicEffect.SetTextureCube(mDaylight->GetTextureCube());
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		mSkyBoxMode = SkyBoxMode::Sunset;
		mBasicEffect.SetTextureCube(mSunset->GetTextureCube());
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		mSkyBoxMode = SkyBoxMode::Desert;
		mBasicEffect.SetTextureCube(mDesert->GetTextureCube());
	}

	// ѡ�������Ⱦģʽ
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D4))
	{
		mSphereMode = SphereMode::None;
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D5))
	{
		mSphereMode = SphereMode::Reflection;
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D6))
	{
		mSphereMode = SphereMode::Refraction;
	}
	
	// ���ֵ���������
	mEta += mouseState.scrollWheelValue / 12000.0f;
	if (mEta > 1.0f)
	{
		mEta = 1.0f;
	}
	else if (mEta <= 0.2f)
	{
		mEta = 0.2f;
	}
	mBasicEffect.SetRefractionEta(mEta);
		
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
	switch (mSkyBoxMode)
	{
	case SkyBoxMode::Daylight: mDaylight->Cache(md3dImmediateContext, mBasicEffect); break;
	case SkyBoxMode::Sunset: mSunset->Cache(md3dImmediateContext, mBasicEffect); break;
	case SkyBoxMode::Desert: mDesert->Cache(md3dImmediateContext, mBasicEffect); break;
	}

	// ���ƶ�̬��պе�ÿ���棨������Ϊ���ģ�
	for (int i = 0; i < 6; ++i)
	{
		switch (mSkyBoxMode)
		{
		case SkyBoxMode::Daylight: mDaylight->BeginCapture(
			md3dImmediateContext, mBasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		case SkyBoxMode::Sunset: mSunset->BeginCapture(
			md3dImmediateContext, mBasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		case SkyBoxMode::Desert: mDesert->BeginCapture(
			md3dImmediateContext, mBasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		}

		// ������������
		DrawScene(false);
	}

	// �ָ�֮ǰ�Ļ����趨
	switch (mSkyBoxMode)
	{
	case SkyBoxMode::Daylight: mDaylight->Restore(md3dImmediateContext, mBasicEffect, *mCamera); break;
	case SkyBoxMode::Sunset: mSunset->Restore(md3dImmediateContext, mBasicEffect, *mCamera); break;
	case SkyBoxMode::Desert: mDesert->Restore(md3dImmediateContext, mBasicEffect, *mCamera); break;
	}
	
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
			"����ƶ�������Ұ ���ֵ��������� W/S/A/D�ƶ�\n"
			"�л���պ�: 1-���� 2-���� 3-ɳĮ\n"
			"������ģʽ: 4-��   5-���� 6-����\n"
			"��ǰ��պ�: ";

		switch (mSkyBoxMode)
		{
		case SkyBoxMode::Daylight: text += L"����"; break;
		case SkyBoxMode::Sunset: text += L"����"; break;
		case SkyBoxMode::Desert: text += L"ɳĮ"; break;
		}

		text += L" ��ǰģʽ: ";
		switch (mSphereMode)
		{
		case SphereMode::None: text += L"��"; break;
		case SphereMode::Reflection: text += L"����"; break;
		case SphereMode::Refraction: text += L"����\n������: " + std::to_wstring(mEta); break;
		}

		md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
		HR(md2dRenderTarget->EndDraw());
	}

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// ��ʼ����պ����

	mDaylight = std::make_unique<DynamicSkyRender>(
		md3dDevice, md3dImmediateContext, 
		L"Texture\\daylight.jpg", 
		5000.0f, 256);

	mSunset = std::make_unique<DynamicSkyRender>(
		md3dDevice, md3dImmediateContext,
		std::vector<std::wstring>{
		L"Texture\\sunset_posX.bmp", L"Texture\\sunset_negX.bmp",
		L"Texture\\sunset_posY.bmp", L"Texture\\sunset_negY.bmp", 
		L"Texture\\sunset_posZ.bmp", L"Texture\\sunset_negZ.bmp", },
		5000.0f, 256);

	mDesert = std::make_unique<DynamicSkyRender>(
		md3dDevice, md3dImmediateContext,
		L"Texture\\desertcube1024.dds",
		5000.0f, 256);

	mSkyBoxMode = SkyBoxMode::Daylight;
	mBasicEffect.SetTextureCube(mDaylight->GetDynamicTextureCube());

	// ��ʼ��������(����/����)
	mSphereMode = SphereMode::Reflection;
	mEta = 1.0f / 1.51f;

	// ******************
	// ��ʼ����Ϸ����
	//
	
	Model model;
	// ����
	model.SetMesh(md3dDevice, Geometry::CreateSphere(1.0f, 30, 30));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), 
		L"Texture\\stone.dds", 
		nullptr, 
		model.modelParts[0].texDiffuse.GetAddressOf()));
	mSphere.SetModel(std::move(model));
	// ����
	model.SetMesh(md3dDevice, 
		Geometry::CreatePlane(XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f); 
	model.modelParts[0].material.Reflect = XMFLOAT4();
	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Texture\\floor.dds",
		nullptr,
		model.modelParts[0].texDiffuse.GetAddressOf()));
	mGround.SetModel(std::move(model));
	// ����
	model.SetMesh(md3dDevice,
		Geometry::CreateCylinder(0.5f, 2.0f));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Texture\\bricks.dds",
		nullptr,
		model.modelParts[0].texDiffuse.GetAddressOf()));
	mCylinder.SetModel(std::move(model));

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
	
	// ֻ��������з��������Ч��
	if (drawCenterSphere)
	{
		switch (mSphereMode)
		{
		case SphereMode::None: 
			mBasicEffect.SetReflectionEnabled(false);
			mBasicEffect.SetRefractionEnabled(false);
			break;
		case SphereMode::Reflection:
			mBasicEffect.SetReflectionEnabled(true);
			mBasicEffect.SetRefractionEnabled(false);
			break;
		case SphereMode::Refraction:
			mBasicEffect.SetReflectionEnabled(false);
			mBasicEffect.SetRefractionEnabled(true);
			break;
		}
		mSphere.Draw(md3dImmediateContext, mBasicEffect);
	}
	
	// ���Ƶ���
	mBasicEffect.SetReflectionEnabled(false);
	mBasicEffect.SetRefractionEnabled(false);
	mGround.Draw(md3dImmediateContext, mBasicEffect);

	// �������Բ��
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
	// ��Ҫ�̶�λ��
	static std::vector<XMMATRIX> cyliderWorlds = {
		XMMatrixTranslation(0.0f, -1.99f, 0.0f),
		XMMatrixTranslation(4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, -4.5f),
		XMMatrixTranslation(4.5f, -1.99f, -4.5f)
	};
	mCylinder.DrawInstanced(md3dImmediateContext, mBasicEffect, cyliderWorlds);
	
	// �������Բ��
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
	switch (mSkyBoxMode)
	{
	case SkyBoxMode::Daylight: mDaylight->Draw(md3dImmediateContext, mSkyEffect, 
		(drawCenterSphere ? *mCamera : mDaylight->GetCamera())); break;
	case SkyBoxMode::Sunset: mSunset->Draw(md3dImmediateContext, mSkyEffect,
		(drawCenterSphere ? *mCamera : mSunset->GetCamera())); break;
	case SkyBoxMode::Desert: mDesert->Draw(md3dImmediateContext, mSkyEffect, 
		(drawCenterSphere ? *mCamera : mDesert->GetCamera())); break;
	}
	
}

