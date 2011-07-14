#pragma once

#include <string>
#include <list>

namespace MGDF { namespace core { 

class CurrentDirectoryHelper
{
public:
	static CurrentDirectoryHelper *InstancePtr() {
		static CurrentDirectoryHelper c;
		return &c;
	}

	static CurrentDirectoryHelper &Instance() {
		return *CurrentDirectoryHelper::InstancePtr();
	}

	virtual ~CurrentDirectoryHelper(){};

	std::wstring Get();
	void Set(std::wstring directory);
	void Push(std::wstring directory);
	void Pop();
private:
	CurrentDirectoryHelper();
	std::wstring _currentDirectory;
	std::list<std::wstring> _currentDirectories;

	void SetDirectory(std::wstring directory);
};

}}