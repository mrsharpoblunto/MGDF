#pragma once

#include <al.h>
#include <alc.h>
#include <boost/unordered_map.hpp>
#include <stack>

namespace MGDF
{
namespace core
{
namespace audio
{
namespace openal_audio
{

class OpenALSoundSystem
{
public:
	OpenALSoundSystem( void );
	~OpenALSoundSystem( void );

	static OpenALSoundSystem *Instance();

	ALCcontext *GetContext() const {
		return _context;
	}
	size_t GetFreeSources() const {
		return _freeSources.size();
	}

	bool AcquireSource( ALuint *source );
	void ReleaseSource( ALuint source );
private:
	boost::unordered_map<ALuint, bool> _allocatedSources;
	std::stack<ALuint> _freeSources;

	ALCcontext *_context;

	static OpenALSoundSystem *_instance;
};

}
}
}
}
