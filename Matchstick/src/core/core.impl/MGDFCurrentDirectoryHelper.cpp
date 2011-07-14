#include "StdAfx.h"

#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFCurrentDirectoryHelper.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

CurrentDirectoryHelper::CurrentDirectoryHelper()
{
	wchar_t buffer[MAX_PATH];
	DWORD ret;

	ret = GetCurrentDirectoryW(MAX_PATH,buffer);
	if( ret == 0 )
	{
		GetLoggerImpl()->Add(THIS_NAME,"unable to get current directory");
	}
	if(ret > MAX_PATH)
	{
		GetLoggerImpl()->Add(THIS_NAME,"unable to get current directory - buffer to small");
	}
	
	_currentDirectory = buffer;
	_currentDirectories.clear();
}

std::wstring CurrentDirectoryHelper::Get()
{
	if (_currentDirectories.size()>0) {
		return *(_currentDirectories.begin());
	}
	else {
		return _currentDirectory;
	}
}

void CurrentDirectoryHelper::Set(std::wstring directory)
{
	_currentDirectories.clear();
	SetDirectory(directory);
}

void CurrentDirectoryHelper::Push(std::wstring directory)
{
	_currentDirectories.push_front(directory);
	SetDirectory(directory);
}

void CurrentDirectoryHelper::Pop()
{
	if (_currentDirectories.size()>0) {
		_currentDirectories.pop_front();
	}

	if (_currentDirectories.size()>0) {
		SetDirectory(*(_currentDirectories.begin()));
	}
	else {
		SetDirectory(_currentDirectory);
	}
}

void CurrentDirectoryHelper::SetDirectory(std::wstring directory)
{
	if(directory.length() > MAX_PATH-1)
	{
		GetLoggerImpl()->Add(THIS_NAME,"cannot set directory with more than MAX_PATH characters");
	}
	else {
		SetCurrentDirectoryW(directory.c_str());
		_currentDirectory = directory;
	}
}


}}