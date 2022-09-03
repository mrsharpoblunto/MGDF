#include "StdAfx.h"

#include "MGDFParameterConstants.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

const char *ParameterConstants::LOG_LEVEL = "loglevel";

const char *ParameterConstants::USER_DIR_OVERRIDE = "userdiroverride";
const char *ParameterConstants::GAME_DIR_OVERRIDE = "gamediroverride";

const char *ParameterConstants::METRICS_PORT = "metricsport";
const char *ParameterConstants::LOG_ENDPOINT = "logendpoint";
const char *ParameterConstants::STATISTICS_ENDPOINT_OVERRIDE =
    "statisticsendpointoverride";
const char *ParameterConstants::STATISTICS_ENABLED = "statisticsenabled";

const char *ParameterConstants::VALUE_LOG_LEVEL_LOW = "log_low";
const char *ParameterConstants::VALUE_LOG_LEVEL_MEDIUM = "log_medium";
const char *ParameterConstants::VALUE_LOG_LEVEL_HIGH = "log_high";
const char *ParameterConstants::VALUE_LOG_LEVEL_ERROR = "log_error";

}  // namespace core
}  // namespace MGDF