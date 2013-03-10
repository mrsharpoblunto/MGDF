#include "stdafx.h"

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

	struct VFSTestFixture {
		VFSTestFixture() {
			HINSTANCE inst;
			inst = ( HINSTANCE ) GetModuleHandleW( L"core.tests.exe" );
			Resources::Instance( inst );
			Resources::Instance().SetUserBaseDir( true, "junkship" );

			_errorHandler = new MGDF::core::tests::MockErrorHandler();

			_vfs = CreateVirtualFileSystemComponentImpl();
			_vfs->RegisterArchiveHandler( zip::CreateZipArchiveHandlerImpl( ( IErrorHandler * ) _errorHandler ) );
		}

		virtual ~VFSTestFixture() {
			delete _vfs;
			delete _errorHandler;
		}
	protected:
		IVirtualFileSystemComponent *_vfs;
		MGDF::core::tests::MockErrorHandler *_errorHandler;
	};

	/**
	check that zip archives are enumerated correctly by the vfs
	*/
	TEST_FIXTURE( VFSTestFixture, ZipArchiveTests ) {
		_vfs->Mount( ( Resources::Instance().RootDir() + L"../../../tests/content/test.zip" ).c_str() );

		CHECK_WS_EQUAL( L"test.zip", _vfs->GetRoot()->GetName() );
		CHECK_EQUAL( 6, _vfs->GetRoot()->GetChildCount() );
		CHECK_WS_EQUAL( L"game.xml", _vfs->GetFile( L"game.xml" )->GetName() );
		CHECK_WS_EQUAL( L"gameIcon.png", _vfs->GetFile( L"gameIcon.png" )->GetName() );
		CHECK_WS_EQUAL( L"preferences.xml", _vfs->GetFile( L"preferences.xml" )->GetName() );
		CHECK_WS_EQUAL( L"preferenceTemplates.xml", _vfs->GetFile( L"preferenceTemplates.xml" )->GetName() );
		CHECK_WS_EQUAL( L"gameState.xml", _vfs->GetFile( L"boot/gameState.xml" )->GetName() );
		CHECK_WS_EQUAL( L"persistency.xml", _vfs->GetFile( L"boot/persistency.xml" )->GetName() );
		CHECK_WS_EQUAL( L"test.lua", _vfs->GetFile( L"content/test.lua" )->GetName() );
		CHECK( _vfs->GetFile( L"content" )->IsFolder() );
	}

	/*
	check that files inside enumeratoed archives can be read correctly
	*/
	TEST_FIXTURE( VFSTestFixture, ZipArchiveContentTests ) {
		_vfs->Mount( ( Resources::Instance().RootDir() + L"../../../tests/content/test.zip" ).c_str() );

		IFile *file = _vfs->GetFile( L"content/test.lua" );
		file->OpenFile();
		UINT32 size = static_cast<UINT32>( file->GetSize() );
		char* data = new char[size];
		file->Read( ( void * ) data, size );
		std::string contents( data, size );
		delete[] data;
		file->CloseFile();

		std::vector<std::string> list;
		boost::algorithm::replace_all( contents, "\r", "" );
		boost::algorithm::replace_all( contents, "\t", "" );
		boost::split( list, contents, boost::is_any_of( "\n" ) );

		//see if the file has as many lines as we expect
		CHECK_EQUAL( 27, list.size() );
		//check to see the first and last lines are as expected
		CHECK_EQUAL( "class 'ConsoleStorageListener'(MGDF.StorageListener)", list[0] );
		CHECK_EQUAL( "end", list[26] );
	}

	class ContainsFilter: public MGDF::IFileFilter
	{
	public:
		ContainsFilter( std::wstring match )
			: _match( match ) {
		}
		virtual ~ContainsFilter() {}
		virtual bool Accept( const wchar_t *file ) const {
			std::wstring temp( file );
			return temp.find( _match ) != std::wstring::npos;
		}
	private:
		std::wstring _match;
	};

	/**
	check that vfs filters and aliases work as expected
	*/
	TEST_FIXTURE( VFSTestFixture, AliasAndFilterTests ) {
		_vfs->Mount( ( Resources::Instance().RootDir() + L"../../../tests/content/test.zip" ).c_str() );

		ContainsFilter filter1( L"xml" );
		IFile *buffer1[3];
		size_t len1 = 3;
		CHECK( _vfs->GetRoot()->GetAllChildren( &filter1, buffer1, &len1 ) );
		CHECK_EQUAL( 3, len1 );
		CHECK_WS_EQUAL( L"game.xml", buffer1[0]->GetName() );
		CHECK_WS_EQUAL( L"preferenceTemplates.xml", buffer1[1]->GetName() );
		CHECK_WS_EQUAL( L"preferences.xml", buffer1[2]->GetName() );

		CHECK( !_vfs->GetRoot()->GetAllChildren( nullptr, buffer1, &len1 ) );
		CHECK_EQUAL( 6, len1 );

		IFile *buffer2[6];
		CHECK( _vfs->GetRoot()->GetAllChildren( nullptr, buffer2, &len1 ) );
		CHECK_EQUAL( 6, len1 );
		CHECK_WS_EQUAL( L"boot", buffer2[0]->GetName() );
		CHECK_WS_EQUAL( L"content", buffer2[1]->GetName() );
		CHECK_WS_EQUAL( L"game.xml", buffer2[2]->GetName() );
		CHECK_WS_EQUAL( L"gameIcon.png", buffer2[3]->GetName() );
		CHECK_WS_EQUAL( L"preferenceTemplates.xml", buffer2[4]->GetName() );
		CHECK_WS_EQUAL( L"preferences.xml", buffer2[5]->GetName() );
	}

	/**
	check that the standard filesystem is enumerated correctly by the vfs
	*/
	TEST_FIXTURE( VFSTestFixture, FileSystemTests ) {
		_vfs->Mount( ( Resources::Instance().RootDir() + L"../../../tests/content" ).c_str() );

		CHECK_EQUAL( 5, _vfs->GetRoot()->GetChildCount() );
		CHECK_WS_EQUAL( L"test.zip", _vfs->GetFile( L"test.zip" )->GetName() );
		CHECK_EQUAL( true, _vfs->GetFile( L"test.zip" )->IsArchive() );
		CHECK_WS_EQUAL( L"console.json", _vfs->GetFile( L"console.json" )->GetName() );
		CHECK_WS_EQUAL( L"preferences.json", _vfs->GetFile( L"preferences.json" )->GetName() );
		CHECK_WS_EQUAL( L"gameState.json", _vfs->GetFile( L"gameState.json" )->GetName() );
		CHECK_WS_EQUAL( L"Update.json", _vfs->GetFile( L"Update.json" )->GetName() );
	}

	/**
	check that files in the standard filesystem can be read from the vfs correctly
	*/
	TEST_FIXTURE( VFSTestFixture, FileSystemContentTests ) {
		_vfs->Mount( ( Resources::Instance().RootDir() + L"../../../tests/content" ).c_str() );

		IFile *file = _vfs->GetFile( L"console.json" );
		file->OpenFile();
		UINT32 size = static_cast<UINT32>( file->GetSize() );
		char *data = new char[size];
		file->Read( data, size );
		std::string contents( data, size );
		delete[] data;
		file->CloseFile();

		std::vector<std::string> list;
		boost::algorithm::replace_all( contents, "\r", "" );
		boost::algorithm::replace_all( contents, "\t", "" );
		boost::split( list, contents, boost::is_any_of( "\n" ) );

		//see if the file has as many lines as we expect
		CHECK_EQUAL( 14, list.size() );
		//check to see the first and last lines are as expected
		CHECK_EQUAL( "{", list[0] );
		CHECK_EQUAL( "}", list[13] );
	}

}