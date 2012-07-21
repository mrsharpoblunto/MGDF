#include <winunit.h>

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/common/MGDFVersionHelper.hpp"
#include "../../src/core/vfs/MGDFVirtualFileSystemComponentImpl.hpp"
#include "../../src/core/storage/MGDFStorageFactoryComponentImpl.hpp"
#include "MGDFMockLogger.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;
using namespace MGDF::core::storage;

FIXTURE( StorageTestFixture )

namespace StorageTests {
	IVirtualFileSystemComponent *_vfs;
	IStorageFactoryComponent *_storage;
	MGDF::core::tests::MockLogger *_logger;
}
using namespace StorageTests;

SETUP(StorageTestFixture)
{
	HINSTANCE inst;
	inst=(HINSTANCE)GetModuleHandleW(L"core.tests.dll");
	Resources::Instance(inst);
	Resources::Instance().SetUserBaseDir(true,"junkship");

	_logger = new MGDF::core::tests::MockLogger();
	_vfs = CreateVirtualFileSystemComponentImpl(nullptr,nullptr,(ILogger *)_logger);
	_vfs->MapDirectory((Resources::Instance().RootDir()+L"../../tests/content").c_str(),L"",nullptr,false);

	_storage = CreateStorageFactoryComponentImpl(nullptr,nullptr);
}

TEARDOWN(StorageTestFixture)
{
	delete _storage;
	delete _vfs;
	delete _logger;
}

/**
ensure that game files can be read by the engine
*/
BEGIN_TESTF(StorageGameHandlerTest,StorageTestFixture)
{

	std::auto_ptr<IGameStorageHandler> handler(_storage->CreateGameStorageHandler());

	std::wstring path = _vfs->GetFile(L"console.json")->GetPhysicalPath();
	handler->Load(path);

	WIN_ASSERT_EQUAL("Console",handler->GetGameUid());
	WIN_ASSERT_EQUAL("Lua Console",handler->GetGameName());
	Version expected;
	expected.Major = 0;
	expected.Minor=1;
	expected.Build = -1;
	expected.Revision = -1;
	WIN_ASSERT_EQUAL(0,VersionHelper::Compare(handler->GetVersion(),&expected));
}
END_TESTF

/**
ensure that game state files can be read by the engine
*/
BEGIN_TESTF(StorageGameStateHandlerTest,StorageTestFixture)
{
	Version expected = VersionHelper::Create("0.1");
	WIN_ASSERT_EQUAL("0.1",VersionHelper::Format(&expected));
	WIN_ASSERT_EQUAL(0,VersionHelper::Compare(&expected,&VersionHelper::Create("0.1")));	
	WIN_ASSERT_EQUAL(-1,VersionHelper::Compare(&expected,&VersionHelper::Create("0.1.1")));	
	WIN_ASSERT_EQUAL(1,VersionHelper::Compare(&expected,&VersionHelper::Create("0.0.1")));	

	IGameStateStorageHandler *handler = _storage->CreateGameStateStorageHandler("Console",&expected);

	std::wstring path = _vfs->GetFile(L"gamestate.json")->GetPhysicalPath();
	handler->Load(path);

	WIN_ASSERT_EQUAL("Console",handler->GetGameUid());
	WIN_ASSERT_EQUAL(0,VersionHelper::Compare(handler->GetVersion(),&expected));

	std::wstring savePath = Resources::Instance().RootDir()+L"../../tests/content/temp.json";
	handler->Save(savePath);
	delete handler;

	//reload using the freshly saved file, the contents should not have changed
	handler = _storage->CreateGameStateStorageHandler("Console",&expected);
	handler->Load(savePath);

	WIN_ASSERT_EQUAL("Console",handler->GetGameUid());
	WIN_ASSERT_EQUAL(0,VersionHelper::Compare(handler->GetVersion(),&expected));

	boost::filesystem::remove(boost::filesystem::wpath(savePath,boost::filesystem::native));//remove the temp file
	delete handler;
}
END_TESTF

/**
ensure that preferences can be loaded and saved by the engine
*/
BEGIN_TESTF(StoragePreferencesHandlerTest,StorageTestFixture)
{
	IPreferenceConfigStorageHandler *handler = _storage->CreatePreferenceConfigStorageHandler();

	std::wstring path = _vfs->GetFile(L"preferences.json")->GetPhysicalPath();
	handler->Load(path);

	IPreferenceConfigStorageHandler::iterator iter;
	int count=0;
	bool foundResolution=false;
	bool foundScreenX=false;
	for (iter=handler->Begin();iter!=handler->End();++iter) {
		if (iter->first=="resolution") {
			WIN_ASSERT_EQUAL("800*600",iter->second);
			foundResolution = true;
		}
		else if (iter->first=="screenX") {
			WIN_ASSERT_EQUAL("800",iter->second);
			foundScreenX = true;
		}
		++count;
	}
	WIN_ASSERT_TRUE(foundResolution);
	WIN_ASSERT_TRUE(foundScreenX);
	WIN_ASSERT_EQUAL(9,count);

	std::wstring savePath = Resources::Instance().RootDir()+L"../../tests/content/temp.json";
	if (boost::filesystem::exists(boost::filesystem::wpath(savePath,boost::filesystem::native))){
		boost::filesystem::remove(boost::filesystem::wpath(savePath,boost::filesystem::native));//remove the temp file	
	}
	handler->Save(savePath);
	delete handler;

	//reload the file, it should be identical
	handler = _storage->CreatePreferenceConfigStorageHandler();
	handler->Load(path);

	count=0;
	foundResolution=false;
	foundScreenX=false;
	for (iter=handler->Begin();iter!=handler->End();++iter) {
		if (iter->first=="resolution") {
			WIN_ASSERT_EQUAL("800*600",iter->second);
			foundResolution = true;
		}
		else if (iter->first=="screenX") {
			WIN_ASSERT_EQUAL("800",iter->second);
			foundScreenX = true;
		}
		++count;
	}
	WIN_ASSERT_TRUE(foundResolution);
	WIN_ASSERT_TRUE(foundScreenX);
	WIN_ASSERT_EQUAL(9,count);

	boost::filesystem::remove(boost::filesystem::wpath(savePath,boost::filesystem::native));//remove the temp file
	delete handler;
}
END_TESTF