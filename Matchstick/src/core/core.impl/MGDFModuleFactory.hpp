#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

typedef ICustomArchiveHandlers * (*GetCustomArchiveHandlersPtr)(void);
typedef IModule * (*GetModulePtr)(ISystem *);
typedef bool (*IsCompatibleInterfaceVersionPtr)(int);
typedef UINT32 (*GetCompatibleFeatureLevelsPtr)(D3D_FEATURE_LEVEL *,UINT32 *);

class ModuleFactory {
public:
	virtual ~ModuleFactory();
	ModuleFactory(IGame *);

	ICustomArchiveHandlers *GetCustomArchiveHandlers();
	IModule *GetModule(ISystem *system);
	bool IsCompatibleInterfaceVersion(int);
	UINT32 GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,UINT32 *levelSize);

private:
	IGame *_game;
	HINSTANCE _moduleInstance;
	GetCustomArchiveHandlersPtr _getCustomArchiveHandlers;
	GetModulePtr _getModule;
	IsCompatibleInterfaceVersionPtr _isCompatibleInterfaceVersion;
	GetCompatibleFeatureLevelsPtr _getCompatibleFeatureLevels;
};

}}