#include "StdAfx.h"

#include "TextManager.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace Test
{

#define MAX_LINES 100

TextManagerState::TextManagerState( const TextManagerState *state )
{
	for ( auto line : state->_lines ) {
		_lines.push_back( line );
	}
}

void TextManagerState::AddLine( const std::string &line )
{
	Line l;
	l.Content = line;

	if ( _lines.size() == MAX_LINES ) {
		_lines.pop_back();
	}
	_lines.insert( _lines.begin(), l );
}

void TextManagerState::SetStatus( TextColor color, const std::string &text )
{
	_lines[0].StatusText = text;
	_lines[0].StatusColor = color;
}

boost::shared_ptr<TextManagerState> TextManagerState::Interpolate( const TextManagerState *endState, double alpha )
{
	//interpolation isn't really possible with this type of gamestate, so just use the most recent.
	return boost::shared_ptr<TextManagerState> ( new TextManagerState( endState ) );
}

TextManager::~TextManager()
{
	SAFE_RELEASE( _whiteBrush );
	SAFE_RELEASE( _redBrush );
	SAFE_RELEASE( _greenBrush );
	SAFE_RELEASE( _d2dContext );
	SAFE_RELEASE( _dWriteFactory );
	SAFE_RELEASE( _textFormat );
	SAFE_RELEASE( _immediateContext );
}

void TextManager::BackBufferChanged()
{
	_system->GetGraphics()->SetBackBufferRenderTarget( _d2dContext );
}

TextManager::TextManager( ISystem *system )
	: _system( system )
	, _whiteBrush(nullptr)
	, _redBrush(nullptr)
	, _greenBrush(nullptr)
	, _d2dContext(nullptr)
	, _dWriteFactory(nullptr)
	, _textFormat(nullptr)
	, _immediateContext(nullptr)
{
	system->GetGraphics()->GetD3DDevice()->GetImmediateContext( &_immediateContext );
	if ( FAILED( system->GetGraphics()->GetD2DDevice()->CreateDeviceContext( D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_d2dContext ) ) )
	{
		FATALERROR( system, "Unable to create ID2D1DeviceContext" );
	}
	BackBufferChanged();

	if ( FAILED( DWriteCreateFactory(
	            DWRITE_FACTORY_TYPE_SHARED,
	            __uuidof( IDWriteFactory1 ),
	            reinterpret_cast<IUnknown**>( &_dWriteFactory )
	        ) ) ) {
		FATALERROR( system, "Unable to create IDWriteFactory" );
	}

	IDWriteFontCollection *fontCollection;
	if (FAILED(_dWriteFactory->GetSystemFontCollection( &fontCollection ))) {
		FATALERROR( system, "Unable to get system font collection" );
	}

	if (FAILED(_dWriteFactory->CreateTextFormat( 
		L"Arial",
		fontCollection,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		22,L"",&_textFormat))) {
		FATALERROR( system, "Unable to create text format" );
	}

	D2D1_COLOR_F color;
	color.a = color.r = color.g = color.b = 1.0f;
	if (FAILED(_d2dContext->CreateSolidColorBrush(color,&_whiteBrush))) {
		FATALERROR(system,"Unable to create white color brush");
	}

	color.g = color.b = 0.5f;
	if (FAILED(_d2dContext->CreateSolidColorBrush(color,&_redBrush))) {
		FATALERROR(system,"Unable to create red color brush");
	}

	color.g = 1.0f;
	color.r = 0.5f;
	if (FAILED(_d2dContext->CreateSolidColorBrush(color,&_greenBrush))) {
		FATALERROR(system,"Unable to create green color brush");
	}

	SAFE_RELEASE(fontCollection);
}

void TextManager::SetState( boost::shared_ptr<TextManagerState> state )
{
	_state = state;
}

void TextManager::DrawText()
{
	if ( _state ) {
		INT32 starty;
		if ( _state.get()->_lines.size() * 25 < _system->GetGraphics()->GetScreenY() ) {
			starty = ( static_cast<UINT32>( _state.get()->_lines.size() ) * 25 ) - 25;
		} else {
			starty = _system->GetGraphics()->GetScreenY() - 25;
		}

		_d2dContext->BeginDraw();

		for ( auto &line : _state.get()->_lines ) {
			std::wstring content;
			content.assign( line.Content.begin(), line.Content.end() );

			IDWriteTextLayout *textLayout;
			if (FAILED(_dWriteFactory->CreateTextLayout(
					content.c_str(),
					static_cast<UINT32>(content.size()),
					_textFormat,
					static_cast<float>(_system->GetGraphics()->GetCurrentAdaptorMode()->GetWidth()),
					static_cast<float>(_system->GetGraphics()->GetCurrentAdaptorMode()->GetHeight()),
					&textLayout))) {
				FATALERROR(_system,"Unable to create text layout");
			}

			D2D_POINT_2F origin;
			origin.x = 0;
			origin.y = static_cast<float>( starty );

			_d2dContext->DrawTextLayout(origin,textLayout,_whiteBrush);

			SAFE_RELEASE( textLayout );

			if ( line.StatusText != "" ) {
				std::wstring statusText;
				statusText.assign( line.StatusText.begin(), line.StatusText.end() );

				if (FAILED(_dWriteFactory->CreateTextLayout(
						statusText.c_str(),
						static_cast<UINT32>(statusText.size()),
						_textFormat,
						static_cast<float>(_system->GetGraphics()->GetCurrentAdaptorMode()->GetWidth()),
						static_cast<float>(_system->GetGraphics()->GetCurrentAdaptorMode()->GetHeight()),
						&textLayout))) {
					FATALERROR(_system,"Unable to create text layout");
				}

				origin.x = static_cast<float>( _system->GetGraphics()->GetScreenX() ) - 150.0f;
				origin.y = static_cast<float>( starty );

				_d2dContext->DrawTextLayout(origin,textLayout,line.StatusColor == GREEN ? _greenBrush : _redBrush );

				SAFE_RELEASE(textLayout);
			}

			starty -= 25;
			if ( starty <= -25 ) break;
		}

		_d2dContext->EndDraw();
	}
}

}
}