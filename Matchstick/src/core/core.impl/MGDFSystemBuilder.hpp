#pragma once

#include <windows.h>
#include "MGDFSystemImpl.hpp"

namespace MGDF { namespace core {

class SystemBuilder {
public:
	static System *CreateSystem(HINSTANCE instance,HWND window); 
	static void DisposeSystem(System *system);
private:
	static bool RegisterComponents(HINSTANCE instance,HWND window);
	static void UnregisterComponents();
	static void InitParameterManager();
	static void InitResources();
	static void InitLogger();
	static std::string GetApplicationDirectory(HINSTANCE instance); 
};

}}