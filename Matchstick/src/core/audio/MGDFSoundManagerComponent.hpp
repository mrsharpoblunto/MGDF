#pragma once

#include "../common/MGDFSystemComponent.hpp"
#include <MGDF/MGDFSoundManager.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF
{
namespace core
{
namespace audio
{

class ISoundManagerComponent: public ISystemComponent, public ISoundManager
{
public:
	virtual ~ISoundManagerComponent() {}
	virtual void Update() = 0;
};

}
}
}