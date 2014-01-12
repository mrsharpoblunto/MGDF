#pragma once

namespace MGDF
{

/**
 This class is responsible for recording statistics during a game session.
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
	 */
	virtual void  SaveStatistic( const char *name, const char *value ) = 0;
};

}