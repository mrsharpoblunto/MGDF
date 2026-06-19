#pragma once

#include <string>

namespace MGDF {
namespace core {

class ParameterConstants {
 public:
  // prefix identifying which environment variables are MGDF parameters
  // e.g. MGDF_LOGLEVEL maps to the "loglevel" parameter
  static const char *ENV_PREFIX;

  static const char *LOG_LEVEL;
  static const char *USER_DIR_OVERRIDE;
  static const char *GAME_DIR_OVERRIDE;
  static const char *METRICS_PORT;
  static const char *LOG_ENDPOINT;
  static const char *STATISTICS_ENDPOINT_OVERRIDE;
  static const char *STATISTICS_ENABLED;
  static const char *USE_DEFAULT_ADAPTER;

  static const char *VALUE_LOG_LEVEL_LOW;
  static const char *VALUE_LOG_LEVEL_MEDIUM;
  static const char *VALUE_LOG_LEVEL_HIGH;
  static const char *VALUE_LOG_LEVEL_ERROR;
};

}  // namespace core
}  // namespace MGDF