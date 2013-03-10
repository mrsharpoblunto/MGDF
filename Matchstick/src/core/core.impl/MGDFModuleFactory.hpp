#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

typedef bool (*GetCustomArchiveHandlersPtr)(IArchiveHandler **list,UINT32 *length,ILogger *logger,IErrorHandler *errorHandler);
typedef IModule * (*GetModulePtr)(ISystem *);
typedef bool (*IsCompatibleInterfaceVersionPtr)(int);
typedef UINT32 (*GetCompatibleFeatureLevelsPtr)(D3D_FEATURE_LEVEL *,UINT32 *);

class ModuleFactory {
public:
	virtual ~ModuleFactory();
	ModuleFactory(IGame *);

	bool GetCustomArchiveHandlers(IArchiveHandler **list,UINT32 *length,ILogger *logger,IErrorHandler *errorHandler);
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