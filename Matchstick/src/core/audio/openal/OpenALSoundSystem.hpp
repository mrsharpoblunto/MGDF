#pragma once

#include "al.h"
#include "alc.h"
#include <boost/unordered_map.hpp>
#include <stack>

namespace MGDF { namespace core { namespace audio { namespace openal_audio {

#include "../../common/MGDFSingleton.hpp"

class OpenALSoundSystem : public Singleton<OpenALSoundSystem>
{
public:
	OpenALSoundSystem(void);
	~OpenALSoundSystem(void);

	static OpenALSoundSystem *SafeNew();
	static void SafeDelete();

	DECLARE_SINGLETON(OpenALSoundSystem);

	ALCcontext *GetContext(){return _context;}

	size_t GetFreeSources();
	bool AcquireSource(ALuint *source);
	void ReleaseSource(ALuint source);
private:
	boost::unordered_map<ALuint,bool> _allocatedSources;
	std::stack<ALuint> _freeSources;

	ALCcontext *_context;
	static INT32 _refCount;
};

}}}}
