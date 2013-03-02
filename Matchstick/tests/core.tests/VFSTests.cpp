#include "stdafx.h"

#include "UnitTest++.h"

#pragma warning( push )
#pragma warning( disable:4996 )
#include <boost/algorithm/string.hpp>
#pragma warning( pop )

#include "MGDFMockLogger.hpp"
#include "MGDFMockErrorHandler.hpp"
#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/vfs/MGDFVirtualFileSystemComponentImpl.hpp"
#include "../../src/core/vfs/archive/zip/ZipArchiveHandlerImpl.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;

SUITE( VFSTests )
{

struct VFSTestFixture
{
VFSTestFixture()
{
	HINSTANCE inst;
	inst=(HINSTANCE)GetModuleHandleW(L"core.tests.exe");
	Resources::Instance(inst);
	Resources::Instance().SetUserBaseDir(true,"junkship");

	_logger = new MGDF::core::tests::MockLogger();
	_errorHandler = new MGDF::core::tests::MockErrorHandler();

	_vfs = CreateVirtualFileSystemComponentImpl((ILogger *)_logger);
	_vfs->RegisterArchiveHandler(CreateZipArchiveHandlerImpl((ILogger *)_logger,(IErrorHandler *)_errorHandler));	
}

virtual ~VFSTestFixture()
{
	delete _vfs;
	delete _logger;
	delete _errorHandler;
}
protected:
	IVirtualFileSystemComponent *_vfs;
	MGDF::core::tests::MockLogger *_logger;
	MGDF::core::tests::MockErrorHandler *_errorHandler;
};

/**
check that zip archives are enumerated correctly by the vfs
*/
TEST_FIXTURE(VFSTestFixture,ZipArchiveTests)
{
	_vfs->MapDirectory((Resources::Instance().RootDir()+L"../../../tests/content/test.zip").c_str(),L"",nullptr,false);

	CHECK_WS_EQUAL(L"test.zip",_vfs->GetRoot()->GetFirstChild()->GetName());
	CHECK_EQUAL(6,_vfs->GetRoot()->GetFirstChild()->GetChildCount());
	CHECK_WS_EQUAL(L"game.xml",_vfs->GetFile(L"./test.zip/game.xml")->GetName());
	CHECK_WS_EQUAL(L"gameicon.png",_vfs->GetFile(L"./test.zip/gameIcon.png")->GetName());
	CHECK_WS_EQUAL(L"preferences.xml",_vfs->GetFile(L"./test.zip/preferences.xml")->GetName());
	CHECK_WS_EQUAL(L"preferencetemplates.xml",_vfs->GetFile(L"test.zip/preferenceTemplates.xml")->GetName());
	CHECK_WS_EQUAL(L"gamestate.xml",_vfs->GetFile(L"test.zip/boot/gameState.xml")->GetName());
	CHECK_WS_EQUAL(L"persistency.xml",_vfs->GetFile(L"test.zip/boot/persistency.xml")->GetName());
	CHECK_WS_EQUAL(L"test.lua",_vfs->GetFile(L"test.zip/content/test.lua")->GetName());
	CHECK(_vfs->GetFile(L"test.zip/content")->IsFolder());
}

/*
check that files inside enumeratoed archives can be read correctly
*/
TEST_FIXTURE(VFSTestFixture,ZipArchiveContentTests)
{
	_vfs->MapDirectory((Resources::Instance().RootDir()+L"../../../tests/content/test.zip").c_str(),L"",nullptr,false);

	IFile *file = _vfs->GetFile(L"test.zip/content/test.lua");
	file->OpenFile();
	UINT32 size = static_cast<UINT32>(file->GetSize());
	char* data = new char[size];
	file->Read((void *)data,size);
	std::string contents(data,size);
	delete[] data;
	file->CloseFile();

	std::vector<std::string> list;
	boost::algorithm::replace_all(contents,"\r","");
	boost::algorithm::replace_all(contents,"\t","");
	boost::split(list,contents,boost::is_any_of("\n"));

	//see if the file has as many lines as we expect
	CHECK_EQUAL(27,list.size());
	//check to see the first and last lines are as expected
	CHECK_EQUAL("class 'ConsoleStorageListener'(MGDF.StorageListener)",list[0]);
	CHECK_EQUAL("end",list[26]);
}

/**
check that vfs filters and aliases work as expected
*/
TEST_FIXTURE(VFSTestFixture,AliasAndFilterTests)
{
	_vfs->MapDirectory((Resources::Instance().RootDir()+L"../../../tests/content/test.zip").c_str(),L"",nullptr,false);

	_vfs->AddAlias(L"testData",L"./test.zip");
	CHECK_WS_EQUAL(L"game.xml",_vfs->GetFile(L"%Testdata%/game.xml")->GetName());

	std::auto_ptr<MGDF::IFileFilter> filter1(_vfs->GetFilterFactory()->CreateFileExtensionInclusionFilter(L"xml"));
	IFileIterator *files = _vfs->FindFiles(L".",filter1.get(),true);
	UINT32 size=0;
	while (files->HasNext()) 
	{
		files->Next();
		++size;
	}
	CHECK_EQUAL(5,size);
	delete files;

	files = _vfs->FindFiles(L"%testdata%",filter1.get(),false);
	size=0;
	while (files->HasNext()) 
	{
		files->Next();
		++size;
	}
	CHECK_EQUAL(3,size);
	delete files;
	
	std::auto_ptr<MGDF::IFileFilter> filter2(_vfs->GetFilterFactory()->CreateFileExtensionExclusionFilter(L"xml"));
	std::auto_ptr<MGDF::IFileFilter> filter3(_vfs->GetFilterFactory()->CreateFileExtensionInclusionFilter(L"png"));
	files = _vfs->FindFiles(L"%testdata%",filter2->ChainFilter(filter3.get()),false);
	size=0;
	while (files->HasNext()) 
	{
		files->Next();
		++size;
	}
	CHECK_EQUAL(1,size);
	delete files;

	files = _vfs->FindFiles(L"test.zip",nullptr,false);
	size=0;
	while (files->HasNext()) 
	{
		files->Next();
		++size;
	}
	CHECK_EQUAL(6,size);
	delete files;
}

/**
check that the standard filesystem is enumerated correctly by the vfs
*/
TEST_FIXTURE(VFSTestFixture,FileSystemTests)
{
	_vfs->MapDirectory((Resources::Instance().RootDir()+L"../../../tests/content").c_str(),L"",nullptr,false);

	CHECK_EQUAL(5,_vfs->GetRoot()->GetChildCount());
	CHECK_WS_EQUAL(L"test.zip",_vfs->GetFile(L"./test.zip")->GetName());
	CHECK_EQUAL(true,_vfs->GetFile(L"./test.zip")->IsArchive());
	CHECK_WS_EQUAL(L"console.json",_vfs->GetFile(L"./console.json")->GetName());
	CHECK_WS_EQUAL(L"preferences.json",_vfs->GetFile(L"./preferences.json")->GetName());
	CHECK_WS_EQUAL(L"gamestate.json",_vfs->GetFile(L"gameState.json")->GetName());
	CHECK_WS_EQUAL(L"update.json",_vfs->GetFile(L"Update.json")->GetName());
}

/**
check that files in the standard filesystem can be read from the vfs correctly
*/
TEST_FIXTURE(VFSTestFixture,FileSystemContentTests)
{
	_vfs->MapDirectory((Resources::Instance().RootDir()+L"../../../tests/content").c_str(),L"",nullptr,false);

	IFile *file = _vfs->GetFile(L"console.json");
	file->OpenFile();
	UINT32 size = static_cast<UINT32>(file->GetSize());
	char *data = new char[size];
	file->Read(data,size);
	std::string contents(data,size);
	delete[] data;
	file->CloseFile();

	std::vector<std::string> list;
	boost::algorithm::replace_all(contents,"\r","");
	boost::algorithm::replace_all(contents,"\t","");
	boost::split(list,contents,boost::is_any_of("\n"));

	//see if the file has as many lines as we expect
	CHECK_EQUAL(14,list.size());
	//check to see the first and last lines are as expected
	CHECK_EQUAL("{",list[0]);
	CHECK_EQUAL("}",list[13]);
}

}