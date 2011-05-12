#pragma once

#include "../../MGDFDefaultFileImpl.hpp"

namespace MGDF { namespace core { namespace vfs { namespace zip {

/**
 this class acts the same as a normal file in all ways except that it has a non empty archive name and returns true as
 an archive. This allows for the actual phsyical zip file to be accessed like any other file, but it also allows for 
 accessing the contents of the zip file as if it were a folder
 */
class ZipFileRoot: public DefaultFileImpl 
{
	friend class ZipArchive;
public:
	virtual ~ZipFileRoot();
	virtual bool IsArchive() const;
	virtual const char *GetArchiveName() const;
private:
	ZipFileRoot(std::string path,ILogger *logger,IErrorHandler *errorHandler);
	std::string _archiveName;
};

}}}}