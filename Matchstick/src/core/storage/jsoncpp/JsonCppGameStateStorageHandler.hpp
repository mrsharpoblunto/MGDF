#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFGameStateStorageHandler.hpp"

namespace MGDF { namespace core { namespace storage { namespace jsoncppImpl {

class JsonCppGameStateStorageHandler: public IGameStateStorageHandler
{
public:
	
	JsonCppGameStateStorageHandler(const std::string &gameUid,const Version *version) 
		: _gameUid( gameUid )
		, _version( VersionHelper::Copy(version) )
	{
	}
	virtual ~JsonCppGameStateStorageHandler(){}

	virtual void Load(const std::wstring &);
	virtual void Save(const std::wstring &) const;
	
	std::string GetGameUid() const { return _gameUid; };
	void SetVersion(const Version *version) { _version =  VersionHelper::Copy(version); };
	const Version *GetVersion() const { return &_version; };

private:	
	std::string _gameUid;
	Version _version;
};

}}}}