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

	std::string Get();
	void Set(std::string directory);
	void Push(std::string directory);
	void Pop();
private:
	CurrentDirectoryHelper();
	std::string _currentDirectory;
	std::list<std::string> _currentDirectories;

	void SetDirectory(std::string directory);
};

}}