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

	if (!mScreenFadeEffect.InitAll(md3dDevice))
		return false;

	if (!mMinimapEffect.InitAll(md3dDevice))
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
		// С��ͼ����ģ������
		mMinimap.SetMesh(md3dDevice, Geometry::Create2DShow(1.0f - 100.0f / mClientWidth * 2,  -1.0f + 100.0f / mClientHeight * 2, 
			100.0f / mClientWidth * 2, 100.0f / mClientHeight * 2));
		// ��Ļ���뵭�������С����
		mScreenFadeRender = std::make_unique<TextureRender>(md3dDevice, mClientWidth, mClientHeight, false);
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

	// ���µ��뵭��ֵ��������������ж�
	if (mFadeUsed)
	{
		mFadeAmount += mFadeSign * dt / 2.0f;	// 2sʱ�䵭��/����
		if (mFadeSign > 0.0f && mFadeAmount > 1.0f)
		{
			mFadeAmount = 1.0f;
			mFadeUsed = false;	// ��������
		}
		else if (mFadeSign < 0.0f && mFadeAmount < 0.0f)
		{
			mFadeAmount = 0.0f;
			SendMessage(MainWnd(), WM_DESTROY, 0, 0);	// �رճ���
			// ���ﲻ������������Ϊ���͹رմ��ڵ���Ϣ��Ҫ��һ��������ر�
		}
	}
	else
	{
		// ********************
		// ����������Ĳ���
		//

		// �����ƶ�
		if (keyState.IsKeyDown(Keyboard::W))
			cam1st->Walk(dt * 3.0f);
		if (keyState.IsKeyDown(Keyboard::S))
			cam1st->Walk(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		// ��Ұ��ת����ֹ��ʼ�Ĳ�ֵ�����µ�ͻȻ��ת
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);

		// �����ƶ���Χ
		XMFLOAT3 adjustedPos;
		XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorReplicate(-90.0f), XMVectorReplicate(90.0f)));
		cam1st->SetPosition(adjustedPos);
	}

	// ���¹۲����
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());
	mBasicEffect.SetEyePos(mCamera->GetPositionXM());
	mMinimapEffect.SetEyePos(mCamera->GetPositionXM());
	
	// ����
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Q))
		mPrintScreenStarted = true;
		
	// ���ù���ֵ
	mMouse->ResetScrollWheelValue();

	// �˳����򣬿�ʼ����
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Escape))
	{
		mFadeSign = -1.0f;
		mFadeUsed = true;
	}
}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	
	// ******************
	// ����Direct3D����
	//

	// Ԥ����պ󱸻�����
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	if (mFadeUsed)
	{
		// ��ʼ����/����
		mScreenFadeRender->Begin(md3dImmediateContext);
	}


	// ����������
	DrawScene(false);

	// �˴�����С��ͼ����Ļ����
	UINT strides[1] = { sizeof(VertexPosTex) };
	UINT offsets[1] = { 0 };
	
	// С��ͼ��ЧӦ��
	mMinimapEffect.SetRenderDefault(md3dImmediateContext);
	mMinimapEffect.Apply(md3dImmediateContext);
	// ������С��ͼ
	md3dImmediateContext->IASetVertexBuffers(0, 1, mMinimap.modelParts[0].vertexBuffer.GetAddressOf(), strides, offsets);
	md3dImmediateContext->IASetIndexBuffer(mMinimap.modelParts[0].indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	md3dImmediateContext->DrawIndexed(6, 0, 0);

	if (mFadeUsed)
	{
		// ��������/��������ʱ���Ƶĳ�������Ļ���뵭����Ⱦ������
		mScreenFadeRender->End(md3dImmediateContext);

		// ��Ļ���뵭����ЧӦ��
		mScreenFadeEffect.SetRenderDefault(md3dImmediateContext);
		mScreenFadeEffect.SetFadeAmount(mFadeAmount);
		mScreenFadeEffect.SetTexture(mScreenFadeRender->GetOutputTexture());
		mScreenFadeEffect.SetWorldViewProjMatrix(XMMatrixIdentity());
		mScreenFadeEffect.Apply(md3dImmediateContext);
		// ������������������Ļ
		md3dImmediateContext->IASetVertexBuffers(0, 1, mFullScreenShow.modelParts[0].vertexBuffer.GetAddressOf(), strides, offsets);
		md3dImmediateContext->IASetIndexBuffer(mFullScreenShow.modelParts[0].indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		md3dImmediateContext->DrawIndexed(6, 0, 0);
		// ��ؽ��������ɫ���ϵ���Դ����Ϊ��һ֡��ʼ������Ϊ��ȾĿ��
		mScreenFadeEffect.SetTexture(nullptr);
		mScreenFadeEffect.Apply(md3dImmediateContext);
	}
	
	// ��������Q���£���ֱ𱣴浽output.dds��output.png��
	if (mPrintScreenStarted)
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		// �������
		mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
		HR(SaveDDSTextureToFile(md3dImmediateContext.Get(), backBuffer.Get(), L"Screenshot\\output.dds"));
		HR(SaveWICTextureToFile(md3dImmediateContext.Get(), backBuffer.Get(), GUID_ContainerFormatPng, L"Screenshot\\output.png"));
		// ��������
		mPrintScreenStarted = false;
	}



	// ******************
	// ����Direct2D����
	//
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();
		std::wstring text = L"��ǰ�����ģʽ: ��һ�˳�  Esc�˳�\n"
			"����ƶ�������Ұ W/S/A/D�ƶ�\n"
			"Q-����(���output.dds��output.png��ScreenShot�ļ���)";



		md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
		HR(md2dRenderTarget->EndDraw());
	}

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	mPrintScreenStarted = false;
	mFadeUsed = true;	// ��ʼ����
	mFadeAmount = 0.0f;
	mFadeSign = 1.0f;


	// ******************
	// ��ʼ������Render-To-Texture�Ķ���
	//
	mMinimapRender = std::make_unique<TextureRender>(md3dDevice, 400, 400, true);
	mScreenFadeRender = std::make_unique<TextureRender>(md3dDevice, mClientWidth, mClientHeight, false);

	// ******************
	// ��ʼ����Ϸ����
	//

	// �����������
	CreateRandomTrees();

	// ��ʼ������
	mObjReader.Read(L"Model\\ground.mbo", L"Model\\ground.obj");
	mGround.SetModel(Model(md3dDevice, mObjReader));

	// ��ʼ�����񣬷��������½�200x200
	mMinimap.SetMesh(md3dDevice, Geometry::Create2DShow(0.75f, -0.66666666f, 0.25f, 0.33333333f));
	
	// ����������Ļ�������ģ��
	mFullScreenShow.SetMesh(md3dDevice, Geometry::Create2DShow());

	// ******************
	// ��ʼ�������
	//

	// Ĭ�������
	mCameraMode = CameraMode::FirstPerson;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();
	

	// С��ͼ�����
	mMinimapCamera = std::unique_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mMinimapCamera->SetViewPort(0.0f, 0.0f, 200.0f, 200.0f);	// 200x200С��ͼ
	mMinimapCamera->LookTo(
		XMVectorSet(0.0f, 10.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, -1.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
	mMinimapCamera->UpdateViewMatrix();

	// ******************
	// ��ʼ����������仯��ֵ
	//

	// ��ҹ��Ч
	mBasicEffect.SetFogColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	mBasicEffect.SetFogStart(5.0f);
	mBasicEffect.SetFogRange(20.0f);

	// С��ͼ��Χ����
	mMinimapEffect.SetFogState(true);
	mMinimapEffect.SetInvisibleColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	mMinimapEffect.SetMinimapRect(XMVectorSet(-95.0f, 95.0f, 95.0f, -95.0f));
	mMinimapEffect.SetVisibleRange(25.0f);

	// �����(Ĭ��)
	DirectionalLight dirLight[4];
	dirLight[0].Ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
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
	// ��ȾС��ͼ����
	// 

	mBasicEffect.SetViewMatrix(mMinimapCamera->GetViewXM());
	mBasicEffect.SetProjMatrix(XMMatrixOrthographicLH(190.0f, 190.0f, 1.0f, 20.0f));	// ʹ������ͶӰ����(�����������λ��)
	// �ر���Ч
	mBasicEffect.SetFogState(false);
	mMinimapRender->Begin(md3dImmediateContext);
	DrawScene(true);
	mMinimapRender->End(md3dImmediateContext);

	mMinimapEffect.SetTexture(mMinimapRender->GetOutputTexture());


	// ������Ч���ָ�ͶӰ��������ƫ���Ĺ���
	mBasicEffect.SetFogState(true);
	mBasicEffect.SetProjMatrix(mCamera->GetProjXM());
	dirLight[0].Ambient = XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f);
	dirLight[0].Diffuse = XMFLOAT4(0.16f, 0.16f, 0.16f, 1.0f);
	for (int i = 0; i < 4; ++i)
		mBasicEffect.SetDirLight(i, dirLight[i]);
	

	return true;
}

void GameApp::DrawScene(bool drawMinimap)
{

	mBasicEffect.SetTextureUsed(true);


	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
	if (drawMinimap)
	{
		// С��ͼ�»���������
		mTrees.DrawInstanced(md3dImmediateContext, mBasicEffect, mInstancedData);
	}
	else
	{
		// ͳ��ʵ�ʻ��Ƶ�������Ŀ
		std::vector<XMMATRIX> acceptedData;
		// Ĭ����׶��ü�
		acceptedData = Collision::FrustumCulling(mInstancedData, mTrees.GetLocalBoundingBox(),
			mCamera->GetViewXM(), mCamera->GetProjXM());
		// Ĭ��Ӳ��ʵ��������
		mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
		mTrees.DrawInstanced(md3dImmediateContext, mBasicEffect, acceptedData);
	}
	
	// ���Ƶ���
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
	mGround.Draw(md3dImmediateContext, mBasicEffect);	
}

void GameApp::CreateRandomTrees()
{
	srand((unsigned)time(nullptr));
	// ��ʼ����
	mObjReader.Read(L"Model\\tree.mbo", L"Model\\tree.obj");
	mTrees.SetModel(Model(md3dDevice, mObjReader));
	XMMATRIX S = XMMatrixScaling(0.015f, 0.015f, 0.015f);

	BoundingBox treeBox = mTrees.GetLocalBoundingBox();
	// ��ȡ����Χ�ж���
	mTreeBoxData = Collision::CreateBoundingBox(treeBox, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	// ����ľ�ײ���������λ��y = -2��ƽ��
	treeBox.Transform(treeBox, S);
	XMMATRIX T0 = XMMatrixTranslation(0.0f, -(treeBox.Center.y - treeBox.Extents.y + 2.0f), 0.0f);
	// �������144������������
	float theta = 0.0f;
	for (int i = 0; i < 16; ++i)
	{
		// ȡ5-95�İ뾶�����������
		for (int j = 0; j < 3; ++j)
		{
			// ����ԽԶ����ľԽ��
			for (int k = 0; k < 2 * j + 1; ++k)
			{
				float radius = (float)(rand() % 30 + 30 * j + 5);
				float randomRad = rand() % 256 / 256.0f * XM_2PI / 16;
				XMMATRIX T1 = XMMatrixTranslation(radius * cosf(theta + randomRad), 0.0f, radius * sinf(theta + randomRad));
				XMMATRIX R = XMMatrixRotationY(rand() % 256 / 256.0f * XM_2PI);
				XMMATRIX World = S * R * T0 * T1;
				mInstancedData.push_back(World);
			}
		}
		theta += XM_2PI / 16;
	}
}
