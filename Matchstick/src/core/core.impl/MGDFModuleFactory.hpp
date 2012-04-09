#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

typedef ICustomArchiveHandlers * (*GetCustomArchiveHandlersPtr)(void);
typedef IModule * (*GetModulePtr)(ISystem *);
typedef bool (*IsCompatibleInterfaceVersionPtr)(int);

class ModuleFactory {
public:
	virtual ~ModuleFactory();
	ModuleFactory(IGame *);

	ICustomArchiveHandlers *GetCustomArchiveHandlers();
	IModule *GetModule(ISystem *system);
	bool IsCompatibleInterfaceVersion(int);
private:
	IGame *_game;
	HINSTANCE _moduleInstance;
	GetCustomArchiveHandlersPtr _getCustomArchiveHandlers;
	GetModulePtr _getModule;
	IsCompatibleInterfaceVersionPtr _isCompatibleInterfaceVersion;
};

}}