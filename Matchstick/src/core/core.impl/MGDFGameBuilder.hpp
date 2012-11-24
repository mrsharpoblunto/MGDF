#pragma once

#include "MGDFGameImpl.hpp"
#include "../storage/MGDFStorageFactoryComponentImpl.hpp"

namespace MGDF { namespace core {

class GameBuilder {
public:
	static Game *LoadGame(storage::IGameStorageHandler *handler);
private:
	static Game *CreateGame(const std::string &uid,const std::string &name,INT32 interfaceVersion,const Version *version);
};

}}