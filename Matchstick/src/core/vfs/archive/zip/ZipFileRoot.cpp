#include "stdafx.h"

#include "ZipFileRoot.hpp"

namespace MGDF { namespace core { namespace vfs { namespace zip {

ZipFileRoot::~ZipFileRoot()
{
	if (!_children) return;
	for (auto iter=_children->begin();iter!=_children->end();++iter) {
		delete static_cast<FileBaseImpl *>(iter->second);
	}
}

}}}}