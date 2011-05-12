#pragma once

#include <MGDF/MGDFModule.hpp>
#include <vector>
#include <hash_map>

namespace MGDF { namespace core {

	class ModuleInitialiser : public DisposeImpl<IModuleInitialiser>
{
public:
	ModuleInitialiser(void);
	virtual ~ModuleInitialiser(void);

	virtual void AddParam(const char *,const char *);
	virtual void RemoveParam(const char * key);
	virtual const char * GetParam(const char * key) const;
	virtual void Dispose();

	void AddParams(std::string params);
private:
	stdext::hash_map<std::string,std::string> _params;
};

}}
