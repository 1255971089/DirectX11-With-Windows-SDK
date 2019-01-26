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
	mMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

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

	// ����ͶӰ����
	mBasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));

}

void GameApp::UpdateScene(float dt)
{

	// ��������¼�����ȡ���ƫ����
	Mouse::State mouseState = mMouse->GetState();
	Mouse::State lastMouseState = mMouseTracker.GetLastState();
	mMouseTracker.Update(mouseState);

	Keyboard::State keyState = mKeyboard->GetState();
	mKeyboardTracker.Update(keyState);

	UINT stride = (mShowMode != Mode::SplitedSphere ? sizeof(VertexPosColor) : sizeof(VertexPosNormalColor));
	UINT offset = 0;


	// ******************
	// �л�����
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Q))
	{
		mShowMode = Mode::SplitedTriangle;
		ResetSplitedTriangle();
		mIsWireFrame = false;
		mShowNormal = false;
		mCurrIndex = 0;
		stride = sizeof(VertexPosColor);
		md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::W))
	{
		mShowMode = Mode::SplitedSnow;
		ResetSplitedSnow();
		mIsWireFrame = true;
		mShowNormal = false;
		mCurrIndex = 0;
		stride = sizeof(VertexPosColor);
		md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::E))
	{
		mShowMode = Mode::SplitedSphere;
		ResetSplitedSphere();
		mIsWireFrame = false;
		mShowNormal = false;
		mCurrIndex = 0;
		stride = sizeof(VertexPosNormalColor);
		md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}

	// ******************
	// �л�����
	//
	for (int i = 0; i < 7; ++i)
	{
		if (mKeyboardTracker.IsKeyPressed((Keyboard::Keys)((int)Keyboard::D1 + i)))
		{
			md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[i].GetAddressOf(), &stride, &offset);
			mCurrIndex = i;
		}
	}

	// ******************
	// �л��߿�/��
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::M))
	{
		if (mShowMode != Mode::SplitedSnow)
		{
			mIsWireFrame = !mIsWireFrame;
		}
	}

	// ******************
	// �Ƿ���ӷ�����
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::N))
	{
		if (mShowMode == Mode::SplitedSphere)
		{
			mShowNormal = !mShowNormal;
		}
	}

	// ******************
	// ����ÿ֡�仯��ֵ
	//
	if (mShowMode == Mode::SplitedSphere)
	{
		// ������ת����
		static float theta = 0.0f;
		theta += 0.3f * dt;
		mBasicEffect.SetWorldMatrix(XMMatrixRotationY(theta));
	}
	else
	{
		mBasicEffect.SetWorldMatrix(XMMatrixIdentity());
	}

}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);


	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	// ���ݵ�ǰ����ģʽ������Ҫ������Ⱦ�ĸ�����Դ
	if (mShowMode == Mode::SplitedTriangle)
	{
		mBasicEffect.SetRenderSplitedTriangle(md3dImmediateContext);
	}
	else if (mShowMode == Mode::SplitedSnow)
	{
		mBasicEffect.SetRenderSplitedSnow(md3dImmediateContext);
	}
	else if (mShowMode == Mode::SplitedSphere)
	{
		mBasicEffect.SetRenderSplitedSphere(md3dImmediateContext);
	}

	// �����߿�/��ģʽ
	if (mIsWireFrame)
	{
		md3dImmediateContext->RSSetState(RenderStates::RSWireframe.Get());
	}
	else
	{
		md3dImmediateContext->RSSetState(nullptr);
	}

	// ���л��ƣ��ǵ�Ӧ�ó����������ı��
	mBasicEffect.Apply(md3dImmediateContext);
	md3dImmediateContext->Draw(mVertexCounts[mCurrIndex], 0);
	// ���Ʒ�����
	if (mShowNormal)
	{
		mBasicEffect.SetRenderNormal(md3dImmediateContext);
		mBasicEffect.Apply(md3dImmediateContext);
		md3dImmediateContext->Draw(mVertexCounts[mCurrIndex], 0);
	}


	// ******************
	// ����Direct2D����
	//
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();
		std::wstring text = L"�л����Σ�Q-������(��/�߿�) W-ѩ��(�߿�) E-��(��/�߿�)\n"
			L"����������1 - 7�����ν�����Խ��Խ��ϸ\n"
			L"M-��/�߿��л�\n\n"
			L"��ǰ����: " + std::to_wstring(mCurrIndex + 1) + L"\n"
			"��ǰ����: ";
		if (mShowMode == Mode::SplitedTriangle)
			text += L"������";
		else if (mShowMode == Mode::SplitedSnow)
			text += L"ѩ��";
		else
			text += L"��";

		if (mIsWireFrame)
			text += L"(�߿�)";
		else
			text += L"(��)";

		if (mShowMode == Mode::SplitedSphere)
		{
			if (mShowNormal)
				text += L"(N-�رշ�����)";
			else
				text += L"(N-����������)";
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
	// ��ʼ������
	//

	// Ĭ�ϻ���������
	mShowMode = Mode::SplitedTriangle;
	mIsWireFrame = false;
	mShowNormal = false;
	ResetSplitedTriangle();
	// Ԥ�Ȱ󶨶��㻺����
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[0].GetAddressOf(), &stride, &offset);

	// ******************
	// ��ʼ������仯��ֵ
	//

	// �����
	DirectionalLight dirLight;
	dirLight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.Direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	mBasicEffect.SetDirLight(0, dirLight);
	// ����
	Material material;
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
	mBasicEffect.SetMaterial(material);
	// �����λ��
	mBasicEffect.SetEyePos(XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f));
	// ����
	mBasicEffect.SetWorldMatrix(XMMatrixIdentity());
	mBasicEffect.SetViewMatrix(XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f),
		XMVectorZero(),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
	mBasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));

	mBasicEffect.SetSphereCenter(XMFLOAT3(0.0f, 0.0f, 0.0f));
	mBasicEffect.SetSphereRadius(2.0f);

	return true;
}


void GameApp::ResetSplitedTriangle()
{
	// ******************
	// ��ʼ��������
	//

	// ���������ζ���
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.0f * 3, 0.866f * 3, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};
	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;	// ������Ҫ����������׶�ͨ��GPUд��
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// ��Ҫ��������������ǩ
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mVertexBuffers[0].ReleaseAndGetAddressOf()));


	// �����ζ�����
	mVertexCounts[0] = 3;
	// ��ʼ�����ж��㻺����
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 3;
		mVertexCounts[i] = mVertexCounts[i - 1] * 3;
		HR(md3dDevice->CreateBuffer(&vbd, nullptr, mVertexBuffers[i].ReleaseAndGetAddressOf()));
		mBasicEffect.SetStreamOutputSplitedTriangle(md3dImmediateContext, mVertexBuffers[i - 1], mVertexBuffers[i]);
		md3dImmediateContext->Draw(mVertexCounts[i - 1], 0);
	}
}

void GameApp::ResetSplitedSnow()
{
	// ******************
	// ѩ�����δӳ�ʼ�������ο�ʼ����Ҫ6������
	//

	// ���������ζ���
	float sqrt3 = sqrt(3.0f);
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(0.0f, sqrt3 / 2, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(0.0f, sqrt3 / 2, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(-3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
	};
	// �������ο�Ⱥ͸߶ȶ��Ŵ�3��
	for (VertexPosColor& v : vertices)
	{
		v.pos.x *= 3;
		v.pos.y *= 3;
	}

	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;	// ������Ҫ����������׶�ͨ��GPUд��
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// ��Ҫ��������������ǩ
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mVertexBuffers[0].ReleaseAndGetAddressOf()));

	// ������
	mVertexCounts[0] = 6;
	// ��ʼ�����ж��㻺����
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 4;
		mVertexCounts[i] = mVertexCounts[i - 1] * 4;
		HR(md3dDevice->CreateBuffer(&vbd, nullptr, mVertexBuffers[i].ReleaseAndGetAddressOf()));
		mBasicEffect.SetStreamOutputSplitedSnow(md3dImmediateContext, mVertexBuffers[i - 1], mVertexBuffers[i]);
		md3dImmediateContext->Draw(mVertexCounts[i - 1], 0);
	}
}

void GameApp::ResetSplitedSphere()
{
	// ******************
	// ��������
	//

	VertexPosNormalColor basePoint[] = {
		{ XMFLOAT3(0.0f, 2.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(2.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-2.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f, 0.0f, -2.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f, -2.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	};
	int indices[] = { 0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 1, 4, 1, 2, 5, 2, 3, 5, 3, 4, 5, 4, 1, 5 };

	std::vector<VertexPosNormalColor> vertices;
	for (int pos : indices)
	{
		vertices.push_back(basePoint[pos]);
	}


	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;	// ������Ҫ����������׶�ͨ��GPUд��
	vbd.ByteWidth = (UINT)(vertices.size() * sizeof(VertexPosNormalColor));
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// ��Ҫ��������������ǩ
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mVertexBuffers[0].ReleaseAndGetAddressOf()));

	// ������
	mVertexCounts[0] = 24;
	// ��ʼ�����ж��㻺����
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 4;
		mVertexCounts[i] = mVertexCounts[i - 1] * 4;
		HR(md3dDevice->CreateBuffer(&vbd, nullptr, mVertexBuffers[i].ReleaseAndGetAddressOf()));
		mBasicEffect.SetStreamOutputSplitedSphere(md3dImmediateContext, mVertexBuffers[i - 1], mVertexBuffers[i]);
		md3dImmediateContext->Draw(mVertexCounts[i - 1], 0);
	}
}



