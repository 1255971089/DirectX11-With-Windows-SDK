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

	// ��ȡ����
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(mCamera);

	if (mCameraMode == CameraMode::Free)
	{
		// ******************
		// ����������Ĳ���
		//

		// �����ƶ�
		if (keyState.IsKeyDown(Keyboard::W))
		{
			cam1st->MoveForward(dt * 3.0f);
		}
		if (keyState.IsKeyDown(Keyboard::S))
		{
			cam1st->MoveForward(dt * -3.0f);
		}
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		// ��Ұ��ת����ֹ��ʼ�Ĳ�ֵ�����µ�ͻȻ��ת
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);
	}

	// ******************
	// ���������
	//

	// ��λ��������[-49.9f, 49.9f]��������
	// ��������
	XMFLOAT3 adjustedPos;
	XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), 
		XMVectorSet(-49.9f, 0.0f, -49.9f, 0.0f), XMVectorSet(49.9f, 99.9f, 49.9f, 0.0f)));
	cam1st->SetPosition(adjustedPos);

	// ���¹۲����
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetEyePos(mCamera->GetPositionXM());
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());

	// ******************
	// ������Ч
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		mFogEnabled = !mFogEnabled;
		mBasicEffect.SetFogState(mFogEnabled);
	}

	// ******************
	// ����/��ҹ�任
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		mIsNight = !mIsNight;
		if (mIsNight)
		{
			// ��ҹģʽ�±�Ϊ�𽥺ڰ�
			mBasicEffect.SetFogColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
			mBasicEffect.SetFogStart(5.0f);
		}
		else
		{
			// ����ģʽ���Ӧ��Ч
			mBasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
			mBasicEffect.SetFogStart(15.0f);
		}
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		mEnableAlphaToCoverage = !mEnableAlphaToCoverage;
	}

	// ******************
	// ������ķ�Χ
	//
	if (mouseState.scrollWheelValue != 0)
	{
		// һ�ι��ֹ�������С��λΪ120
		mFogRange += mouseState.scrollWheelValue / 120;
		if (mFogRange < 15.0f)
			mFogRange = 15.0f;
		else if (mFogRange > 175.0f)
			mFogRange = 175.0f;
		mBasicEffect.SetFogRange(mFogRange);
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
	// ����Direct3D����
	//
	
	// ���ñ���ɫ
	if (mIsNight)
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	}
	else
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	}
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ���Ƶ���
	mBasicEffect.SetRenderDefault(md3dImmediateContext);
	mGround.Draw(md3dImmediateContext, mBasicEffect);

	// ������
	mBasicEffect.SetRenderBillboard(md3dImmediateContext, mEnableAlphaToCoverage);
	mBasicEffect.SetMaterial(mTreeMat);
	UINT stride = sizeof(VertexPosSize);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, mPointSpritesBuffer.GetAddressOf(), &stride, &offset);
	mBasicEffect.Apply(md3dImmediateContext);
	md3dImmediateContext->Draw(16, 0);

	// ******************
	// ����Direct2D����
	//
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();
		std::wstring text = L"1-��Ч���� 2-����/��ҹ��Ч�л� 3-AlphaToCoverage���� Esc-�˳�\n"
			"����-������Ч��Χ\n"
			"��֧�������ӽ������\n";
		text += std::wstring(L"AlphaToCoverage״̬: ") + (mEnableAlphaToCoverage ? L"����\n" : L"�ر�\n");
		text += std::wstring(L"��Ч״̬: ") + (mFogEnabled ? L"����\n" : L"�ر�\n");
		if (mFogEnabled)
		{
			text += std::wstring(L"�������: ") + (mIsNight ? L"��ҹ\n" : L"����\n");
			text += L"��Ч��Χ: " + std::to_wstring(mIsNight ? 5 : 15) + L"-" +
				std::to_wstring((mIsNight ? 5 : 15) + (int)mFogRange);
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
	// ��ʼ����������
	//

	// ��ʼ����������Դ
	ComPtr<ID3D11Texture2D> test;
	HR(CreateDDSTexture2DArrayFromFile(
		md3dDevice.Get(),
		md3dImmediateContext.Get(),
		std::vector<std::wstring>{
			L"Texture\\tree0.dds",
			L"Texture\\tree1.dds",
			L"Texture\\tree2.dds",
			L"Texture\\tree3.dds"},
		test.GetAddressOf(),
		mTreeTexArray.GetAddressOf()));
	mBasicEffect.SetTextureArray(mTreeTexArray);

	// ��ʼ���㾫�黺����
	InitPointSpritesBuffer();

	// ��ʼ�����Ĳ���
	mTreeMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mTreeMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mTreeMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	ComPtr<ID3D11ShaderResourceView> texture;
	// ��ʼ���ذ�
	mGround.SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, -5.0f, 0.0f), XMFLOAT2(100.0f, 100.0f), XMFLOAT2(10.0f, 10.0f)));
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\Grass.dds", nullptr, texture.GetAddressOf()));
	mGround.SetTexture(texture);
	Material material;
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	mGround.SetMaterial(material);

	// ******************
	// ��ʼ������仯��ֵ
	//

	// �����
	DirectionalLight dirLight[4];
	dirLight[0].Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].Diffuse = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
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

	// ******************
	// ��ʼ�������
	//
	mCameraMode = CameraMode::Free;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();

	mBasicEffect.SetWorldMatrix(XMMatrixIdentity());
	mBasicEffect.SetViewMatrix(camera->GetViewXM());
	mBasicEffect.SetProjMatrix(camera->GetProjXM());
	mBasicEffect.SetEyePos(camera->GetPositionXM());

	// ******************
	// ��ʼ����Ч��������
	//

	// Ĭ�ϰ��죬����AlphaToCoverage
	mIsNight = false;
	mEnableAlphaToCoverage = true;

	// ��״̬Ĭ�Ͽ���
	mFogEnabled = 1;
	mFogRange = 75.0f;

	mBasicEffect.SetFogState(mFogEnabled);
	mBasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
	mBasicEffect.SetFogStart(15.0f);
	mBasicEffect.SetFogRange(75.0f);

	
	
	return true;
}

void GameApp::InitPointSpritesBuffer()
{
	srand((unsigned)time(nullptr));
	VertexPosSize vertexes[16];
	float theta = 0.0f;
	for (int i = 0; i < 16; ++i)
	{
		// ȡ20-50�İ뾶�����������
		float radius = (float)(rand() % 31 + 20);
		float randomRad = rand() % 256 / 256.0f * XM_2PI / 16;
		vertexes[i].pos = XMFLOAT3(radius * cosf(theta + randomRad), 8.0f, radius * sinf(theta + randomRad));
		vertexes[i].size = XMFLOAT2(30.0f, 30.0f);
		theta += XM_2PI / 16;
	}

	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;	// ���ݲ����޸�
	vbd.ByteWidth = sizeof (vertexes);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertexes;
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mPointSpritesBuffer.GetAddressOf()));
}
