#pragma once

#include <MGDF/MGDFError.hpp>

namespace MGDF
{

/**
 Provides an interface for recording statistics during a game session.
 If the game has a statistics service configured, the stats log will be uploaded by the
 GamesManager at the end of a play session.
 */
class IStatisticsManager
{
public:
	/**
	 Logs a statistic as a key/value pair
	 \param name the name of the statistic tracker key
	 \param value the value of the statistic
	 \return MGDF_OK if the statistic could be saved, MGDF_ERR_INVALID_STATS_KEY if the statistic key was invalid 
	 due to containing a space character or being more than 255 characters in length, MGDF_ERR_INVALID_STATS_VALUE
	 if the statistic value is more than 255 characters in length
	 */
	virtual MGDFError  SaveStatistic( const char *name, const char *value ) = 0;
};

}