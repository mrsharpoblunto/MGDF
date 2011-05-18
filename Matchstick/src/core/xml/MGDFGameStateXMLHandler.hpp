#pragma once

#include <MGDF/MGDFVersion.hpp>
#include <MGDF/MGDFDisposable.hpp>
#include <exception>
#include <string>
#include <vector>
#include "../common/MGDFVersionHelper.hpp"

namespace MGDF { namespace core { namespace xml {

class IGameStateXMLHandler: public IDisposable
{
public:
	virtual bool Load(std::string)=0;
	virtual void Save(std::string) const=0;

	virtual std::string GetGameUid() const=0;
	virtual void SetVersion(const Version *version)=0;
	virtual const Version *GetVersion() const=0;
};

}}}