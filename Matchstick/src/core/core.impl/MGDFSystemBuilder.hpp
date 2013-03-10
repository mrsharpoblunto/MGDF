#pragma once

#include "MGDFSystemImpl.hpp"

namespace MGDF
{
namespace core
{

class SystemBuilder
{
public:
	static System *CreateSystem();
	static void DisposeSystem( System *system );
private:
	static bool RegisterBaseComponents();
	static bool RegisterAdditionalComponents( std::string gameUid );
	static void UnregisterComponents();
	static void InitParameterManager();
	static void InitResources( std::string gameUid = "" );
	static void InitLogger();
	static std::string GetApplicationDirectory( HINSTANCE instance );
};

}
}