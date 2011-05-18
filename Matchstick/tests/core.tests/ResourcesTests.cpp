#include <winunit.h>

#include <boost/lexical_cast.hpp>
#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/common/MGDFUniqueIDAllocator.hpp"

using namespace MGDF::core;

FIXTURE(ResourcesTestFixture)

SETUP(ResourcesTestFixture)
{
	HINSTANCE inst;
	inst=(HINSTANCE)GetModuleHandle("core.tests.dll");
	Resources::Instance(inst,true);
}

TEARDOWN(ResourcesTestFixture)
{
}

BEGIN_TESTF(ResourcesTests,ResourcesTestFixture)
{
	WIN_ASSERT_NOT_EQUAL(Resources::Instance().RootDir(),".");

	std::string root = Resources::Instance().RootDir();
	WIN_ASSERT_EQUAL(root + "schemas/schema1.xsd",Resources::Instance().SchemaFile("schema1.xsd"));

	WIN_ASSERT_EQUAL(root +"games/junkship/bin/module.dll",Resources::Instance().Module("junkship"));

	WIN_ASSERT_EQUAL(root + "games/junkship/game.xml",Resources::Instance().GameFile("junkship"));
	WIN_ASSERT_EQUAL(root + "games/junkship/preferences.xml",Resources::Instance().GameDefaultPreferencesFile("junkship"));
	WIN_ASSERT_EQUAL(root + "games/junkship/bin/",Resources::Instance().BinDir("junkship"));
	WIN_ASSERT_EQUAL(root + "games/junkship/content/",Resources::Instance().ContentDir("junkship"));
	WIN_ASSERT_EQUAL(root + "games/core/preferences.xml", Resources::Instance().CorePreferencesFile());

	//use the games directory override
	Resources::Instance().SetGamesBaseDir("c:/MGDF/");
	WIN_ASSERT_EQUAL("c:/MGDF/junkship/game.xml",Resources::Instance().GameFile("junkship"));
	WIN_ASSERT_EQUAL("c:/MGDF/junkship/preferences.xml",Resources::Instance().GameDefaultPreferencesFile("junkship"));
	WIN_ASSERT_EQUAL("c:/MGDF/junkship/bin/",Resources::Instance().BinDir("junkship"));
	WIN_ASSERT_EQUAL("c:/MGDF/junkship/content/",Resources::Instance().ContentDir("junkship"));
	WIN_ASSERT_EQUAL(root + "games/core/preferences.xml", Resources::Instance().CorePreferencesFile());

	WIN_ASSERT_EQUAL(root + "user/",Resources::Instance().UserBaseDir());
	WIN_ASSERT_EQUAL(root + "user/working/",Resources::Instance().WorkingDir());
	WIN_ASSERT_EQUAL(root + "user/games/junkship/",Resources::Instance().UserDir("junkship"));
	WIN_ASSERT_EQUAL(root + "user/games/junkship/preferences.xml",Resources::Instance().GameUserPreferencesFile("junkship"));
	WIN_ASSERT_EQUAL(root + "user/games/junkship/save1/gameState.xml",Resources::Instance().GameStateSaveFile("junkship","save1"));
	WIN_ASSERT_EQUAL(root + "user/games/junkship/save1/"+boost::lexical_cast<std::string>(UniqueIDAllocator::GetID()+1)+".sav",Resources::Instance().SaveFile("junkship","save1"));
	WIN_ASSERT_EQUAL(root + "user/games/junkship/save1/",Resources::Instance().SaveDir("junkship","save1"));
	WIN_ASSERT_EQUAL(root + "user/games/junkship/save1/data/",Resources::Instance().SaveDataDir("junkship","save1"));

}
END_TESTF