#pragma once

#include <boost/regex.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF { namespace core { namespace vfs { namespace filters {

class FilterBase: public DisposeImpl<IFileFilter>
{
public:
	virtual ~FilterBase(){}
	FilterBase();
	virtual void Dispose();
	virtual IFileFilter *ChainFilter(IFileFilter *filter);
	virtual bool FilterFile(const wchar_t *);
protected:
	virtual bool DoFilterFile(std::wstring file)=0;
private:
	IFileFilter *_chainedFilter;
};

}}}}