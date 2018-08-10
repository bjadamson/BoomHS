#pragma once
#include <common/auto_resource.hpp>
#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <extlibs/glm.hpp>
#include <extlibs/openal.hpp>
#include <optional>
#include <stdio.h>
#include <string>
#include <vector>

namespace boomhs
{

struct AudioSourceState
{
  ALuint al_state;

  void allocate();
  void destroy_impl();

  NO_COPY(AudioSourceState);
  MOVE_DEFAULT(AudioSourceState);
};

struct AudioBufferState
{
  ALuint buffer;

  NO_COPY(AudioBufferState);
  MOVE_DEFAULT(AudioBufferState);

  void allocate();
  void destroy_impl();
};

using AudioBuffer         = common::AutoResource<AudioBufferState>;
using AudioSourceResource = common::AutoResource<AudioSourceState>;

class AudioSource
{
  AudioSourceResource resource_;
  NO_COPY(AudioSource);

  // This constructor is intended to only be called by the AudioFactory friend class.
  AudioSource(AudioSourceResource&&);
  friend class AudioFactory;

public:
  MOVE_DEFAULT(AudioSource);

  void associate(AudioBuffer&);

  void set_looping(bool);
  void set_volume(float);
  void set_pitch(float);

  void set_position(float, float, float);
  void set_position(glm::vec3 const&);

  void set_velocity(glm::vec3 const&);
  void set_velocity(float, float, float);

  void pause();
  void play();
  void stop();

  bool is_playing() const;
};

class AudioFactory
{
  AudioFactory() = delete;

public:
  static Result<AudioSource, std::string> create_audio_source(AudioSourceResource&&);
};

struct AudioObject
{
  AudioSource source;
  AudioBuffer buffer;

  MOVE_DEFAULT(AudioObject);
  NO_COPY(AudioObject);

  AudioObject(AudioSource&&, AudioBuffer&&);
};

struct WaterAudioSystem
{
  AudioObject water_audio;

  // ctors
  MOVE_DEFAULT(WaterAudioSystem);
  NO_COPY(WaterAudioSystem);
  WaterAudioSystem(AudioObject&&);

  // methods
  void play_inwater_sound(common::Logger&);
  void stop_inwater_sound(common::Logger&);

  bool is_playing_watersound() const;
  void set_volume(float);

  static Result<WaterAudioSystem, std::string> create();
};

} // namespace boomhs

namespace boomhs::audio
{

bool
enumerating_devices_supported();

std::vector<std::string>
get_all_devices();

ALenum
waveinfo_to_al_format(short, short);

} // namespace boomhs::audio
