// CFW1GlyphRenderStates.cpp

#include "FW1Precompiled.h"

#include "CFW1GlyphRenderStates.h"

#define SAFE_RELEASE(pObject) { if(pObject) { (pObject)->Release(); (pObject) = NULL; } }


namespace FW1FontWrapper {


// Construct
CFW1GlyphRenderStates::CFW1GlyphRenderStates() :
	m_pDevice(NULL),
	m_featureLevel(D3D_FEATURE_LEVEL_9_1),
	
	m_pVertexShaderQuad(NULL),
	m_pVertexShaderClipQuad(NULL),
	m_pQuadInputLayout(NULL),
	
	m_pVertexShaderPoint(NULL),
	m_pPointInputLayout(NULL),
	m_pGeometryShaderPoint(NULL),
	m_pGeometryShaderClipPoint(NULL),
	m_hasGeometryShader(false),
	
	m_pPixelShader(NULL),
	m_pPixelShaderClip(NULL),
	
	m_pConstantBuffer(NULL),
	
	m_pBlendState(NULL),
	m_pSamplerState(NULL),
	m_pRasterizerState(NULL),
	m_pDepthStencilState(NULL)
{
}


// Destruct
CFW1GlyphRenderStates::~CFW1GlyphRenderStates() {
	SAFE_RELEASE(m_pDevice);
	
	SAFE_RELEASE(m_pVertexShaderQuad);
	SAFE_RELEASE(m_pVertexShaderClipQuad);
	SAFE_RELEASE(m_pQuadInputLayout);
	
	SAFE_RELEASE(m_pVertexShaderPoint);
	SAFE_RELEASE(m_pPointInputLayout);
	SAFE_RELEASE(m_pGeometryShaderPoint);
	SAFE_RELEASE(m_pGeometryShaderClipPoint);
	
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pPixelShaderClip);
	
	SAFE_RELEASE(m_pConstantBuffer);
	
	SAFE_RELEASE(m_pBlendState);
	SAFE_RELEASE(m_pSamplerState);
	SAFE_RELEASE(m_pRasterizerState);
	SAFE_RELEASE(m_pDepthStencilState);
}


// Init
HRESULT CFW1GlyphRenderStates::initRenderResources(
	IFW1Factory *pFW1Factory,
	ID3D11Device *pDevice,
	bool wantGeometryShader,
	bool anisotropicFiltering
) {
	HRESULT hResult = initBaseObject(pFW1Factory);
	if(FAILED(hResult))
		return hResult;
	
	if(pDevice == NULL)
		return E_INVALIDARG;
	
	pDevice->AddRef();
	m_pDevice = pDevice;
	m_featureLevel = m_pDevice->GetFeatureLevel();
	
	hResult = S_OK;
	
	// Create all needed resources
	if(SUCCEEDED(hResult))
		hResult = createQuadShaders();
	if(SUCCEEDED(hResult))
		hResult = createPixelShaders();
	if(SUCCEEDED(hResult))
		hResult = createConstantBuffer();
	if(SUCCEEDED(hResult))
		hResult = createRenderStates(anisotropicFiltering);
	if(SUCCEEDED(hResult) && wantGeometryShader) {
		hResult = createGlyphShaders();
		if(FAILED(hResult))
			hResult = S_OK;
	}
	
	if(SUCCEEDED(hResult))
		hResult = S_OK;
	
	return hResult;
}


// Create quad shaders
HRESULT CFW1GlyphRenderStates::createQuadShaders() {
	
	// Vertex shaders
#include "vsSimpleStr.h"
#include "vsClipStr.h"

	// Create vertex shader
	ID3D11VertexShader *pVS;
		
	HRESULT hResult = m_pDevice->CreateVertexShader(vsSimpleStr,sizeof(vsSimpleStr), NULL, &pVS);
	if(FAILED(hResult)) {
		m_lastError = L"Failed to create vertex shader";
	}
	else {
		// Create vertex shader
		ID3D11VertexShader *pVSClip;
				
		hResult = m_pDevice->CreateVertexShader(
			vsClipStr,
			sizeof(vsClipStr),
			NULL,
			&pVSClip
		);
		if(FAILED(hResult)) {
			m_lastError = L"Failed to create clipping vertex shader";
		}
		else {
			// Create input layout
			ID3D11InputLayout *pInputLayout;
					
			// Quad vertex input layout
			D3D11_INPUT_ELEMENT_DESC inputElements[] = {
				{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"GLYPHCOLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
			};
					
			hResult = m_pDevice->CreateInputLayout(
				inputElements,
				2,
				vsSimpleStr,
				sizeof(vsSimpleStr),
				&pInputLayout
			);
			if(FAILED(hResult)) {
				m_lastError = L"Failed to create input layout";
			}
			else {
				// Success
				m_pVertexShaderQuad = pVS;
				m_pVertexShaderClipQuad = pVSClip;
				m_pQuadInputLayout = pInputLayout;
						
				hResult = S_OK;
			}
					
			if(FAILED(hResult))
				pVSClip->Release();
		}
			
		if(FAILED(hResult))
			pVS->Release();
	}
	
	return hResult;
}

// Create point to quad geometry shader
HRESULT CFW1GlyphRenderStates::createGlyphShaders() {
	if(m_featureLevel < D3D_FEATURE_LEVEL_10_0)
		return E_FAIL;
	
	// Geometry shader constructing glyphs from point input and texture buffer
#include "gsSimpleStr.h"

	// Geometry shader with rect clipping
#include "gsClipStr.h"

	// Vertex shader
#include "vsEmptyStr.h"

	// Create geometry shader
	ID3D11GeometryShader *pGS;
		
	HRESULT hResult = m_pDevice->CreateGeometryShader(gsSimpleStr, sizeof(gsSimpleStr), NULL, &pGS);
	if(FAILED(hResult)) {
		m_lastError = L"Failed to create geometry shader";
	}
	else {
	
		// Create clipping geometry shader
		ID3D11GeometryShader *pGSClip;
				
		hResult = m_pDevice->CreateGeometryShader(
			gsClipStr,
			sizeof(gsClipStr),
			NULL,
			&pGSClip
		);
		if(FAILED(hResult)) {
			m_lastError = L"Failed to create clipping geometry shader";
		}
		else {
			// Create vertex shader
			ID3D11VertexShader *pVSEmpty;
						
			hResult = m_pDevice->CreateVertexShader(
				vsEmptyStr,
				sizeof(vsEmptyStr),
				NULL,
				&pVSEmpty
			);
			if(FAILED(hResult)) {
				m_lastError = L"Failed to create empty vertex shader";
			}
			else {
				ID3D11InputLayout *pInputLayout;
							
				// Input layout for geometry shader
				D3D11_INPUT_ELEMENT_DESC inputElements[] = {
					{"POSITIONINDEX", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
					{"GLYPHCOLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
				};
							
				hResult = m_pDevice->CreateInputLayout(
					inputElements,
					2,
					vsEmptyStr,
					sizeof(vsEmptyStr),
					&pInputLayout
				);
				if(FAILED(hResult)) {
					m_lastError = L"Failed to create input layout for geometry shader";
				}
				else {
					// Success
					m_pVertexShaderPoint = pVSEmpty;
					m_pGeometryShaderPoint = pGS;
					m_pGeometryShaderClipPoint = pGSClip;
					m_pPointInputLayout = pInputLayout;
					m_hasGeometryShader = true;
								
					hResult = S_OK;
				}
							
				if(FAILED(hResult))
					pVSEmpty->Release();
			}

			if(FAILED(hResult))
				pGSClip->Release();
		}
			
		if(FAILED(hResult))
			pGS->Release();
	}

	
	return hResult;
}

// Create pixel shaders
HRESULT CFW1GlyphRenderStates::createPixelShaders() {
	
	// Pixel shader
#include "psStr.h"
	
	// Clipping pixel shader
#include "psClipStr.h"

	// Create pixel shader
	ID3D11PixelShader *pPS;
		
	HRESULT hResult = m_pDevice->CreatePixelShader(psStr, sizeof(psStr), NULL, &pPS);
	if(FAILED(hResult)) {
		m_lastError = L"Failed to create pixel shader";
	}
	else {
		// Create pixel shader
		ID3D11PixelShader *pPSClip;
				
		hResult = m_pDevice->CreatePixelShader(
			psClipStr,
			sizeof(psClipStr),
			NULL, &pPSClip
		);
		if(FAILED(hResult)) {
			m_lastError = L"Failed to create clipping pixel shader";
		}
		else {
			// Success
			m_pPixelShader = pPS;
			m_pPixelShaderClip = pPSClip;
					
			hResult = S_OK;
		}
			
		if(FAILED(hResult))
			pPS->Release();
	}
	
	return hResult;
}


// Create constant buffer
HRESULT CFW1GlyphRenderStates::createConstantBuffer() {
	// Create constant buffer
	D3D11_BUFFER_DESC constantBufferDesc;
	ID3D11Buffer *pConstantBuffer;
			
	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
	constantBufferDesc.ByteWidth = sizeof(ShaderConstants);
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			
	HRESULT hResult = m_pDevice->CreateBuffer(&constantBufferDesc, NULL, &pConstantBuffer);
	if(FAILED(hResult)) {
		m_lastError = L"Failed to create constant buffer";
	}
	else {
		// Success
		m_pConstantBuffer = pConstantBuffer;
				
		hResult = S_OK;
	}

	return hResult;
}


// Create render states
HRESULT CFW1GlyphRenderStates::createRenderStates(bool anisotropicFiltering) {
	// Create blend-state
	D3D11_BLEND_DESC blendDesc;
	ID3D11BlendState *pBlendState;
	
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	for(int i=0; i < 4; ++i) {
		blendDesc.RenderTarget[i].BlendEnable = TRUE;
		blendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}
	
	HRESULT hResult = m_pDevice->CreateBlendState(&blendDesc, &pBlendState);
	if(FAILED(hResult)) {
		m_lastError = L"Failed to create blend state";
	}
	else {
		// Create sampler state
		D3D11_SAMPLER_DESC samplerDesc;
		ID3D11SamplerState *pSamplerState;
		
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));
		if(anisotropicFiltering) {
			samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
			samplerDesc.MaxAnisotropy = 2;
			if(m_featureLevel >= D3D_FEATURE_LEVEL_9_2)
				samplerDesc.MaxAnisotropy = 5;
		}
		else
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		
		hResult = m_pDevice->CreateSamplerState(&samplerDesc, &pSamplerState);
		if(FAILED(hResult)) {
			m_lastError = L"Failed to create sampler state";
		}
		else {
			// Create rasterizer state
			D3D11_RASTERIZER_DESC rasterizerDesc;
			ID3D11RasterizerState *pRasterizerState;
			
			ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
			rasterizerDesc.FillMode = D3D11_FILL_SOLID;
			rasterizerDesc.CullMode = D3D11_CULL_NONE;
			rasterizerDesc.FrontCounterClockwise = FALSE;
			rasterizerDesc.DepthClipEnable = TRUE;
			
			hResult = m_pDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);
			if(FAILED(hResult)) {
				m_lastError = L"Failed to create rasterizer state";
			}
			else {
				// Create depth-stencil state
				D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
				ID3D11DepthStencilState *pDepthStencilState;
				
				ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
				depthStencilDesc.DepthEnable = FALSE;
				
				hResult = m_pDevice->CreateDepthStencilState(&depthStencilDesc, &pDepthStencilState);
				if(FAILED(hResult)) {
					m_lastError = L"Failed to create depth stencil state";
				}
				else {
					// Success
					m_pBlendState = pBlendState;
					m_pSamplerState = pSamplerState;
					m_pRasterizerState = pRasterizerState;
					m_pDepthStencilState = pDepthStencilState;
					
					hResult = S_OK;
				}
				
				if(FAILED(hResult))
					pRasterizerState->Release();
			}
			
			if(FAILED(hResult))
				pSamplerState->Release();
		}
		
		if(FAILED(hResult))
			pBlendState->Release();
	}
	
	return hResult;
}


}// namespace FW1FontWrapper
