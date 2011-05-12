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
	typedef struct {
		std::string Name,Parameters;
	} ModuleState;

	//we need to load modules in the reverse order they were saved in order to preserve the module stack ordering.
	typedef std::vector<ModuleState>::const_reverse_iterator iterator;

	virtual void Add(std::string name)=0;

	virtual iterator Begin() const=0;
	virtual iterator End() const=0;

	virtual bool Load(std::string)=0;
	virtual void Save(std::string) const=0;

	virtual std::string GetGameUid() const=0;
	virtual void SetVersion(const Version *version)=0;
	virtual const Version *GetVersion() const=0;
};

}}}