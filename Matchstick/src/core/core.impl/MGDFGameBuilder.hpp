#pragma once

#include "MGDFGameImpl.hpp"
#include "../storage/MGDFStorageFactoryComponentImpl.hpp"

namespace MGDF
{
namespace core
{

class GameBuilder
{
public:
	static MGDFError LoadGame( storage::IGameStorageHandler *handler, Game **game );
private:
	static MGDFError CreateGame( const std::string &uid, const std::string &name, INT32 interfaceVersion, const Version *version, Game **game );
};

}
}