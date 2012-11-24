#pragma once

#include <vector>
#include <MGDF/MGDFList.hpp>

namespace MGDF { namespace core {

template<typename I,typename T> class ListImpl: public I
{
public:
	ListImpl(){
	}

	virtual ~ListImpl(void){
	}

	void Add(T item)
	{
		_items.push_back(item);
	}

	void Remove(size_t index)
	{
		_items.erase(_items.begin()+index);
	}

	void  Clear()
	{
		_items.clear();
	}

	virtual size_t Size() const
	{
		return _items.size();
	}

	virtual T Get(size_t index) const {
		return _items[index];
	}

	const std::vector<T> *Items() const {
		return &_items;
	}

private:
	std::vector<T> _items;
};

}}