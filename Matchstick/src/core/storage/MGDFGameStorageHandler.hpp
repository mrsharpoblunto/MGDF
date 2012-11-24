#pragma once

#include <exception>
#include <string>
#include <vector>
#include <MGDF/MGDFVersion.hpp>
#include <MGDF/MGDFDisposable.hpp>

namespace MGDF { namespace core { namespace storage {

class IGameStorageHandler: public IDisposable
{
public:
	virtual std::string GetGameName() const=0;
	virtual std::string GetGameUid() const=0;
	virtual const Version *GetVersion() const=0;
	virtual INT32 GetInterfaceVersion() const=0;
	virtual std::string GetParameterString() const=0;
	virtual void Load(const std::wstring &load)=0;
};

}}}