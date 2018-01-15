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
};

}
}