#include "stdafx.h"

#include "UnitTest++.h"

#include "../../src/core/common/MGDFResources.hpp"

using namespace MGDF::core;

SUITE(ResourcesTests)
{

struct ResourcesTestFixture {
	ResourcesTestFixture() {
		HINSTANCE inst;
		inst=(HINSTANCE)GetModuleHandleW(L"core.tests.exe");
		Resources::Instance(inst);
		Resources::Instance().SetUserBaseDir(true,"junkship");
	}
	virtual ~ResourcesTestFixture()
	{
	}
};

TEST_FIXTURE(ResourcesTestFixture,PathTests)
{
	std::wstring root = Resources::Instance().RootDir();

	CHECK_WS_EQUAL(root +L"game/bin/module.dll",Resources::Instance().Module());

	CHECK_WS_EQUAL(root + L"game/game.json",Resources::Instance().GameFile());
	CHECK_WS_EQUAL(root + L"game/preferences.json",Resources::Instance().GameDefaultPreferencesFile());
	CHECK_WS_EQUAL(root + L"game/bin/",Resources::Instance().BinDir());
	CHECK_WS_EQUAL(root + L"game/content/",Resources::Instance().ContentDir());
	CHECK_WS_EQUAL(root + L"resources/preferences.json", Resources::Instance().CorePreferencesFile());

	//use the games directory override
	Resources::Instance().SetGameBaseDir(L"c:/MGDF/");
	CHECK_WS_EQUAL(L"c:/MGDF/game.json",Resources::Instance().GameFile());
	CHECK_WS_EQUAL(L"c:/MGDF/preferences.json",Resources::Instance().GameDefaultPreferencesFile());
	CHECK_WS_EQUAL(L"c:/MGDF/bin/",Resources::Instance().BinDir());
	CHECK_WS_EQUAL(L"c:/MGDF/content/",Resources::Instance().ContentDir());
	CHECK_WS_EQUAL(root + L"resources/preferences.json", Resources::Instance().CorePreferencesFile());

	CHECK_WS_EQUAL(root + L"user/junkship/",Resources::Instance().UserBaseDir());
	CHECK_WS_EQUAL(root + L"user/junkship/working/",Resources::Instance().WorkingDir());
	CHECK_WS_EQUAL(root + L"user/junkship/preferences.json",Resources::Instance().GameUserPreferencesFile());
	CHECK_WS_EQUAL(root + L"user/junkship/saves/",Resources::Instance().SaveBaseDir());
	CHECK_WS_EQUAL(root + L"user/junkship/saves/save1/gameState.json",Resources::Instance().GameStateSaveFile("save1"));
	CHECK_WS_EQUAL(root + L"user/junkship/saves/save1/",Resources::Instance().SaveDir("save1"));
	CHECK_WS_EQUAL(root + L"user/junkship/saves/save1/data/",Resources::Instance().SaveDataDir("save1"));
}

}