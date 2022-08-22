#pragma once

#include <string>

namespace MGDF {
namespace core {

class ParameterConstants {
 public:
  static const char *LOG_LEVEL;
  static const char *USER_DIR_OVERRIDE;
  static const char *GAME_DIR_OVERRIDE;
  static const char *METRICS_PORT;
  static const char *LOG_ENDPOINT;
  static const char *STATISTICS_ENDPOINT;

  static const char *VALUE_LOG_LEVEL_LOW;
  static const char *VALUE_LOG_LEVEL_MEDIUM;
  static const char *VALUE_LOG_LEVEL_HIGH;
  static const char *VALUE_LOG_LEVEL_ERROR;
};

}  // namespace core
}  // namespace MGDF