#include "StdAfx.h"

#include "MGDFGlobalCallbacks.hpp"

#include <boost/filesystem/operations.hpp>
#include "../common/MGDFResources.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFCurrentDirectoryHelper.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

GlobalCallbacks::~GlobalCallbacks()
{
	if (_globalCallbacksInstance!=NULL) {
		FreeLibrary(_globalCallbacksInstance);
	}
}

GlobalCallbacks::GlobalCallbacks(IGame *game)
{
	_game = game;

	_globalCallbacksInstance = NULL;
	_getCustomArchiveHandlers = NULL;
	_getGameStateMigrator = NULL;

	boost::filesystem::path globalModule(Resources::Instance().GlobalModule(_game->GetUid()),boost::filesystem::native);
	if (boost::filesystem::exists(globalModule)) 
	{
		GetLoggerImpl()->Add(THIS_NAME,"Loading Global Module found");
		CurrentDirectoryHelper::Instance().Push(Resources::Instance().ModulesDir(game->GetUid()));
		_globalCallbacksInstance = LoadLibrary(Resources::Instance().GlobalModule(_game->GetUid()).c_str());
		CurrentDirectoryHelper::Instance().Pop();

		if (_globalCallbacksInstance!=NULL) {
			_getGameStateMigrator = (GetGameStateMigratorPtr)GetProcAddress(_globalCallbacksInstance, "GetGameStateMigrator");
			if (_getGameStateMigrator!=NULL) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded GameStateMigrator from Global Module");
			}

			_getCustomArchiveHandlers = (GetCustomArchiveHandlersPtr)GetProcAddress(_globalCallbacksInstance, "GetCustomArchiveHandlers");
			if (_getGameStateMigrator!=NULL) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded CustomArchiveHandlers from Global Module");
			}
		}
		else {
			GetLoggerImpl()->Add(THIS_NAME,"Failed to load Global Module");
		}
	}
}

ICustomArchiveHandlers *GlobalCallbacks::GetCustomArchiveHandlers()
{
	if (_getCustomArchiveHandlers!=NULL) {
		return _getCustomArchiveHandlers();
	}
	else {
		return NULL;
	}
}

IGameStateMigrator *GlobalCallbacks::GetGameStateMigrator()
{
	if (_getGameStateMigrator!=NULL) {
		return _getGameStateMigrator();
	}
	else {
		return NULL;
	}
}

}}