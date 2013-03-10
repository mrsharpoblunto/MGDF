#include "stdafx.h"

#include "../../src/core/common/MGDFParameterManager.hpp"

using namespace MGDF::core;

SUITE(ParameterManagerTests)
{	
	TEST(TestParseParameters)
	{
		ParameterManager::Instance().AddParameterString("-parameter:value -novalueparam -parameter1:value1");
		CHECK(ParameterManager::Instance().HasParameter("parameter")); 
		CHECK(ParameterManager::Instance().HasParameter("novalueparam"));
		CHECK(ParameterManager::Instance().HasParameter("parameter1"));
		CHECK_EQUAL("value",ParameterManager::Instance().GetParameter("parameter"));
		CHECK_EQUAL("value1",ParameterManager::Instance().GetParameter("parameter1"));
	}
}

