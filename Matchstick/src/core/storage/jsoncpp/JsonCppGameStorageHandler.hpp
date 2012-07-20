#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFGameStorageHandler.hpp"

namespace MGDF { namespace core { namespace storage { namespace jsoncppImpl {

class JsonCppGameStorageHandler: public DisposeImpl<IGameStorageHandler> 
{
public:
	JsonCppGameStorageHandler(){};
	virtual ~JsonCppGameStorageHandler(){};

	virtual void Dispose();

	virtual std::string GetGameName() const;
	virtual std::string GetGameUid() const;
	virtual int GetInterfaceVersion() const;
	virtual const Version *GetVersion() const;
	virtual std::string GetParameterString() const;

	virtual void Load(const std::wstring &);

private:
	std::string _gameName,_gameUid,_parameterString;
	int _interfaceVersion;
	Version _version;
};

}}}}