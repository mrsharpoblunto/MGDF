#pragma once

#include <d3d11.h>
#include <dwrite_1.h>
#include "Common.hpp"
#include <memory>
#include <MGDF/MGDF.hpp>

namespace MGDF
{
namespace Test
{

enum TextColor
{
	GREEN,
	RED,
};

typedef struct {
	TextColor Color;
	std::string Content;

	TextColor StatusColor;
	std::string StatusText;
} Line;

class TextManager;

class TESTCOMMON_DLL TextManagerState
{
	friend class TextManager;
public:
	virtual ~TextManagerState() {};
	TextManagerState() {};
	TextManagerState( const TextManagerState *state );
	void AddLine( const std::string &line );
	void SetStatus( TextColor color, const std::string &text );
	std::shared_ptr<TextManagerState> Interpolate( const TextManagerState *state, double alpha );
private:
#pragma warning(push)
#pragma warning(disable: 4251)
	std::vector<Line> _lines;
#pragma warning(pop)
};

class TESTCOMMON_DLL TextManager
{
public:
	virtual ~TextManager();
	TextManager( IRenderHost *renderHost );
	void SetState( std::shared_ptr<TextManagerState> state );
	void BackBufferChange();
	void BeforeBackBufferChange();
	void BeforeDeviceReset();
	void DrawText();
private:
#pragma warning(push)
#pragma warning(disable: 4251)
	std::shared_ptr<TextManagerState> _state;
#pragma warning(pop)
	IRenderHost *_renderHost;
	ID2D1SolidColorBrush *_whiteBrush;
	ID2D1SolidColorBrush *_redBrush;
	ID2D1SolidColorBrush *_greenBrush;
	ID2D1DeviceContext *_d2dContext;
	IDWriteFactory1 *_dWriteFactory;
	IDWriteTextFormat *_textFormat;
	ID3D11DeviceContext *_immediateContext;
};
}
}