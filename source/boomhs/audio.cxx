#include <boomhs/audio.hpp>

namespace boomhs
{

ALsizei constexpr NUM_AUDIO_BUFFERS = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioSourceState
void
AudioSourceState::allocate()
{
  alGenSources(NUM_AUDIO_BUFFERS, &al_state);
}

void
AudioSourceState::destroy_impl()
{
  alDeleteSources(NUM_AUDIO_BUFFERS, &al_state);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioBufferState
void
AudioBufferState::allocate()
{
  alGenBuffers(NUM_AUDIO_BUFFERS, &buffer);
}

void
AudioBufferState::destroy_impl()
{
  alDeleteBuffers(NUM_AUDIO_BUFFERS, &buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioSource
AudioSource::AudioSource(AudioSourceResource&& r)
    : resource_(MOVE(r))
{
  resource_->allocate();
}

void
AudioSource::associate(AudioBuffer& buffer)
{
  alSourcei(resource_->al_state, AL_BUFFER, buffer->buffer);
}

void
AudioSource::set_looping(bool const loop)
{
  alSourcei(resource_->al_state, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
}

void
AudioSource::set_volume(float const v)
{
  alSourcef(resource_->al_state, AL_GAIN, v);
}

void
AudioSource::set_pitch(float const p)
{
  alSourcef(resource_->al_state, AL_PITCH, p);
}

void
AudioSource::set_position(float const x, float const y, float const z)
{
  alSource3f(resource_->al_state, AL_POSITION, x, y, z);
}

void
AudioSource::set_position(glm::vec3 const& pos)
{
  set_position(pos.x, pos.y, pos.z);
}

void
AudioSource::set_velocity(float const x, float const y, float const z)
{
  alSource3f(resource_->al_state, AL_VELOCITY, x, y, z);
}

void
AudioSource::set_velocity(glm::vec3 const& vel)
{
  set_velocity(vel.x, vel.y, vel.z);
}

void
AudioSource::pause()
{
  alSourcePause(resource_->al_state);
}

void
AudioSource::play()
{
  alSourcePlay(resource_->al_state);
}

void
AudioSource::stop()
{
  alSourceStop(resource_->al_state);
}

bool
AudioSource::is_playing() const
{
  ALint value;
  alGetSourcei(resource_->al_state, AL_SOURCE_STATE, &value);
  return value == AL_PLAYING;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioObject
AudioObject::AudioObject(AudioSource&& s, AudioBuffer&& b)
    : source(MOVE(s))
    , buffer(MOVE(b))
{
  // Associate the source and buffers
  //
  // This should occur AFTER they have been moved into their final spot, associate takes a
  // referernce to the buffer (and it shoulnd't be moved, otherwise the reference will be
  // invalidated).
  source.associate(buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioFactory
Result<AudioSource, std::string>
AudioFactory::create_audio_source(AudioSourceResource&& r)
{
  r->allocate();
  AudioSource source{MOVE(r)};

  return Ok(MOVE(source));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// WaterAudioSystem
WaterAudioSystem::WaterAudioSystem(AudioObject&& o)
    : water_audio(MOVE(o))
{
}

Result<WaterAudioSystem, std::string>
WaterAudioSystem::create()
{
  AudioSourceResource r{AudioSourceState{}};
  auto                source = TRY_MOVEOUT(AudioFactory::create_audio_source(MOVE(r)));

  source.set_pitch(1.0f);
  source.set_volume(1.0f);
  source.set_position(0, 0, 0);
  source.set_velocity(2, 0, 0);
  source.set_looping(false);

  AudioBuffer buffer{AudioBufferState{}};
  buffer->allocate();
  // check for errors

  WaveInfo* wave = WaveOpenFileForReading("assets/audio/water stream.wav");
  if (!wave) {
    return ErrCString("failed to read wave file\n");
  }
  ON_SCOPE_EXIT([&wave]() { WaveCloseFile(wave); });
  // check for errors

  int ret = WaveSeekFile(0, wave);
  if (ret) {
    return ErrCString("failed to seek wave file\n");
  }
  // check for errors

  std::vector<char> wave_buffer;
  wave_buffer.reserve(wave->dataSize);
  char* buffer_data = wave_buffer.data();

  ret = WaveReadFile(buffer_data, wave->dataSize, wave);
  if (ret != static_cast<int>(wave->dataSize)) {
    return Err(fmt::sprintf("short read: %d, want: %d\n", ret, wave->dataSize));
  }
  alBufferData(buffer->buffer, audio::waveinfo_to_al_format(wave->channels, wave->bitsPerSample),
               buffer_data, wave->dataSize, wave->sampleRate);
  // check for errors

  AudioObject      audio_object{MOVE(source), MOVE(buffer)};
  WaterAudioSystem was{MOVE(audio_object)};
  return Ok(MOVE(was));
}

bool
WaterAudioSystem::is_playing_watersound() const
{
  return water_audio.source.is_playing();
}

void
WaterAudioSystem::play_inwater_sound(stlw::Logger& logger)
{
  auto& source = water_audio.source;
  if (!source.is_playing()) {
    source.play();
    // check for errors

    LOG_ERROR("RESTARTING SOUND");
  }
}

void
WaterAudioSystem::stop_inwater_sound(stlw::Logger&)
{
  auto& source = water_audio.source;
  source.stop();
}

void
WaterAudioSystem::set_volume(float const v)
{
  auto& source = water_audio.source;
  source.set_volume(v);
}

} // namespace boomhs

namespace boomhs::audio
{

bool
enumerating_devices_supported()
{
  ALboolean const enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
  return enumeration == AL_TRUE;
}

std::vector<std::string>
get_all_devices()
{
  auto* devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);

  ALCchar const *device = devices, *next = devices + 1;

  std::vector<std::string> device_names;
  while (device && *device != '\0' && next && *next != '\0') {
    device_names.emplace_back(std::string{device});
    size_t const len = ::strlen(device);
    device += (len + 1);
    next += (len + 2);
  }
  return device_names;
}

ALenum
waveinfo_to_al_format(short const channels, short const samples)
{
  bool const stereo = (channels > 1);

  switch (samples) {
  case 16:
    if (stereo) {
      return AL_FORMAT_STEREO16;
    }
    else {
      return AL_FORMAT_MONO16;
    }
  case 8:
    if (stereo) {
      return AL_FORMAT_STEREO8;
    }
    else {
      return AL_FORMAT_MONO8;
    }
  default:
    return -1;
  }
}

} // namespace boomhs::audio
