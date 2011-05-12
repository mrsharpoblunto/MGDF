#pragma once

#include "d3d9.h"
#include "Common.hpp"
#include <boost/shared_ptr.hpp>
#include <MGDF/MGDF.hpp>

namespace MGDF {
	namespace Test {

#define GREEN 0xff44ff44
#define WHITE 0xffffffff
#define RED 0xffff4444

		typedef struct {
			unsigned int Color;
			std::string Content;

			unsigned int StatusColor;
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
			void AddLine(unsigned int color,std::string line);
			void SetStatus(unsigned int color,std::string text);
			boost::shared_ptr<TextManagerState> Interpolate(const TextManagerState *state,double alpha);
		private:
			std::vector<Line> _lines;
		};

		class TESTCOMMON_DLL TextManager
		{
		public:
			virtual ~TextManager();
			TextManager(ISystem *system);
			void SetState(boost::shared_ptr<TextManagerState> state);
			void DrawText();
			void OnResetDevice();
			void OnLostDevice();
		private:
			boost::shared_ptr<TextManagerState> _state;
			ISystem *_system;
			ID3DXFont *_font;
		};
	}
}