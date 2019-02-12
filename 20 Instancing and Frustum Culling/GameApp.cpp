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

	// ��ȡ����
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(mCamera);

	// ******************
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

	// ******************
	// ������������
	//

	// ��λ��������[-119.9f, 119.9f]��������
	// ��������
	XMFLOAT3 adjustedPos;
	XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(),
		XMVectorSet(-119.9f, 0.0f, -119.9f, 0.0f), XMVectorSet(119.9f, 99.9f, 119.9f, 0.0f)));
	cam1st->SetPosition(adjustedPos);

	// ���¹۲����
	mBasicEffect.SetEyePos(mCamera->GetPositionXM());
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());

	// ******************
	// ��׶��ü�����
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		mEnableInstancing = !mEnableInstancing;
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		mEnableFrustumCulling = !mEnableFrustumCulling;
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
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ͳ��ʵ�ʻ��Ƶ�������Ŀ
	std::vector<XMMATRIX> acceptedData;
	// �Ƿ�����׶��ü�
	if (mEnableFrustumCulling)
	{
		acceptedData = Collision::FrustumCulling(mInstancedData, mTrees.GetLocalBoundingBox(),
			mCamera->GetViewXM(), mCamera->GetProjXM());
	}
	// ȷ��ʹ�õ����ݼ�
	const std::vector<XMMATRIX>& refData = mEnableFrustumCulling ? acceptedData : mInstancedData;
	// �Ƿ���Ӳ��ʵ����
	if (mEnableInstancing)
	{
		// Ӳ��ʵ��������
		mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
		mTrees.DrawInstanced(md3dImmediateContext, mBasicEffect, refData);
	}
	else
	{
		// ��������ʽ�������
		mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
		for (FXMMATRIX mat : refData)
		{
			mTrees.SetWorldMatrix(mat);
			mTrees.Draw(md3dImmediateContext, mBasicEffect);
		}
	}

	// ���Ƶ���
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
	mGround.Draw(md3dImmediateContext, mBasicEffect);

	// ******************
	// ����Direct2D����
	//
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();
		std::wstring text = L"��ǰ�����ģʽ: �����ӽ�  Esc�˳�\n"
			"����ƶ�������Ұ WSAD�ƶ�\n"
			"���ּ�1:Ӳ��ʵ�������� 2:��׶��ü�����\n"
			"Ӳ��ʵ����: ";
		text += (mEnableInstancing ? L"��" : L"��");
		text += L" ��׶��ü�: ";
		text += (mEnableFrustumCulling ? L"��" : L"��");
		text += L"\n������ľ��: " + std::to_wstring(refData.size());
		md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
		HR(md2dRenderTarget->EndDraw());
	}

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// Ĭ�Ͽ�����׶��ü���Ӳ��ʵ����
	mEnableInstancing = true;
	mEnableFrustumCulling = true;

	// ******************
	// ��ʼ����Ϸ����
	//

	// �����������
	CreateRandomTrees();

	// ��ʼ������
	mObjReader.Read(L"Model\\ground.mbo", L"Model\\ground.obj");
	mGround.SetModel(Model(md3dDevice, mObjReader));

	// ******************
	// ��ʼ�������
	//
	mCameraMode = CameraMode::Free;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(camera->GetViewXM());
	mBasicEffect.SetProjMatrix(camera->GetProjXM());

	// ******************
	// ��ʼ������仯��ֵ
	//

	// �����
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

	return true;
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
	// �������256������������
	float theta = 0.0f;
	for (int i = 0; i < 16; ++i)
	{
		// ȡ5-125�İ뾶�����������
		for (int j = 0; j < 4; ++j)
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

