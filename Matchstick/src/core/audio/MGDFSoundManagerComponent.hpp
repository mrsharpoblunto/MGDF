#pragma once

#include "../common/MGDFSystemComponent.hpp"
#include <MGDF/MGDFSoundManager.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <MGDF/MGDFDisposable.hpp>

namespace MGDF { namespace core { namespace audio {

class ISoundManagerComponent: public ISystemComponent,public ISoundManager,public IDisposable
{
public:
	virtual void Update()=0;
};

}}}