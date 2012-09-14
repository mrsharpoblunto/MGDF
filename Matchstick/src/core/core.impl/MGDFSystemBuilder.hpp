#pragma once

#include <windows.h>
#include "MGDFSystemImpl.hpp"

namespace MGDF { namespace core {

class SystemBuilder {
public:
	static System *CreateSystem(HINSTANCE instance,HWND window); 
	static void DisposeSystem(System *system);
private:
	static bool RegisterBaseComponents(HINSTANCE instance,HWND window);
	static bool RegisterAdditionalComponents(HINSTANCE instance,HWND window,std::string gameUid);
	static void UnregisterComponents();
	static void InitParameterManager();
	static void InitResources(std::string gameUid="");
	static void InitLogger();
	static std::string GetApplicationDirectory(HINSTANCE instance); 
};

}}