#include <winunit.h>

#include <boost/lexical_cast.hpp>
#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/common/MGDFUniqueIDAllocator.hpp"

using namespace MGDF::core;

FIXTURE(ResourcesTestFixture)

SETUP(ResourcesTestFixture)
{
	HINSTANCE inst;
	inst=(HINSTANCE)GetModuleHandleW(L"core.tests.dll");
	Resources::Instance(inst);
	Resources::Instance().SetUserBaseDir(true,"junkship");
}

TEARDOWN(ResourcesTestFixture)
{
}

BEGIN_TESTF(ResourcesTests,ResourcesTestFixture)
{
	WIN_ASSERT_NOT_EQUAL(Resources::Instance().RootDir(),L".");

	std::wstring root = Resources::Instance().RootDir();
	WIN_ASSERT_EQUAL(root + L"schemas/schema1.xsd",Resources::Instance().SchemaFile(L"schema1.xsd"));

	WIN_ASSERT_EQUAL(root +L"game/bin/module.dll",Resources::Instance().Module());

	WIN_ASSERT_EQUAL(root + L"game/game.xml",Resources::Instance().GameFile());
	WIN_ASSERT_EQUAL(root + L"game/preferences.xml",Resources::Instance().GameDefaultPreferencesFile());
	WIN_ASSERT_EQUAL(root + L"game/bin/",Resources::Instance().BinDir());
	WIN_ASSERT_EQUAL(root + L"game/content/",Resources::Instance().ContentDir());
	WIN_ASSERT_EQUAL(root + L"resources/preferences.xml", Resources::Instance().CorePreferencesFile());

	//use the games directory override
	Resources::Instance().SetGameBaseDir(L"c:/MGDF/");
	WIN_ASSERT_EQUAL(L"c:/MGDF/game.xml",Resources::Instance().GameFile());
	WIN_ASSERT_EQUAL(L"c:/MGDF/preferences.xml",Resources::Instance().GameDefaultPreferencesFile());
	WIN_ASSERT_EQUAL(L"c:/MGDF/bin/",Resources::Instance().BinDir());
	WIN_ASSERT_EQUAL(L"c:/MGDF/content/",Resources::Instance().ContentDir());
	WIN_ASSERT_EQUAL(root + L"resources/preferences.xml", Resources::Instance().CorePreferencesFile());

	WIN_ASSERT_EQUAL(root + L"user/junkship/",Resources::Instance().UserBaseDir());
	WIN_ASSERT_EQUAL(root + L"user/junkship/working/",Resources::Instance().WorkingDir());
	WIN_ASSERT_EQUAL(root + L"user/junkship/preferences.xml",Resources::Instance().GameUserPreferencesFile());
	WIN_ASSERT_EQUAL(root + L"user/junkship/saves/",Resources::Instance().SaveBaseDir());
	WIN_ASSERT_EQUAL(root + L"user/junkship/saves/save1/gameState.xml",Resources::Instance().GameStateSaveFile("save1"));
	WIN_ASSERT_EQUAL(root + L"user/junkship/saves/save1/",Resources::Instance().SaveDir("save1"));
	WIN_ASSERT_EQUAL(root + L"user/junkship/saves/save1/data/",Resources::Instance().SaveDataDir("save1"));

}
END_TESTF