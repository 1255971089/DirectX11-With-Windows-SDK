#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// ��������Effects.h��d3dUtil.h����
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;

//
// BasicEffect::Impl ��Ҫ����BasicEffect�Ķ���
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:

	//
	// ��Щ�ṹ���ӦHLSL�Ľṹ�塣��Ҫ��16�ֽڶ���
	//

	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
		Material material;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMVECTOR eyePos;
	};

	struct CBDrawingStates
	{
		DirectX::XMVECTOR fogColor;
		int fogEnabled;
		float fogStart;
		float fogRange;
		float pad;
	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};


	struct CBChangesRarely
	{
		DirectionalLight dirLight[BasicEffect::maxLights];
		PointLight pointLight[BasicEffect::maxLights];
		SpotLight spotLight[BasicEffect::maxLights];
	};

public:
	// ������ʽָ��
	Impl() = default;
	~Impl() = default;

public:
	// ��Ҫ16�ֽڶ�������ȷ���ǰ��
	CBufferObject<0, CBChangesEveryDrawing> cbDrawing;		// ÿ�ζ�����Ƶĳ���������
	CBufferObject<1, CBChangesEveryFrame>   cbFrame;		// ÿ֡���Ƶĳ���������
	CBufferObject<2, CBDrawingStates>       cbStates;		// ÿ�λ���״̬����ĳ���������
	CBufferObject<3, CBChangesOnResize>     cbOnResize;		// ÿ�δ��ڴ�С����ĳ���������
	CBufferObject<4, CBChangesRarely>		cbRarely;		// �����������ĳ���������
	BOOL isDirty;											// �Ƿ���ֵ���
	std::vector<CBufferBase*> cBufferPtrs;					// ͳһ�����������еĳ���������


	ComPtr<ID3D11VertexShader> basicVS;
	ComPtr<ID3D11PixelShader> basicPS;

	ComPtr<ID3D11VertexShader> billboardVS;
	ComPtr<ID3D11GeometryShader> billboardGS;
	ComPtr<ID3D11PixelShader> billboardPS;


	ComPtr<ID3D11InputLayout> vertexPosSizeLayout;			// �㾫�����벼��
	ComPtr<ID3D11InputLayout> vertexPosNormalTexLayout;		// 3D�������벼��

	ComPtr<ID3D11ShaderResourceView> texture;				// ���ڻ��Ƶ�����
	ComPtr<ID3D11ShaderResourceView> textures;				// ���ڻ��Ƶ���������
};

//
// BasicEffect
//

namespace
{
	// BasicEffect����
	static BasicEffect * pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
	if (pInstance)
		throw std::exception("BasicEffect is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect & BasicEffect::Get()
{
	if (!pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *pInstance;
}


bool BasicEffect::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->cBufferPtrs.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	ComPtr<ID3DBlob> blob;

	// ******************
	// ����3D����
	//
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS.cso", L"HLSL\\Basic_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->basicVS.GetAddressOf()));
	// �����������벼��
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), pImpl->vertexPosNormalTexLayout.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS.cso", L"HLSL\\Basic_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->basicPS.GetAddressOf()));


	// ******************
	// ���ƹ����
	//
	HR(CreateShaderFromFile(L"HLSL\\Billboard_VS.cso", L"HLSL\\Billboard_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->billboardVS.GetAddressOf()));
	// �����������벼��
	HR(device->CreateInputLayout(VertexPosSize::inputLayout, ARRAYSIZE(VertexPosSize::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), pImpl->vertexPosSizeLayout.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Billboard_GS.cso", L"HLSL\\Billboard_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->billboardGS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Billboard_PS.cso", L"HLSL\\Billboard_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->billboardPS.GetAddressOf()));


	pImpl->cBufferPtrs.assign({
		&pImpl->cbDrawing, 
		&pImpl->cbFrame, 
		&pImpl->cbStates, 
		&pImpl->cbOnResize, 
		&pImpl->cbRarely});

	// ��������������
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	return true;
}

void BasicEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalTexLayout.Get());
	deviceContext->VSSetShader(pImpl->basicVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->basicPS.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicEffect::SetRenderBillboard(ComPtr<ID3D11DeviceContext> deviceContext, bool enableAlphaToCoverage)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosSizeLayout.Get());
	deviceContext->VSSetShader(pImpl->billboardVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->billboardGS.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->billboardPS.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(
		(enableAlphaToCoverage ? RenderStates::BSAlphaToCoverage.Get() : nullptr),
		nullptr, 0xFFFFFFFF);

}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->cbDrawing;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixInverse(nullptr, W);	// ����ת�õ���
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->cbOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->cbDrawing;
	cBuffer.data.material = material;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	pImpl->texture = texture;
}

void BasicEffect::SetTextureArray(ComPtr<ID3D11ShaderResourceView> textures)
{
	pImpl->textures = textures;
}

void XM_CALLCONV BasicEffect::SetEyePos(FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->isDirty = cBuffer.isDirty = true;
}



void BasicEffect::SetFogState(bool isOn)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.fogEnabled = isOn;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogStart(float fogStart)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.fogStart = fogStart;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogColor(DirectX::XMVECTOR fogColor)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.fogColor = fogColor;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogRange(float fogRange)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.fogRange = fogRange;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// ���������󶨵���Ⱦ������
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	pCBuffers[3]->BindVS(deviceContext);

	pCBuffers[0]->BindGS(deviceContext);
	pCBuffers[1]->BindGS(deviceContext);
	pCBuffers[3]->BindGS(deviceContext);

	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);
	pCBuffers[4]->BindPS(deviceContext);

	// ��������
	deviceContext->PSSetShaderResources(0, 1, pImpl->texture.GetAddressOf());
	deviceContext->PSSetShaderResources(1, 1, pImpl->textures.GetAddressOf());

	if (pImpl->isDirty)
	{
		pImpl->isDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}

