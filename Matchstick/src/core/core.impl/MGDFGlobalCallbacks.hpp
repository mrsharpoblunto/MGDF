#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

typedef IGameStateMigrator * (*GetGameStateMigratorPtr)(void);
typedef ICustomArchiveHandlers * (*GetCustomArchiveHandlersPtr)(void);

class GlobalCallbacks {
public:
	virtual ~GlobalCallbacks();
	GlobalCallbacks(IGame *);

	ICustomArchiveHandlers *GetCustomArchiveHandlers();
	IGameStateMigrator *GetGameStateMigrator();
private:
	IGame *_game;
	HINSTANCE _globalCallbacksInstance;
	GetCustomArchiveHandlersPtr _getCustomArchiveHandlers;
	GetGameStateMigratorPtr _getGameStateMigrator;
};

}}