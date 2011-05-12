#pragma once

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

template<typename T>
class BufferedGameState
{
public:
	BufferedGameState()
	{
		_pendingState=new T();
		_currentState=NULL;
		_previousState=NULL;
	};

	virtual ~BufferedGameState()
	{
		if (_pendingState) delete _pendingState;
		if (_currentState)  delete _currentState;
		if (_previousState)  delete _previousState;
	};

	T *Pending()
	{
		return _pendingState;
	}

	boost::shared_ptr<T> Interpolate(double alpha)
	{
		boost::mutex::scoped_lock lock(_stateMutex);
		if (_previousState!=NULL && _currentState!=NULL)
		{
			return boost::shared_ptr<T>(_previousState->Interpolate(_currentState,alpha));
		}
		return boost::shared_ptr<T>();
	}

	void Flip()
	{
		boost::mutex::scoped_lock lock(_stateMutex);
		if (_previousState) delete _previousState;
		_previousState = _currentState;
		_currentState = new T(_pendingState);

	}

private:
	boost::mutex _stateMutex;
	T *_pendingState;
	T *_currentState;
	T *_previousState;
};
