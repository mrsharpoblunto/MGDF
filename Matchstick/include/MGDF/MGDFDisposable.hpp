#pragma once

namespace MGDF {

	/**
	base class which can be passed across from the system to a module and freed successfully by the module
	*/
	class IDisposable
	{
	public:
		void operator delete(void* p) {
			if (p) {
				IDisposable* id = static_cast<IDisposable*>(p);
				id->Dispose();
			}
		}

	protected:
		/**
		call the implementation classes destructor
		*/
		virtual void Dispose()=0;
	};

	//any class that implements IDipsosable must also inherit from DisposeImpl
	template<typename T> class DisposeImpl : public T {
	public:
		void operator delete(void* p) {
			::operator delete(p);
		}
	};
}