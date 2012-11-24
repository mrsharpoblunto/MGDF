#include "StdAfx.h"

#include <fstream>
#include <sstream>

#include "MGDFQuad.hpp"
#include "common\MGDFResources.hpp"

namespace MGDF { namespace core {

D3D11_INPUT_ELEMENT_DESC layoutDescription[] =
{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

Quad::Quad(ID3D11Device *device)
{
	{
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;

		XMFLOAT3 quadPosition[4];
		quadPosition[0] = XMFLOAT3(1.0f, 1.0f,1.0f);
		quadPosition[1] = XMFLOAT3(1.0f,-1.0f, 1.0f);
		quadPosition[2] = XMFLOAT3(-1.0f,1.0f,1.0f);
		quadPosition[3] = XMFLOAT3(-1.0f, -1.0f, 1.0f);

		D3D11_SUBRESOURCE_DATA data;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		desc.ByteWidth = sizeof(XMFLOAT3) * 4;
		data.pSysMem = &quadPosition;
		HR(device->CreateBuffer(&desc,&data,&_positionBuffer));
	}

	{
		D3D11_RASTERIZER_DESC desc;
		ZeroMemory(&desc,sizeof(D3D11_RASTERIZER_DESC));
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = true;
		HR(device->CreateRasterizerState(&desc,&_rasterizerState));
	}

	{
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory(&dsDesc,sizeof(D3D11_DEPTH_STENCIL_DESC));
		dsDesc.DepthEnable = false;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.StencilEnable = false;
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		HR(device->CreateDepthStencilState(&dsDesc,&_depthStencilState));
	}

	{
		D3D11_BLEND_DESC bDesc;
		bDesc.AlphaToCoverageEnable = false;
		bDesc.IndependentBlendEnable = false;
		bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bDesc.RenderTarget[0].BlendEnable = true;
		bDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		bDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		bDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HR(device->CreateBlendState(&bDesc,&_blendState));
	}

	{
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc,sizeof(D3D11_BUFFER_DESC));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = sizeof(XMFLOAT4);
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		device->CreateBuffer(&desc,nullptr,&_pixelShaderCBuffer);
	}

	std::string pixelShaderData;
	std::string vertexShaderData;
	{
		std::wstring pixelShader = Resources::Instance().RootDir()+L"resources/quad_ps.cso";
		std::ifstream input(pixelShader.c_str(),std::ios::in);
		std::stringstream buffer;
		buffer << input.rdbuf();
		pixelShaderData = buffer.str();
		HR(device->CreatePixelShader(pixelShaderData.c_str(),pixelShaderData.size(),nullptr,&_pixelShader));
	}
	{
		std::wstring vertexShader = Resources::Instance().RootDir()+L"resources/quad_vs.cso";
		std::ifstream input(vertexShader.c_str(),std::ios::in);
		std::stringstream buffer;
		buffer << input.rdbuf();
		vertexShaderData = buffer.str();
		HR(device->CreateVertexShader(vertexShaderData.c_str(),vertexShaderData.size(),nullptr,&_vertexShader));
	}

	HR(device->CreateInputLayout(layoutDescription,1,vertexShaderData.c_str(),vertexShaderData.size(),&_layout));
}

Quad::~Quad()
{
	SAFE_RELEASE(_positionBuffer);
	SAFE_RELEASE(_rasterizerState);
	SAFE_RELEASE(_depthStencilState);
	SAFE_RELEASE(_blendState);
	SAFE_RELEASE(_layout);
	SAFE_RELEASE(_vertexShader);
	SAFE_RELEASE(_pixelShader);
	SAFE_RELEASE(_pixelShaderCBuffer);
}

void Quad::Resize(ID3D11DeviceContext *context,float topX,float topY,float width,float height,float screenX,float screenY)
{
	float fx = 2.0f/screenX;
	float fy = 2.0f/screenY;

	D3D11_MAPPED_SUBRESOURCE data;
	HR(context->Map(_positionBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&data));

	XMFLOAT3 *quadPosition = (XMFLOAT3 *)data.pData;
	quadPosition[0] = XMFLOAT3((topX + width) * fx - 1.0f, (2.0f - topY * fy) - 1.0f,1.0f);
	quadPosition[1] = XMFLOAT3((topX + width) * fx - 1.0f, (2.0f - (topY +height) * fy) - 1.0f, 1.0f);
	quadPosition[2] = XMFLOAT3(topX * fx - 1.0f, (2.0f - topY * fy) - 1.0f,1.0f);
	quadPosition[3] = XMFLOAT3(topX * fx - 1.0f, (2.0f - (topY +height) * fy) - 1.0f, 1.0f);

	context->Unmap(_positionBuffer,0);
}

void Quad::Draw(ID3D11DeviceContext *context,const XMFLOAT4 &color)
{
	SaveState(context);

	//setup the geometry
	context->IASetInputLayout(_layout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	const UINT32 offsets = 0;
	const UINT32 strides = sizeof(XMFLOAT3);
	context->IASetVertexBuffers(0,1,&_positionBuffer,&strides,&offsets);

	//copy the color into the pixel shaders constant buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HR(context->Map(_pixelShaderCBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource));
	*((XMFLOAT4 *)mappedResource.pData) = color;
	context->Unmap(_pixelShaderCBuffer,0);
	context->PSSetConstantBuffers(0,1,&_pixelShaderCBuffer);

	//setup the shaders
	context->VSSetShader(_vertexShader,nullptr,0);
	context->PSSetShader(_pixelShader,nullptr,0);

	//set up the new render states
	context->OMSetBlendState(_blendState,nullptr,0xffffffff);
	context->RSSetState(_rasterizerState);
	context->OMSetDepthStencilState(_depthStencilState,0);

	context->Draw(4,0);

	RestoreState(context);
}

void Quad::SaveState(ID3D11DeviceContext *context)
{
	context->OMGetBlendState(&_currentBlendState,_currentBlendFactor,&_currentSampleMask);
	context->RSGetState(&_currentRasterizerState);
	context->OMGetDepthStencilState(&_currentDepthStencilState,&_currentStencilRef);
	_currentPixelShaderNumClassInstances = 256;
	context->PSGetShader(&_currentPixelShader,_currentPixelShaderClassInstances,&_currentPixelShaderNumClassInstances);
	_currentVertexShaderNumClassInstances = 256;
	context->VSGetShader(&_currentVertexShader,_currentVertexShaderClassInstances,&_currentVertexShaderNumClassInstances);
	context->IAGetInputLayout(&_currentInputLayout);
	context->IAGetPrimitiveTopology(&_currentPrimitiveTopology);
	context->PSGetConstantBuffers(0,1,&_currentPSCBuffer);
	context->IAGetVertexBuffers(0,1,&_currentVertexBuffer,&_currentStride,&_currentOffset);

}

void Quad::RestoreState(ID3D11DeviceContext *context)
{
	context->OMSetBlendState(_currentBlendState,_currentBlendFactor,_currentSampleMask);
	context->RSSetState(_currentRasterizerState);
	context->OMSetDepthStencilState(_currentDepthStencilState,_currentStencilRef);
	context->PSSetShader(_currentPixelShader,_currentPixelShaderClassInstances,_currentPixelShaderNumClassInstances);
	context->VSSetShader(_currentVertexShader,_currentVertexShaderClassInstances,_currentVertexShaderNumClassInstances);
	context->IASetInputLayout(_currentInputLayout);
	context->IASetPrimitiveTopology(_currentPrimitiveTopology);
	context->PSSetConstantBuffers(0,1,&_currentPSCBuffer);
	context->IASetVertexBuffers(0,1,&_currentVertexBuffer,&_currentStride,&_currentOffset);

	SAFE_RELEASE(_currentBlendState);
	SAFE_RELEASE(_currentRasterizerState);
	SAFE_RELEASE(_currentDepthStencilState);
	SAFE_RELEASE(_currentPixelShader);
	for (UINT32 i = 0;i < _currentPixelShaderNumClassInstances;++i)
	{
		SAFE_RELEASE(_currentPixelShaderClassInstances[i]);
	}
	SAFE_RELEASE(_currentVertexShader);
	for (UINT32 i = 0;i < _currentVertexShaderNumClassInstances;++i)
	{
		SAFE_RELEASE(_currentVertexShaderClassInstances[i]);
	}
	SAFE_RELEASE(_currentInputLayout);
	SAFE_RELEASE(_currentPSCBuffer);
	SAFE_RELEASE(_currentVertexBuffer);
}

}}
