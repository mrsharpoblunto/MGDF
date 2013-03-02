#include "stdafx.h"

#include "unittest++.h"
#include "../../src/core/common/MGDFParameterManagerImpl.hpp"

using namespace MGDF::core;

SUITE(ParameterManagerTests)
{	
	TEST(TestParseParameters)
	{
		GetParameterManagerImpl()->AddParameterString("-parameter:value -novalueparam -parameter1:value1");
		CHECK(GetParameterManagerImpl()->HasParameter("parameter")); 
		CHECK(GetParameterManagerImpl()->HasParameter("novalueparam"));
		CHECK(GetParameterManagerImpl()->HasParameter("parameter1"));
		CHECK_EQUAL("value",GetParameterManagerImpl()->GetParameter("parameter"));
		CHECK_EQUAL("value1",GetParameterManagerImpl()->GetParameter("parameter1"));
	}
}

