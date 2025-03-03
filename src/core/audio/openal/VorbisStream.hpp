#pragma once

#include <AL/al.h>
#include <MGDF/MGDF.h>
#include <Vorbis/vorbisfile.h>

#include <MGDF/ComObject.hpp>

#include "OpenALSoundManagerComponentImpl.hpp"

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

constexpr uint32_t VORBIS_BUFFER_COUNT = 4;

typedef INT32 (*LPOVCLEAR)(OggVorbis_File *vf);
typedef long (*LPOVREAD)(OggVorbis_File *vf, char *buffer, INT32 length,
                         INT32 bigendianp, INT32 word, INT32 sgned,
                         INT32 *bitstream);
typedef ogg_int64_t (*LPOVPCMTOTAL)(OggVorbis_File *vf, INT32 i);
typedef vorbis_info *(*LPOVINFO)(OggVorbis_File *vf, INT32 link);
typedef vorbis_comment *(*LPOVCOMMENT)(OggVorbis_File *vf, INT32 link);
typedef INT32 (*LPOVOPENCALLBACKS)(void *datasource, OggVorbis_File *vf,
                                   char *initial, long ibytes,
                                   ov_callbacks callbacks);
enum VorbisStreamState { NOT_STARTED, PLAY, PAUSE, STOP };

class VorbisStream : public ComBase<IMGDFSoundStream> {
 public:
  virtual ~VorbisStream();
  VorbisStream(ComObject<IMGDFReadOnlyFile> source,
               OpenALSoundManagerComponentImpl *manager);
  static HRESULT TryCreate(ComObject<IMGDFReadOnlyFile> source,
                           OpenALSoundManagerComponentImpl *manager,
                           ComObject<VorbisStream> &stream);

  HRESULT __stdcall GetName(wchar_t *name, UINT64 *length) final;
  float __stdcall GetVolume() final;
  void __stdcall SetVolume(float volume) final;
  void __stdcall Stop() final;
  void __stdcall Pause() final;
  HRESULT __stdcall Play() final;
  BOOL __stdcall IsStopped() final;
  BOOL __stdcall IsPaused() final;
  BOOL __stdcall IsPlaying() final;
  UINT32 __stdcall GetPosition() final;
  UINT32 __stdcall GetLength() final;

  void Update();
  void SetGlobalVolume(float globalVolume);

 private:
  HRESULT InitStream();
  void UninitStream();

  std::wstring _name;
  ComObject<IMGDFReadOnlyFile> _dataSource;
  ComObject<IMGDFFileReader> _reader;
  ALuint _buffers[VORBIS_BUFFER_COUNT];
  ALuint _source;
  ALint _totalBuffersProcessed;
  unsigned long _frequency;
  unsigned long _format;
  unsigned long _channels;
  unsigned long _bufferSize;
  std::vector<char> _decodeBuffer;
  OggVorbis_File _vorbisFile;
  vorbis_info *_vorbisInfo;

  INT32 _initLevel;
  VorbisStreamState _state;
  float _volume;
  float _globalVolume;

  ComObject<OpenALSoundManagerComponentImpl> _soundManager;

  static INT32 _references;
  static HINSTANCE _vorbisInstance;
  static LPOVCLEAR fn_ov_clear;
  static LPOVREAD fn_ov_read;
  static LPOVPCMTOTAL fn_ov_pcm_total;
  static LPOVINFO fn_ov_info;
  static LPOVCOMMENT fn_ov_comment;
  static LPOVOPENCALLBACKS fn_ov_open_callbacks;

  static unsigned long DecodeOgg(OggVorbis_File *vorbisFile, char *decodeBuffer,
                                 unsigned long bufferSize,
                                 unsigned long channels);

  static HRESULT InitVorbis();
  static void UninitVorbis();
  static void Swap(short &s1, short &s2);

  // vorbis callbacks to read from the MGDF virtual file
  static size_t ov_read_func(void *ptr, size_t size, size_t nmemb,
                             void *datasource);
  static int ov_seek_func(void *datasource, ogg_int64_t offset, int whence);
  static int ov_close_func(void *datasource);
  static long ov_tell_func(void *datasource);
};

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF