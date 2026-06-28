// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023 DS

#pragma once

#include "al_helpers.h"

#ifndef ALC_EXT_EFX_NAME
#define ALC_EXT_EFX_NAME                         "ALC_EXT_EFX"
#endif
#ifndef ALC_EFX_MAJOR_VERSION
#define ALC_EFX_MAJOR_VERSION                    0x20001
#endif
#ifndef ALC_EFX_MINOR_VERSION
#define ALC_EFX_MINOR_VERSION                    0x20002
#endif
#ifndef ALC_MAX_AUXILIARY_SENDS
#define ALC_MAX_AUXILIARY_SENDS                  0x20003
#endif
#ifndef AL_METERS_PER_UNIT
#define AL_METERS_PER_UNIT                       0x20004
#endif
#ifndef AL_DIRECT_FILTER
#define AL_DIRECT_FILTER                         0x20005
#endif
#ifndef AL_AUXILIARY_SEND_FILTER
#define AL_AUXILIARY_SEND_FILTER                 0x20006
#endif
#ifndef AL_AIR_ABSORPTION_FACTOR
#define AL_AIR_ABSORPTION_FACTOR                 0x20007
#endif
#ifndef AL_ROOM_ROLLOFF_FACTOR
#define AL_ROOM_ROLLOFF_FACTOR                   0x20008
#endif
#ifndef AL_CONE_OUTER_GAINHF
#define AL_CONE_OUTER_GAINHF                     0x20009
#endif
#ifndef AL_REVERB_DENSITY
#define AL_REVERB_DENSITY                        0x0001
#endif
#ifndef AL_REVERB_DIFFUSION
#define AL_REVERB_DIFFUSION                      0x0002
#endif
#ifndef AL_REVERB_GAIN
#define AL_REVERB_GAIN                           0x0003
#endif
#ifndef AL_REVERB_GAINHF
#define AL_REVERB_GAINHF                         0x0004
#endif
#ifndef AL_REVERB_DECAY_TIME
#define AL_REVERB_DECAY_TIME                     0x0005
#endif
#ifndef AL_REVERB_DECAY_HFRATIO
#define AL_REVERB_DECAY_HFRATIO                  0x0006
#endif
#ifndef AL_REVERB_REFLECTIONS_GAIN
#define AL_REVERB_REFLECTIONS_GAIN               0x0007
#endif
#ifndef AL_REVERB_REFLECTIONS_DELAY
#define AL_REVERB_REFLECTIONS_DELAY              0x0008
#endif
#ifndef AL_REVERB_LATE_REVERB_GAIN
#define AL_REVERB_LATE_REVERB_GAIN               0x0009
#endif
#ifndef AL_REVERB_LATE_REVERB_DELAY
#define AL_REVERB_LATE_REVERB_DELAY              0x000A
#endif
#ifndef AL_REVERB_AIR_ABSORPTION_GAINHF
#define AL_REVERB_AIR_ABSORPTION_GAINHF          0x000B
#endif
#ifndef AL_REVERB_ROOM_ROLLOFF_FACTOR
#define AL_REVERB_ROOM_ROLLOFF_FACTOR            0x000C
#endif
#ifndef AL_REVERB_DECAY_HFLIMIT
#define AL_REVERB_DECAY_HFLIMIT                  0x000D
#endif
#ifndef AL_ECHO_DELAY
#define AL_ECHO_DELAY                            0x0001
#endif
#ifndef AL_ECHO_LRDELAY
#define AL_ECHO_LRDELAY                          0x0002
#endif
#ifndef AL_ECHO_DAMPING
#define AL_ECHO_DAMPING                          0x0003
#endif
#ifndef AL_ECHO_FEEDBACK
#define AL_ECHO_FEEDBACK                         0x0004
#endif
#ifndef AL_ECHO_SPREAD
#define AL_ECHO_SPREAD                           0x0005
#endif
#ifndef AL_LOWPASS_GAIN
#define AL_LOWPASS_GAIN                          0x0001
#endif
#ifndef AL_LOWPASS_GAINHF
#define AL_LOWPASS_GAINHF                        0x0002
#endif
#ifndef AL_EFFECT_TYPE
#define AL_EFFECT_TYPE                           0x8001
#endif
#ifndef AL_EFFECT_NULL
#define AL_EFFECT_NULL                           0x0000
#endif
#ifndef AL_EFFECT_REVERB
#define AL_EFFECT_REVERB                         0x0001
#endif
#ifndef AL_EFFECT_ECHO
#define AL_EFFECT_ECHO                           0x0004
#endif
#ifndef AL_FILTER_TYPE
#define AL_FILTER_TYPE                           0x8001
#endif
#ifndef AL_FILTER_NULL
#define AL_FILTER_NULL                           0x0000
#endif
#ifndef AL_FILTER_LOWPASS
#define AL_FILTER_LOWPASS                        0x0001
#endif
#ifndef AL_AUXILIARY_EFFECT_SLOT_EFFECT
#define AL_AUXILIARY_EFFECT_SLOT_EFFECT          0x0001
#endif
#ifndef AL_AUXILIARY_EFFECT_SLOT_GAIN
#define AL_AUXILIARY_EFFECT_SLOT_GAIN            0x0002
#endif
#ifndef AL_AUXILIARY_EFFECT_SLOT_NULL
#define AL_AUXILIARY_EFFECT_SLOT_NULL            0x0000
#endif

namespace sound {

typedef void (AL_APIENTRY *LPALGENEFFECTS)(ALsizei n, ALuint *effects);
typedef void (AL_APIENTRY *LPALDELETEEFFECTS)(ALsizei n, const ALuint *effects);
typedef void (AL_APIENTRY *LPALEFFECTI)(ALuint effect, ALenum param, ALint iValue);
typedef void (AL_APIENTRY *LPALEFFECTF)(ALuint effect, ALenum param, ALfloat flValue);
typedef void (AL_APIENTRY *LPALGENFILTERS)(ALsizei n, ALuint *filters);
typedef void (AL_APIENTRY *LPALDELETEFILTERS)(ALsizei n, const ALuint *filters);
typedef void (AL_APIENTRY *LPALFILTERI)(ALuint filter, ALenum param, ALint iValue);
typedef void (AL_APIENTRY *LPALFILTERF)(ALuint filter, ALenum param, ALfloat flValue);
typedef void (AL_APIENTRY *LPALGENAUXILIARYEFFECTSLOTS)(ALsizei n, ALuint *slots);
typedef void (AL_APIENTRY *LPALDELETEAUXILIARYEFFECTSLOTS)(ALsizei n, const ALuint *slots);
typedef void (AL_APIENTRY *LPALAUXILIARYEFFECTSLOTI)(ALuint slot, ALenum param, ALint iValue);

/**
 * Struct for AL and ALC extensions
 */
struct ALExtensions
{
	explicit ALExtensions(const ALCdevice *deviceHandle [[maybe_unused]]);

#ifdef AL_SOFT_direct_channels_remix
	bool have_ext_AL_SOFT_direct_channels_remix = false;
#endif

	bool have_ext_ALC_EXT_EFX = false;
	LPALGENEFFECTS alGenEffects = nullptr;
	LPALDELETEEFFECTS alDeleteEffects = nullptr;
	LPALEFFECTI alEffecti = nullptr;
	LPALEFFECTF alEffectf = nullptr;
	LPALGENFILTERS alGenFilters = nullptr;
	LPALDELETEFILTERS alDeleteFilters = nullptr;
	LPALFILTERI alFilteri = nullptr;
	LPALFILTERF alFilterf = nullptr;
	LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
	LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
	LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;
};

}
