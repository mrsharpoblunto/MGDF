#pragma once

#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include "MGDFFileBaseImpl.hpp"

namespace MGDF
{
namespace core
{
namespace vfs
{

class DefaultFileImpl : public FileBaseImpl, public IFileReader
{
public:
	DefaultFileImpl( const std::wstring &name, const std::wstring &physicalPath, IFile *parent, IErrorHandler *handler );
	virtual ~DefaultFileImpl();

	bool IsOpen() const override {
		boost::mutex::scoped_lock( _mutex );
		return ( _fileStream != nullptr );
	}

	MGDFError OpenFile( IFileReader **reader ) override;

	bool IsFolder() const override{
		return false;
	}
	bool IsArchive() const override{
		return false;
	}
	const wchar_t *GetArchiveName() const override{
		return nullptr;
	}
	const wchar_t *GetPhysicalPath() const override{
		return _path.c_str();
	}
	const wchar_t *GetName() const override{
		return _name.c_str();
	}
	
	void Close() override;
	UINT32 Read( void* buffer, UINT32 length ) override;
	void SetPosition( INT64 pos ) override;
	INT64 GetPosition() const override;
	bool EndOfFile() const override;
	INT64 GetSize() const override;

private:
	std::ifstream *_fileStream;
	INT64 _filesize;
	std::wstring _name;
	std::wstring _path;
	IErrorHandler *_errorHandler;
};


}
}
}
