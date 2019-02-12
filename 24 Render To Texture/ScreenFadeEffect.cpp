#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// ��������Effects.h��d3dUtil.h����
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;


//
// ScreenFadeEffect::Impl ��Ҫ����ScreenFadeEffect�Ķ���
//

class ScreenFadeEffect::Impl : public AlignedType<ScreenFadeEffect::Impl>
{
public:

	//
	// ��Щ�ṹ���ӦHLSL�Ľṹ�塣��Ҫ��16�ֽڶ���
	//

	struct CBChangesEveryFrame
	{
		float fadeAmount;
		XMFLOAT3 pad;
	};

	struct CBChangesRarely
	{
		XMMATRIX worldViewProj;
	};


public:
	// ������ʽָ��
	Impl() = default;
	~Impl() = default;

public:
	CBufferObject<0, CBChangesEveryFrame> cbFrame;		// ÿ֡�޸ĵĳ���������
	CBufferObject<1, CBChangesRarely>	cbRarely;		// �����޸ĵĳ���������
	

	BOOL isDirty;										// �Ƿ���ֵ���
	std::vector<CBufferBase*> cBufferPtrs;				// ͳһ�����������еĳ���������

	ComPtr<ID3D11VertexShader> screenFadeVS;
	ComPtr<ID3D11PixelShader> screenFadePS;

	ComPtr<ID3D11InputLayout> vertexPosTexLayout;

	ComPtr<ID3D11ShaderResourceView> texture;			// ���ڵ��뵭��������
};



//
// ScreenFadeEffect
//

namespace
{
	// ScreenFadeEffect����
	static ScreenFadeEffect * pInstance = nullptr;
}

ScreenFadeEffect::ScreenFadeEffect()
{
	if (pInstance)
		throw std::exception("ScreenFadeEffect is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<ScreenFadeEffect::Impl>();
}

ScreenFadeEffect::~ScreenFadeEffect()
{
}

ScreenFadeEffect::ScreenFadeEffect(ScreenFadeEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

ScreenFadeEffect & ScreenFadeEffect::operator=(ScreenFadeEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

ScreenFadeEffect & ScreenFadeEffect::Get()
{
	if (!pInstance)
		throw std::exception("ScreenFadeEffect needs an instance!");
	return *pInstance;
}

bool ScreenFadeEffect::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->cBufferPtrs.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	ComPtr<ID3DBlob> blob;

	// ******************
	// ����������ɫ��
	//

	HR(CreateShaderFromFile(L"HLSL\\ScreenFade_VS.cso", L"HLSL\\ScreenFade_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->screenFadeVS.GetAddressOf()));
	// �������㲼��
	HR(device->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexPosTexLayout.GetAddressOf()));

	// ******************
	// ����������ɫ��
	//

	HR(CreateShaderFromFile(L"HLSL\\ScreenFade_PS.cso", L"HLSL\\ScreenFade_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->screenFadePS.GetAddressOf()));



	pImpl->cBufferPtrs.assign({
		&pImpl->cbFrame,
		&pImpl->cbRarely
		});

	// ��������������
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	return true;
}

void ScreenFadeEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->vertexPosTexLayout.Get());
	deviceContext->VSSetShader(pImpl->screenFadeVS.Get(), nullptr, 0);
	deviceContext->PSSetShader(pImpl->screenFadePS.Get(), nullptr, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV ScreenFadeEffect::SetWorldViewProjMatrix(DirectX::FXMMATRIX W, DirectX::CXMMATRIX V, DirectX::CXMMATRIX P)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.worldViewProj = XMMatrixTranspose(W * V * P);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV ScreenFadeEffect::SetWorldViewProjMatrix(DirectX::FXMMATRIX WVP)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.worldViewProj = XMMatrixTranspose(WVP);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void ScreenFadeEffect::SetFadeAmount(float fadeAmount)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.fadeAmount = fadeAmount;
	pImpl->isDirty = cBuffer.isDirty = true;
}


void ScreenFadeEffect::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	pImpl->texture = texture;
}

void ScreenFadeEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// ���������󶨵���Ⱦ������
	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	// ����SRV
	deviceContext->PSSetShaderResources(0, 1, pImpl->texture.GetAddressOf());

	if (pImpl->isDirty)
	{
		pImpl->isDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}
