#include "StdAfx.h"

#include <limits.h>
#include "../../common/MGDFExceptions.hpp"
#include "OpenALSoundSystem.hpp"

#include "VorbisStream.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace audio { namespace openal_audio {

INT32 VorbisStream::_references=0;
HINSTANCE VorbisStream::_vorbisInstance=nullptr;
LPOVCLEAR VorbisStream::fn_ov_clear=nullptr;
LPOVREAD VorbisStream::fn_ov_read=nullptr;
LPOVPCMTOTAL VorbisStream::fn_ov_pcm_total=nullptr;
LPOVINFO VorbisStream::fn_ov_info=nullptr;
LPOVCOMMENT VorbisStream::fn_ov_comment=nullptr;
LPOVOPENCALLBACKS VorbisStream::fn_ov_open_callbacks=nullptr;

VorbisStream::~VorbisStream()
{
	if (--_references==0)
	{
		UninitVorbis();
	}
	UninitStream();
	_soundManager->RemoveSoundStream(this);
}

void VorbisStream::Dispose()
{
	delete this;
}

void VorbisStream::UninitStream()
{
	if (_initLevel == 5)
	{
		alSourceStop(_source);
		alSourcei(_source, AL_BUFFER, 0);
		OpenALSoundSystem::Instance()->ReleaseSource(_source);
	}

	if (_initLevel >= 4) {
		alDeleteBuffers( VORBIS_BUFFER_COUNT, _buffers);
	}

	if (_initLevel >= 3 && _decodeBuffer!=nullptr)
	{
		delete[] _decodeBuffer;
	}

	if (_initLevel >= 2) {// Close OggVorbis stream
		fn_ov_clear(&_vorbisFile);
	}

	//close the datasource
	if (_initLevel >= 1 && _dataSource->IsOpen()) {
		_dataSource->CloseFile();
	}
	_initLevel = 0;
}

const wchar_t * VorbisStream::GetName() const
{
	return _dataSource->GetName();
}

bool VorbisStream::IsStopped() const
{
	return _state==STOP;
}

bool VorbisStream::IsPaused() const
{
	return _state==PAUSE;
}

bool VorbisStream::IsPlaying() const
{
	return _state==PLAY;
}

VorbisStream::VorbisStream(IFile *source,OpenALSoundManagerComponentImpl *manager)
{
	_dataSource = source;
	_soundManager = manager;
	_globalVolume = _soundManager->GetStreamVolume();
	_volume = 1.0;

	if (_references++==0)
	{
		if (!InitVorbis()) 
		{
			_references--;
			SETLASTERROR(_soundManager->GetComponentErrorHandler(),MGDF_ERR_VORBIS_LIB_LOAD_FAILED,"Failed to find OggVorbis DLLs (vorbisfile.dll, ogg.dll, or vorbis.dll)");
			throw MGDFException("Failed to find OggVorbis DLLs (vorbisfile.dll, ogg.dll, or vorbis.dll)");
		}
	}

	InitStream();
}

void VorbisStream::InitStream()
{	
	_initLevel = 0;
	try 
	{
		_state = NOT_STARTED;

		if (!_dataSource->IsOpen()) {
			_dataSource->OpenFile();
		}
		_initLevel++;//data source open

		_totalBuffersProcessed=0;
		_frequency = 0;
		_format = 0;
		_channels = 0;

		ov_callbacks callbacks;
		callbacks.read_func = &VorbisStream::ov_read_func;
		callbacks.seek_func = &VorbisStream::ov_seek_func;
		callbacks.close_func = &VorbisStream::ov_close_func;
		callbacks.tell_func = &VorbisStream::ov_tell_func;

		INT32 retVal = fn_ov_open_callbacks(_dataSource, &_vorbisFile, nullptr, 0, callbacks);

		// Create an OggVorbis file stream
		if (retVal == 0)
		{
			_initLevel++;//vorbis file stream created

			// Get some information about the file (Channels, Format, and Frequency)
			_vorbisInfo = fn_ov_info(&_vorbisFile, -1);
			if (_vorbisInfo!=nullptr)
			{
				_frequency = _vorbisInfo->rate;
				_channels = _vorbisInfo->channels;
				if (_vorbisInfo->channels == 1)
				{
					_format = AL_FORMAT_MONO16;
					// Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
					_bufferSize = _frequency >> 1;
					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					_bufferSize -= (_bufferSize % 2);
				}
				else if (_vorbisInfo->channels == 2)
				{
					_format = AL_FORMAT_STEREO16;
					// Set BufferSize to 250ms (Frequency * 4 (16bit stereo) divided by 4 (quarter of a second))
					_bufferSize = _frequency;
					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					_bufferSize -= (_bufferSize % 4);
				}
				else if (_vorbisInfo->channels == 4)
				{
					_format = alGetEnumValue("AL_FORMAT_QUAD16");
					// Set BufferSize to 250ms (Frequency * 8 (16bit 4-channel) divided by 4 (quarter of a second))
					_bufferSize = _frequency * 2;
					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					_bufferSize -= (_bufferSize % 8);
				}
				else if (_vorbisInfo->channels == 6)
				{
					_format = alGetEnumValue("AL_FORMAT_51CHN16");
					// Set BufferSize to 250ms (Frequency * 12 (16bit 6-channel) divided by 4 (quarter of a second))
					_bufferSize = _frequency * 3;
					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					_bufferSize -= (_bufferSize % 12);
				}
			}

			if (_format != 0)
			{
				// Allocate a buffer to be used to store decoded data for all Buffers
				_decodeBuffer = new char[_bufferSize];
				_initLevel++;//decode buffer allocated

				alGenBuffers( VORBIS_BUFFER_COUNT, _buffers );
				_initLevel++;//vorbis stream buffers generated

				bool createdSource = OpenALSoundSystem::Instance()->AcquireSource(&_source);
				if (!createdSource)
				{
					SETLASTERROR(_soundManager->GetComponentErrorHandler(),MGDF_ERR_NO_FREE_SOURCES,"No free sound sources to create stream");
					throw MGDFException("No free sound sources");
				}
				_initLevel++;//sound source created

				// Fill all the Buffers with decoded audio data from the OggVorbis file
				INT32 bytesWritten;
				for (INT32 i = 0; i < VORBIS_BUFFER_COUNT; ++i)
				{
					bytesWritten = DecodeOgg(&_vorbisFile, _decodeBuffer, _bufferSize, _channels);
					if (bytesWritten)
					{
						alBufferData(_buffers[i], _format, _decodeBuffer, bytesWritten, _frequency);
						alSourceQueueBuffers(_source, 1, &_buffers[i]);
					}
				}
			}
			else 
			{
				SETLASTERROR(_soundManager->GetComponentErrorHandler(),MGDF_ERR_INVALID_FORMAT,"Failed to find format information, or unsupported format");
				throw MGDFException("Failed to find format information, or unsupported format");
			}
		}
	}
	catch (...)
	{
		if (--_references==0)
		{
			UninitVorbis();
		}
		UninitStream();
	}
	SetVolume(_volume);
}

bool VorbisStream::InitVorbis()
{
	_vorbisInstance = LoadLibrary("vorbisfile.dll");
	if (_vorbisInstance!=nullptr)
	{
		fn_ov_clear = (LPOVCLEAR)GetProcAddress(_vorbisInstance, "ov_clear");
		fn_ov_read = (LPOVREAD)GetProcAddress(_vorbisInstance, "ov_read");
		fn_ov_pcm_total = (LPOVPCMTOTAL)GetProcAddress(_vorbisInstance, "ov_pcm_total");
		fn_ov_info = (LPOVINFO)GetProcAddress(_vorbisInstance, "ov_info");
		fn_ov_comment = (LPOVCOMMENT)GetProcAddress(_vorbisInstance, "ov_comment");
		fn_ov_open_callbacks = (LPOVOPENCALLBACKS)GetProcAddress(_vorbisInstance, "ov_open_callbacks");

		if (fn_ov_clear && fn_ov_read && fn_ov_pcm_total && fn_ov_info &&
			fn_ov_comment && fn_ov_open_callbacks)
		{
			return true;
		}
	}
	return false;
}

void VorbisStream::UninitVorbis()
{
	if (_vorbisInstance!=nullptr)
	{
		FreeLibrary(_vorbisInstance);
		_vorbisInstance = nullptr;
	}
}

void VorbisStream::Play()
{
	if (_state == NOT_STARTED) {
		alSourcef(_source,AL_GAIN,_volume * _globalVolume);
		alSourcePlay(_source);
	}
	else if (_state == PAUSE)
	{
		alSourcePlay(_source);
	}
	else if (_state == STOP) {
		UninitStream();
		InitStream();
		alSourcef(_source,AL_GAIN,_volume * _globalVolume);
		alSourcePlay(_source);
	}

	_state = PLAY;
}

void VorbisStream::Pause()
{
	if (_state == PLAY) {
		alSourcePause(_source);
		_state=PAUSE;
	}
}

void VorbisStream::Stop()
{
	if (_state != STOP) {
		alSourceStop(_source);
		_state = STOP;
	}
}

void VorbisStream::SetGlobalVolume(float globalVolume){
	_globalVolume = globalVolume;
	SetVolume(_volume);
}


float VorbisStream::GetVolume() const
{
	return _volume;
}

void VorbisStream::SetVolume(float volume) {
	_volume = volume;
	alSourcef(_source,AL_GAIN,_volume * _globalVolume);
}

UINT32 VorbisStream::GetPosition()
{
	double position = ov_time_tell(&_vorbisFile);
	double bufferOffset = position - ((VORBIS_BUFFER_COUNT -1) * 0.25);
	float currentOffset;
	alGetSourcef(_source, AL_SEC_OFFSET, &currentOffset);
	double actualOffset = bufferOffset + currentOffset;
	return (UINT32)actualOffset*1000;
}

UINT32 VorbisStream::GetLength()
{
	return (UINT32)(ov_time_total(&_vorbisFile,-1)*1000);
}

void VorbisStream::Update()
{
	if (_state != PAUSE && _state != STOP) {
		// Request the number of OpenAL Buffers have been processed (played) on the Source
		ALint buffersProcessed = 0;
		alGetSourcei(_source, AL_BUFFERS_PROCESSED, &buffersProcessed);

		_totalBuffersProcessed += buffersProcessed;

		// For each processed buffer, remove it from the Source Queue, read next chunk of audio
		// data from disk, fill buffer with new data, and add it to the Source Queue
		INT32 bytesWritten;
		while (buffersProcessed)
		{
			// Remove the Buffer from the Queue.  (buffer contains the Buffer ID for the unqueued Buffer)
			ALuint buffer = 0;
			alSourceUnqueueBuffers(_source, 1, &buffer);

			// Read more audio data (if there is any)
			bytesWritten = DecodeOgg(&_vorbisFile, _decodeBuffer, _bufferSize, _channels);
			if (bytesWritten)
			{
				alBufferData(buffer, _format, _decodeBuffer, bytesWritten, _frequency);
				alSourceQueueBuffers(_source, 1, &buffer);
			}

			buffersProcessed--;
		}

		// Check the status of the Source.  If it is not playing, then playback was completed,
		// or the Source was starved of audio data, and needs to be restarted.
		ALint state;
		alGetSourcei(_source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			// If there are Buffers in the Source Queue then the Source was starved of audio
			// data, so needs to be restarted (because there is more audio data to play)
			ALint queuedBuffers;
			alGetSourcei(_source, AL_BUFFERS_QUEUED, &queuedBuffers);
			if (queuedBuffers)
			{
				alSourcePlay(_source);
			}
			else {
				_state = STOP;
			}
		}
	}
}

unsigned long VorbisStream::DecodeOgg(OggVorbis_File *vorbisFile, char *decodeBuffer, unsigned long bufferSize, unsigned long channels)
{
	INT32 currentSection;
	long decodeSize;
	short *samples;

	UINT32 bytesDone = 0;
	while (true)
	{
		decodeSize = fn_ov_read(vorbisFile, decodeBuffer + bytesDone, bufferSize - bytesDone, 0, 2, 1, &currentSection);
		if (decodeSize > 0)
		{
			bytesDone += decodeSize;

			if (bytesDone >= bufferSize)
				break;
		}
		else
		{
			break;
		}
	}

	// Mono, Stereo and 4-Channel files decode into the same channel order as WAVEFORMATEXTENSIBLE,
	// however 6-Channels files need to be re-ordered
	if (channels == 6)
	{		
		samples = (short*)decodeBuffer;
		for (UINT32 i = 0; i < (bufferSize>>1); i+=6)
		{
			// WAVEFORMATEXTENSIBLE Order : FL, FR, FC, LFE, RL, RR
			// OggVorbis Order            : FL, FC, FR,  RL, RR, LFE
			Swap(samples[i+1], samples[i+2]);
			Swap(samples[i+3], samples[i+5]);
			Swap(samples[i+4], samples[i+5]);
		}
	}

	return bytesDone;
}

void VorbisStream::Swap(short &s1, short &s2)
{
	short sTemp = s1;
	s1 = s2;
	s2 = sTemp;
}

size_t VorbisStream::ov_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	IFile *file = (IFile *)datasource;
	if ((size*nmemb)>UINT32_MAX) return 0;//can't read as much as was requested
	return file->Read(ptr,static_cast<UINT32>(size*nmemb));
}

int VorbisStream::ov_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	IFile *file = (IFile *)datasource;
	
	if (offset > ULONG_MAX) return -1;
	unsigned long lOffset = (unsigned long)offset;
	switch (whence)
	{
		case SEEK_SET:
			if (offset>file->GetSize()) {
				return -1;
			}
			else {
				file->SetPosition(lOffset);
				return 0;
			}
			break;
		case SEEK_CUR:
			if ((lOffset+file->GetPosition())>file->GetSize()) {
				return -1;
			}
			else {
				file->SetPosition(file->GetPosition()+lOffset);
				return 0;
			}
			break;
		case SEEK_END:
			if ((file->GetSize()-lOffset)<0) {
				return -1;
			}
			else {
				file->SetPosition(file->GetSize()-lOffset);
				return 0;
			}
			break;
	}
	return -1;
}

int VorbisStream::ov_close_func(void *datasource)
{
	IFile *file = (IFile *)datasource;
	file->CloseFile();
	return 0;
}

long VorbisStream::ov_tell_func(void *datasource)
{
	IFile *file = (IFile *)datasource;
	INT64 pos = file->GetPosition();
	if (pos > LONG_MAX) return LONG_MAX;
	return static_cast<long>(file->GetPosition());
}


}}}}