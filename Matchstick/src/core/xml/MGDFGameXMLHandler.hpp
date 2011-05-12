#pragma once

#include <exception>
#include <string>
#include <vector>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <MGDF/MGDFVersion.hpp>
#include <MGDF/MGDFDisposable.hpp>

namespace MGDF { namespace core { namespace xml {

class IGameXMLHandler: public IDisposable
{
public:
	virtual std::string GetGameName() const=0;
	virtual std::string GetGameUid() const=0;
	virtual const Version *GetVersion() const=0;
	virtual int GetInterfaceVersion() const=0;
	virtual std::string GetParameterString() const=0;
	virtual void Load(std::string)=0;
};

}}}