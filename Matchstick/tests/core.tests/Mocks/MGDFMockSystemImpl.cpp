#include "MGDFMockModule.hpp"
#include "MGDFMockSystemImpl.hpp"

namespace MGDF { namespace core { namespace tests {

MockSystemImpl::MockSystemImpl()
{
	_lastError.Description=NULL;
	_lastError.Sender=NULL;
}

MockSystemImpl::~MockSystemImpl(void)
{
	QueuePopModules(_moduleStack.size()-1);
	if (_lastError.Description!=NULL)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=NULL)
	{
		delete[] _lastError.Sender;
	}
}

IModule *MockSystemImpl::GetTopModule()
{
	if (_moduleStack.size()>0) {
		return *(_moduleStack.begin());
	}
	return NULL;
}

const Version *MockSystemImpl::GetMGDFVersion() const
{
	return &_version;
}

void MockSystemImpl::SetLastError(const char *sender,int code,const char *description)
{
	if (_lastError.Description!=NULL)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=NULL)
	{
		delete[] _lastError.Sender;
	}

	if (description!=NULL)
	{
		int size = strlen(description)+1;
		_lastError.Description = new char[size];
		strcpy_s(_lastError.Description,size,description);
	}
	if (sender!=NULL)
	{
		int size = strlen(sender)+1;
		_lastError.Sender = new char[size];
		strcpy_s(_lastError.Sender,size,sender);
	}
}

void MockSystemImpl::QueuePushNewModule(const char *n,IModuleInitialiser *init)
{
	std::string name = n;
	if (_moduleStack.size()>0) {
		_moduleSuspendedFunction(GetTopModule());
	}
	_moduleStack.push_front(new MockModule(name,this));
}

void MockSystemImpl::QueuePopModules(unsigned int count)
{
	for (int i=count>(_moduleStack.size()-1)?(_moduleStack.size()-1):count;i>0;--i) {
		IModule *module = GetTopModule();
		_modulePoppedFunction(module);
		delete module;
		_moduleStack.pop_front();
	}

	if (_moduleStack.size()>0) {
		_moduleResumedFunction(GetTopModule());
	}
}

void MockSystemImpl::QueueSwapTopModule(const char *n,IModuleInitialiser *init)
{
	std::string name = n;

	//old module gets popped off the stack
	IModule *module = GetTopModule();
	_modulePoppedFunction(module);
	delete module;
	_moduleStack.pop_front();

	if (_moduleStack.size()>0) {
		IModule *module = GetTopModule();
		_moduleResumedFunction(module);//this causes the no.2 to be resumed
		_moduleSuspendedFunction(module);//but because we're adding a new one straight away it is immediatly suspended again
	}

	_moduleStack.push_front(new MockModule(name,this));
}


std::list<IModule *> *MockSystemImpl::GetModules()
{
	return &_moduleStack;
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