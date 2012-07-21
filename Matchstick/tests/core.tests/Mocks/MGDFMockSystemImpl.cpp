#include "MGDFMockModule.hpp"
#include "MGDFMockSystemImpl.hpp"

namespace MGDF { namespace core { namespace tests {

MockSystemImpl::MockSystemImpl()
{
	_lastError.Description=nullptr;
	_lastError.Sender=nullptr;
	_module = nullptr;
}

MockSystemImpl::~MockSystemImpl(void)
{
	if (_module!=nullptr) delete _module;

	if (_lastError.Description!=nullptr)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=nullptr)
	{
		delete[] _lastError.Sender;
	}
}

IModule *MockSystemImpl::GetModule()
{
	return _module;
}

const Version *MockSystemImpl::GetMGDFVersion() const
{
	return &_version;
}

void MockSystemImpl::SetLastError(const char *sender,int code,const char *description)
{
	if (_lastError.Description!=nullptr)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=nullptr)
	{
		delete[] _lastError.Sender;
	}

	if (description!=nullptr)
	{
		int size = strlen(description)+1;
		_lastError.Description = new char[size];
		strcpy_s(_lastError.Description,size,description);
	}
	if (sender!=nullptr)
	{
		int size = strlen(sender)+1;
		_lastError.Sender = new char[size];
		strcpy_s(_lastError.Sender,size,sender);
	}
}

void MockSystemImpl::FatalError(const char *s,const char *m)
{
	std::string source = s;
	std::string message = m;
	_fatalErrorFunction(source,message);
}

void MockSystemImpl::ShutDown()
{
	_shutDownFunction();
}

}}}