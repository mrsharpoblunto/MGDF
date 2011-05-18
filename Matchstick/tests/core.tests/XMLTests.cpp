#include <winunit.h>

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/common/MGDFVersionHelper.hpp"
#include "../../src/core/vfs/MGDFVirtualFileSystemComponentImpl.hpp"
#include "../../src/core/xml/MGDFXMLFactoryComponentImpl.hpp"
#include "MGDFMockLogger.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;
using namespace MGDF::core::xml;

FIXTURE( XMLTestFixture )

namespace XMLTests {
	IVirtualFileSystemComponent *_vfs;
	IXMLFactoryComponent *_xml;
	MGDF::core::tests::MockLogger *_logger;
}
using namespace XMLTests;

SETUP(XMLTestFixture)
{
	HINSTANCE inst;
	inst=(HINSTANCE)GetModuleHandle("core.tests.dll");
	Resources::Instance(inst,true);

	_logger = new MGDF::core::tests::MockLogger();
	_vfs = CreateVirtualFileSystemComponentImpl(NULL,NULL,(ILogger *)_logger);
	_vfs->MapDirectory((Resources::Instance().RootDir()+"../../tests/content").c_str(),"",NULL,false);

	_xml = CreateXMLFactoryComponentImpl(NULL,NULL);
}

TEARDOWN(XMLTestFixture)
{
	delete _xml;
	delete _vfs;
	delete _logger;
}

/**
ensure that game files can be read by the engine
*/
BEGIN_TESTF(XMLGameHandlerTest,XMLTestFixture)
{

	std::auto_ptr<IGameXMLHandler> handler(_xml->CreateGameXMLHandler());

	std::string path = _vfs->GetFile("console.xml")->GetPhysicalPath();
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
BEGIN_TESTF(XMLGameStateHandlerTest,XMLTestFixture)
{
	Version expected = VersionHelper::Create("0.1");
	WIN_ASSERT_EQUAL("0.1",VersionHelper::Format(&expected));
	WIN_ASSERT_EQUAL(0,VersionHelper::Compare(&expected,&VersionHelper::Create("0.1")));	
	WIN_ASSERT_EQUAL(-1,VersionHelper::Compare(&expected,&VersionHelper::Create("0.1.1")));	
	WIN_ASSERT_EQUAL(1,VersionHelper::Compare(&expected,&VersionHelper::Create("0.0.1")));	

	IGameStateXMLHandler *handler = _xml->CreateGameStateXMLHandler("Console",&expected);

	std::string path = _vfs->GetFile("gamestate.xml")->GetPhysicalPath();
	handler->Load(path);

	WIN_ASSERT_EQUAL("Console",handler->GetGameUid());
	WIN_ASSERT_EQUAL(0,VersionHelper::Compare(handler->GetVersion(),&expected));

	std::string savePath = Resources::Instance().RootDir()+"../../tests/content/temp.xml";
	handler->Save(savePath);
	delete handler;

	//reload using the freshly saved file, the contents should not have changed
	handler = _xml->CreateGameStateXMLHandler("Console",&expected);
	handler->Load(savePath);

	WIN_ASSERT_EQUAL("Console",handler->GetGameUid());
	WIN_ASSERT_EQUAL(0,VersionHelper::Compare(handler->GetVersion(),&expected));

	boost::filesystem::remove(boost::filesystem::path(savePath,boost::filesystem::native));//remove the temp file
	delete handler;
}
END_TESTF

/**
ensure that preferences can be loaded and saved by the engine
*/
BEGIN_TESTF(XMLPreferencesHandlerTest,XMLTestFixture)
{
	IPreferenceConfigXMLHandler *handler = _xml->CreatePreferenceConfigXMLHandler();

	std::string path = _vfs->GetFile("preferences.xml")->GetPhysicalPath();
	handler->Load(path);

	IPreferenceConfigXMLHandler::iterator iter;
	int count=0;
	for (iter=handler->Begin();iter!=handler->End();++iter) {
		if (count==0) {
			WIN_ASSERT_EQUAL("resolution",iter->first);
			WIN_ASSERT_EQUAL("800*600",iter->second);
		}
		else if (count==6) {
			WIN_ASSERT_EQUAL("screenX",iter->first);
			WIN_ASSERT_EQUAL("800",iter->second);
		}
		++count;
	}
	WIN_ASSERT_EQUAL(9,count);

	std::string savePath = Resources::Instance().RootDir()+"../../tests/content/temp.xml";
	if (boost::filesystem::exists(boost::filesystem::path(savePath,boost::filesystem::native))){
		boost::filesystem::remove(boost::filesystem::path(savePath,boost::filesystem::native));//remove the temp file	
	}
	handler->Save(savePath);
	delete handler;

	//reload the file, it should be identical
	handler = _xml->CreatePreferenceConfigXMLHandler();
	handler->Load(path);

	count=0;
	for (iter=handler->Begin();iter!=handler->End();++iter) {
		if (count==0) {
			WIN_ASSERT_EQUAL("resolution",iter->first);
			WIN_ASSERT_EQUAL("800*600",iter->second);
		}
		else if (count==6) {
			WIN_ASSERT_EQUAL("screenX",iter->first);
			WIN_ASSERT_EQUAL("800",iter->second);
		}
		++count;
	}
	WIN_ASSERT_EQUAL(9,count);

	boost::filesystem::remove(boost::filesystem::path(savePath,boost::filesystem::native));//remove the temp file
	delete handler;
}
END_TESTF