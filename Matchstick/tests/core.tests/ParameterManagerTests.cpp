#include "stdafx.h"

#include "../../src/core/common/MGDFParameterManager.hpp"

using namespace MGDF::core;

TEST(ParameterManagerTests, TestParseParameters) {
  ParameterManager::Instance().AddParameterString(
      "-parameter value -novalueparam -parameter1 value1 -parameter2 "
      "\"quoted param\" -parameter3 'quoted param'");
  EXPECT_TRUE(ParameterManager::Instance().HasParameter("parameter"));
  EXPECT_TRUE(ParameterManager::Instance().HasParameter("novalueparam"));
  EXPECT_TRUE(ParameterManager::Instance().HasParameter("parameter1"));
  EXPECT_TRUE(ParameterManager::Instance().HasParameter("parameter2"));
  EXPECT_TRUE(ParameterManager::Instance().HasParameter("parameter3"));
  EXPECT_STREQ("value", ParameterManager::Instance().GetParameter("parameter"));
  EXPECT_STREQ("value1",
               ParameterManager::Instance().GetParameter("parameter1"));
  EXPECT_STREQ("quoted param",
               ParameterManager::Instance().GetParameter("parameter2"));
  EXPECT_STREQ("quoted param",
               ParameterManager::Instance().GetParameter("parameter3"));
}
