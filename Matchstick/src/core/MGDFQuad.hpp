#pragma once

#include <D3D11.h>
#include <DirectXMath.h>

using namespace DirectX;

namespace MGDF { namespace core {

class Quad
{
public:
	Quad(ID3D11Device *device);	
	virtual ~Quad();
	void Resize(ID3D11DeviceContext *context,float topX,float topY,float width,float height,float screenX,float screenY);
	void Draw(ID3D11DeviceContext *context,const XMFLOAT4 &color);
private:
	ID3D11Buffer* _positionBuffer;
	ID3D11Buffer * _pixelShaderCBuffer;
	ID3D11PixelShader *_pixelShader;
	ID3D11VertexShader *_vertexShader;
	ID3D11RasterizerState *_rasterizerState;
	ID3D11BlendState *_blendState;
	ID3D11DepthStencilState *_depthStencilState;
	ID3D11InputLayout* _layout;

	void SaveState(ID3D11DeviceContext *context);
	void RestoreState(ID3D11DeviceContext *context);

	//used for saving and restoring existing device state
	ID3D11RasterizerState *_currentRasterizerState;
	ID3D11BlendState *_currentBlendState;
	ID3D11DepthStencilState *_currentDepthStencilState;
	float _currentBlendFactor[4];
	UINT _currentSampleMask;
	UINT _currentStencilRef;
	ID3D11VertexShader *_currentVertexShader;
	ID3D11PixelShader *_currentPixelShader;
	ID3D11ClassInstance	*_currentVertexShaderClassInstances[256];
	ID3D11ClassInstance	*_currentPixelShaderClassInstances[256];
	UINT _currentVertexShaderNumClassInstances;		
	UINT _currentPixelShaderNumClassInstances;
	ID3D11InputLayout* _currentInputLayout;
	D3D11_PRIMITIVE_TOPOLOGY _currentPrimitiveTopology;
	ID3D11Buffer *_currentPSCBuffer;
	ID3D11Buffer *_currentVertexBuffer;
	UINT _currentStride;
	UINT _currentOffset;
};

}}
