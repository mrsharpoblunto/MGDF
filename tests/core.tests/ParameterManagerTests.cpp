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

TEST(ParameterManagerTests, TestParseValuesContainingHyphens) {
  ParameterManager::Instance().AddParameterString(
      "-gamediroverride \"D:\\dev\\cpp\\junkship-wt-test\\game\" "
      "-dashurl http://my-host:3100/loki/api/v1/push "
      "-quotedwithdash \"a-b c-d\" -usedefaultadapter");
  // quoted path with a hyphen must survive intact (the value used to be
  // truncated at the first '-')
  EXPECT_STREQ("D:\\dev\\cpp\\junkship-wt-test\\game",
               ParameterManager::Instance().GetParameter("gamediroverride"));
  // unquoted value with hyphens runs to the next whitespace
  EXPECT_STREQ("http://my-host:3100/loki/api/v1/push",
               ParameterManager::Instance().GetParameter("dashurl"));
  // quoted value keeps both spaces and hyphens
  EXPECT_STREQ("a-b c-d",
               ParameterManager::Instance().GetParameter("quotedwithdash"));
  // a bare flag immediately following a value is still recognised
  EXPECT_TRUE(ParameterManager::Instance().HasParameter("usedefaultadapter"));
}

TEST(ParameterManagerTests, TestEnvironmentParameters) {
  SetEnvironmentVariableA("MGDF_ENVTESTPARAM", "envvalue");
  // a non MGDF_ prefixed variable must be ignored
  SetEnvironmentVariableA("ENVTESTIGNORED", "ignored");
  ParameterManager::Instance().AddEnvironmentParameters("MGDF_");

  // prefix is stripped and the name is lower-cased
  EXPECT_TRUE(ParameterManager::Instance().HasParameter("envtestparam"));
  EXPECT_STREQ("envvalue",
               ParameterManager::Instance().GetParameter("envtestparam"));
  EXPECT_FALSE(ParameterManager::Instance().HasParameter("envtestignored"));

  SetEnvironmentVariableA("MGDF_ENVTESTPARAM", nullptr);
  SetEnvironmentVariableA("ENVTESTIGNORED", nullptr);
}

TEST(ParameterManagerTests, TestCommandLineOverridesEnvironment) {
  SetEnvironmentVariableA("MGDF_ENVOVERRIDE", "fromenv");
  ParameterManager::Instance().AddEnvironmentParameters("MGDF_");
  // command line is parsed after the environment, so it takes precedence
  ParameterManager::Instance().AddParameterString("-envoverride fromcmdline");

  EXPECT_STREQ("fromcmdline",
               ParameterManager::Instance().GetParameter("envoverride"));

  SetEnvironmentVariableA("MGDF_ENVOVERRIDE", nullptr);
}
