#pragma once

#include <string>

namespace MGDF { namespace core {

class ParameterConstants
{
public:
	static const char *DO_VALIDATION;
	static const char *LOG_LEVEL;
	static const char *BOOT_GAME;
	static const char *USER_DIR_OVERRIDE;
	static const char *GAMES_DIR_OVERRIDE;

	static const char *VALUE_DO_VALIDATION_TRUE;
	static const char *VALUE_LOG_LEVEL_LOW;
	static const char *VALUE_LOG_LEVEL_MEDIUM;
	static const char *VALUE_LOG_LEVEL_HIGH;
	static const char *VALUE_LOG_LEVEL_ERROR;
};

}}