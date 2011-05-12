#pragma once

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF { namespace core { namespace vfs { namespace filters {

class FileFilterFactory: public IFileFilterFactory
{
public:
	FileFilterFactory(){}
	virtual ~FileFilterFactory(){}
	/**
	creates a filter which filters out all instances of files with a matching name
	the name can include regexes
	*/
	virtual IFileFilter *CreateNameExclusionFilter(const char * name) const;

	/**
	creates a filter which filters in all instances of files without a matching name
	the name can include regexes
	*/
	virtual IFileFilter *CreateNameInclusionFilter(const char * name) const;

	/**
	creates a filter which filters in all instance of files with a matching extension
	\param extension the extension to filter (excluding the preceding '.')
	*/
	virtual IFileFilter *CreateFileExtensionExclusionFilter(const char * extension) const;

	/**
	creates a filter which filters out all instance of files with a matching extension
	\param extension the extension to filter (excluding the preceding '.')
	*/
	virtual IFileFilter *CreateFileExtensionInclusionFilter(const char * extension) const;
};

}}}}
