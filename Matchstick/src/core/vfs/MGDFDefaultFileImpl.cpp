#include "StdAfx.h"

#include <cctype>//std::tolower
#include "../common/MGDFExceptions.hpp"
#include "MGDFDefaultFileImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs {

DefaultFileImpl::DefaultFileImpl(const std::wstring &path,ILogger *logger,IErrorHandler *handler)
{
	_logger = logger;
	_errorHandler = handler;
	_fileStream = nullptr;

	_path = path;

	size_t index=_path.size()-1;
	const wchar_t *_pathStr = _path.c_str();
	while (index>0)
	{
		if (_pathStr[index]==L'\\' || _pathStr[index]==L'/')
		{
			_name = &_pathStr[index+1];
			std::transform(_name.begin(), _name.end(), _name.begin(), ::towlower);
			break;
		}
		--index;
	}

	_filesize = 0;
}

DefaultFileImpl::~DefaultFileImpl(void)
{
	CloseFile();
}

bool DefaultFileImpl::IsOpen() const
{
	return (_fileStream!=nullptr);
}

bool DefaultFileImpl::OpenFile()
{
	if (!_fileStream) {
		_fileStream = new std::ifstream(_path.c_str(),std::ios::in|std::ios::binary|std::ios::ate);
		if (_fileStream && !_fileStream->bad() && _fileStream->is_open()) 
		{
			//files over 4.2GB unsupported
			if (_fileStream->tellg()>ULONG_MAX) 
			{
				_filesize = ULONG_MAX;
			}
			else
			{
				_filesize = static_cast<unsigned long>(_fileStream->tellg());
			}
			_fileStream->seekg(0, std::ios::beg);
			return true;
		}
		else 
		{
			delete _fileStream;
			_fileStream = nullptr;

			_logger->Add(THIS_NAME,"Unable to open file stream",LOG_ERROR);
			_errorHandler->SetLastError(THIS_NAME,MGDF_ERR_INVALID_FILE,"Unable to open file stream");
		}
	}
	return false;
}

void DefaultFileImpl::CloseFile()
{
	if(_fileStream) 
	{
		_fileStream->close();
		delete _fileStream;
		_fileStream = nullptr;
	}
}

unsigned int DefaultFileImpl::Read(void* buffer,unsigned int length)
{
	if(_fileStream)
	{
		std::ifstream::pos_type oldPosition = _fileStream->tellg();
		_fileStream->read((char*)buffer,length);
		std::ifstream::pos_type newPosition = _fileStream->tellg();
		return static_cast<unsigned int>(newPosition-oldPosition);
	}
	return 0;
}

void DefaultFileImpl::SetPosition(unsigned long pos)
{
	if(_fileStream)
	{
		_fileStream->seekg(pos);
	}
}

unsigned long DefaultFileImpl::GetPosition() const
{
	if(_fileStream) 
	{
		//files over 4.2GB unsupported
		if (_fileStream->tellg()>ULONG_MAX) 
		{
			return ULONG_MAX;
		}
		else
		{
			return static_cast<unsigned long>(_fileStream->tellg());
		}
	}
	else 
	{
		return 0;
	}
}

bool DefaultFileImpl::EndOfFile() const
{
	if (_fileStream)
	{
		return _fileStream->eof();
	}
	else {
		return true;
	}
}

unsigned long DefaultFileImpl::GetSize()
{
	if (_filesize==0)
	{
		std::ifstream stream(_path.c_str(),std::ios::in|std::ios::binary|std::ios::ate);
		if (!stream.bad() && stream.is_open()) 
		{
			//files over 4.2GB unsupported
			if (stream.tellg()>ULONG_MAX) 
			{
				_filesize = ULONG_MAX;
			}
			else
			{
				_filesize = static_cast<unsigned long>(stream.tellg());
			}

			stream.close();
		}
		else 
		{
			_logger->Add(THIS_NAME,"Unable to open file stream",LOG_ERROR);
			_errorHandler->SetLastError(THIS_NAME,MGDF_ERR_INVALID_FILE,"Unable to open file stream");
		}
	}

	return _filesize;
}

bool DefaultFileImpl::IsFolder() const
{ 
	return false;
}

bool DefaultFileImpl::IsArchive() const
{
	return false;
}

const wchar_t *DefaultFileImpl::GetArchiveName() const
{
	return nullptr;
}

const wchar_t *DefaultFileImpl::GetPhysicalPath() const
{ 
	return _path.c_str();
}

const wchar_t *DefaultFileImpl::GetName() const
{
	return _name.c_str();

}



}}}
