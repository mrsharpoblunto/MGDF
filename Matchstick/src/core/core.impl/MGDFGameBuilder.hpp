#pragma once

#include "MGDFGameImpl.hpp"

namespace MGDF { namespace core {

class GameBuilder {
public:
	static Game *LoadGame();
private:
	static Game *CreateGame(const std::string &uid,const std::string &name,int interfaceVersion,const Version *version);
};

}}