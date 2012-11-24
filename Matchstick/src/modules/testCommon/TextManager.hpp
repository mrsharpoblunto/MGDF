#pragma once

#include "d3d11.h"
#include "FW1FontWrapper/FW1FontWrapper.h"
#include "Common.hpp"
#include <boost/shared_ptr.hpp>
#include <MGDF/MGDF.hpp>

namespace MGDF {
	namespace Test {

#define GREEN 0xff44ff44
#define WHITE 0xffffffff
#define RED 0xff4444ff

		typedef struct {
			UINT32 Color;
			std::string Content;

			UINT32 StatusColor;
			std::string StatusText;
		} Line;

		class TextManager;

		class TESTCOMMON_DLL TextManagerState
		{
			friend class TextManager;
		public:
			virtual ~TextManagerState(){};
			TextManagerState(){};
			TextManagerState(const TextManagerState *state);
			void AddLine(UINT32 color,const std::string &line);
			void SetStatus(UINT32 color,const std::string &text);
			boost::shared_ptr<TextManagerState> Interpolate(const TextManagerState *state,double alpha);
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
			TextManager(ISystem *system);
			void SetState(boost::shared_ptr<TextManagerState> state);
			void DrawText();
		private:
#pragma warning(push)
#pragma warning(disable: 4251)
			boost::shared_ptr<TextManagerState> _state;
#pragma warning(pop)
			ISystem *_system;
			IFW1FontWrapper *_font;
			ID3D11DeviceContext *_immediateContext;
		};
	}
}