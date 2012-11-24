#include "StdAfx.h"

#include "ZipFileImpl.hpp"

// std min&max are used instead of the macros
#ifdef min
	#undef min
	#undef max
#endif

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs { namespace zip {

ZipFileImpl::~ZipFileImpl()
{
	if (_isOpen) {
		_handler->CloseFile(_fileKey);
	}

	//only if no other zip files are referencing the handler do we delete it
	_handler->DecRefCount();
	if (_handler->GetRefCount()==0) {
		delete _handler;
	}
}

bool ZipFileImpl::IsFolder() const
{
	return false;
}

bool ZipFileImpl::IsArchive() const
{
	return true;
}

bool ZipFileImpl::IsOpen() const
{
	return _isOpen;
}

bool ZipFileImpl::OpenFile()
{
	if (!_isOpen) {
		_isOpen = _handler->OpenFile(_fileKey);
		return _isOpen;
	}
	return false;
}

void ZipFileImpl::CloseFile()
{
	if (_isOpen) {
		_handler->CloseFile(_fileKey);
		_isOpen = false;
	}
}

UINT32 ZipFileImpl::Read(void* buffer,UINT32 length)
{
	UINT32 maxRead = 0;
	if (_isOpen) {
		ZipArchive::ZipFileData *zfData = _handler->GetFileData(_fileKey);
		ZipArchive::ZipFileInformation *zfInformation = _handler->GetFileInformation(_fileKey);

		maxRead = std::min<UINT32>(length, static_cast<UINT32>(zfInformation->size - zfData->readPosition));
		memcpy(buffer, zfData->data + zfData->readPosition, maxRead);
		zfData->readPosition += maxRead;
	}

	return maxRead;
}

void ZipFileImpl::SetPosition(INT64 pos)
{
	if (_isOpen) {
		_handler->GetFileData(_fileKey)->readPosition = pos;
	}
}

INT64 ZipFileImpl::GetPosition() const
{
	if (_isOpen) {
		return _handler->GetFileData(_fileKey)->readPosition;
	}
	return -1;
}

bool ZipFileImpl::EndOfFile() const
{
	if (_isOpen) {
		ZipArchive::ZipFileData *zfData = _handler->GetFileData(_fileKey);
		ZipArchive::ZipFileInformation *zfInformation = _handler->GetFileInformation(_fileKey);

		return (zfData->readPosition >= zfInformation->size);
	}
	return true;
}

INT64 ZipFileImpl::GetSize()
{
	return _handler->GetFileInformation(_fileKey)->size;
}

const wchar_t * ZipFileImpl::GetArchiveName() const
{
	return _handler->GetArchiveRoot()->GetName();
}

const wchar_t * ZipFileImpl::GetPhysicalPath() const
{
	return _handler->GetArchiveRoot()->GetPhysicalPath();
}

const wchar_t * ZipFileImpl::GetName() const
{
	return _handler->GetFileInformation(_fileKey)->name.c_str();
}

}}}}