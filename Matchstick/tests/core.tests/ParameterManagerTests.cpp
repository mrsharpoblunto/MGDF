#include <winunit.h>
#include "../../src/core/common/MGDFParameterManagerImpl.hpp"

using namespace MGDF::core;

BEGIN_TEST( ParameterManagerTests )
{
		GetParameterManagerImpl()->AddParameterString("-parameter:value -novalueparam -parameter1:value1");
		WIN_ASSERT_EQUAL(GetParameterManagerImpl()->HasParameter("parameter"),true); 
		WIN_ASSERT_EQUAL(GetParameterManagerImpl()->HasParameter("novalueparam"),true);
		WIN_ASSERT_EQUAL(GetParameterManagerImpl()->HasParameter("parameter1"),true);
		WIN_ASSERT_STRING_EQUAL("value",GetParameterManagerImpl()->GetParameter("parameter"));
		WIN_ASSERT_STRING_EQUAL("value1",GetParameterManagerImpl()->GetParameter("parameter1"));
}
END_TEST

