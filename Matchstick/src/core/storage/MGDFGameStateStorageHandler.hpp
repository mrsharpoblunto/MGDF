#pragma once

#include <MGDF/MGDFVersion.hpp>
#include <exception>
#include <string>
#include <vector>
#include "../common/MGDFVersionHelper.hpp"

namespace MGDF { namespace core { namespace storage {

class IGameStateStorageHandler
{
public:
	virtual ~IGameStateStorageHandler(){}
	virtual void Load(const std::wstring &load)=0;
	virtual void Save(const std::wstring &save) const=0;

	virtual std::string GetGameUid() const=0;
	virtual void SetVersion(const Version *version)=0;
	virtual const Version *GetVersion() const=0;
};

}}}