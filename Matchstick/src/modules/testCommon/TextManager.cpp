#include "StdAfx.h"

#include "TextManager.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

#define MAX_LINES 100

TextManagerState::TextManagerState(const TextManagerState *state)
{
	for (std::vector<Line>::const_iterator iter=state->_lines.begin();iter!=state->_lines.end();++iter)
	{
		_lines.push_back(*iter);
	}
}

void TextManagerState::AddLine(unsigned int color,const std::string &line)
{
	Line l;
	l.Color = color;
	l.Content = line;

	if (_lines.size()==MAX_LINES)
	{
		_lines.pop_back();
	}
	_lines.insert(_lines.begin(),l);
}

void TextManagerState::SetStatus(unsigned int color,const std::string &text)
{
	_lines[0].StatusText = text;
	_lines[0].StatusColor = color;
}

boost::shared_ptr<TextManagerState> TextManagerState::Interpolate(const TextManagerState *endState,double alpha)
{
	//interpolation isn't really possible with this type of gamestate, so just use the most recent.
	return boost::shared_ptr<TextManagerState>(new TextManagerState(endState));
}

TextManager::~TextManager()
{
	if(_font){
      _font->Release();
      _font=NULL;
   }
}

TextManager::TextManager(ISystem *system)
{
	_system = system;
	_font=NULL;

	HRESULT hr=D3DXCreateFont(_system->GetD3DDevice(),     //D3D Device
                     22,               //Font height
                     0,                //Font width
                     FW_NORMAL,        //Font Weight
                     1,                //MipLevels
                     false,            //Italic
                     DEFAULT_CHARSET,  //CharSet
                     OUT_DEFAULT_PRECIS, //OutputPrecision
                     ANTIALIASED_QUALITY, //Quality
                     DEFAULT_PITCH|FF_DONTCARE,//PitchAndFamily
                     "Arial",          //pFacename,
                     &_font);         //ppFont
}

void TextManager::SetState(boost::shared_ptr<TextManagerState> state)
{
	_state = state;
}

void TextManager::DrawText()
{
	if (_state)
	{
		RECT font_rect;

		SetRect(&font_rect,0,0,_system->GetGraphics()->GetScreenX(),_system->GetGraphics()->GetScreenY());

		int starty;
		if (_state.get()->_lines.size()*25 < _system->GetGraphics()->GetScreenY())
		{
		   starty = (_state.get()->_lines.size()*25) - 25;
		}
		else {
			starty = _system->GetGraphics()->GetScreenY() - 25;
		}

		for (std::vector<Line>::iterator iter = _state.get()->_lines.begin();iter!=_state.get()->_lines.end();++iter)
		{
			  SetRect(&font_rect,0,starty,_system->GetGraphics()->GetScreenX(),starty+25);

			_font->DrawText(NULL,        //pSprite
										iter->Content.c_str(),  //pString
										-1,          //Count
										&font_rect,  //pRect
										DT_LEFT|DT_NOCLIP,//Format,
										iter->Color); //Color

			if (iter->StatusText!="")
			{
				SetRect(&font_rect,_system->GetGraphics()->GetScreenX()-200,starty,_system->GetGraphics()->GetScreenX(),starty+25);

				_font->DrawText(NULL,        //pSprite
										iter->StatusText.c_str(),  //pString
										-1,          //Count
										&font_rect,  //pRect
										DT_LEFT|DT_NOCLIP,//Format,
										iter->StatusColor); //Color
			}

			starty-=25;
			if (starty<=-25) break;
		}
	}
}

void TextManager::OnResetDevice()
{
	_font->OnResetDevice();
}

void TextManager::OnLostDevice()
{
	_font->OnLostDevice();
}

}}