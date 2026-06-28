// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023 DS

#pragma once

#include "al_helpers.h"

#ifndef ALC_EXT_EFX
#define ALC_EXT_EFX_NAME                         "ALC_EXT_EFX"
#define ALC_EFX_MAJOR_VERSION                    0x20001
#define ALC_EFX_MINOR_VERSION                    0x20002
#define ALC_MAX_AUXILIARY_SENDS                  0x20003
#define AL_METERS_PER_UNIT                       0x20004
#define AL_DIRECT_FILTER                         0x20005
#define AL_AUXILIARY_SEND_FILTER                 0x20006
#define AL_AIR_ABSORPTION_FACTOR                 0x20007
#define AL_ROOM_ROLLOFF_FACTOR                   0x20008
#define AL_CONE_OUTER_GAINHF                     0x20009
#define AL_REVERB_DENSITY                        0x0001
#define AL_REVERB_DIFFUSION                      0x0002
#define AL_REVERB_GAIN                           0x0003
#define AL_REVERB_GAINHF                         0x0004
#define AL_REVERB_DECAY_TIME                     0x0005
#define AL_REVERB_DECAY_HFRATIO                  0x0006
#define AL_REVERB_REFLECTIONS_GAIN               0x0007
#define AL_REVERB_REFLECTIONS_DELAY              0x0008
#define AL_REVERB_LATE_REVERB_GAIN               0x0009
#define AL_REVERB_LATE_REVERB_DELAY              0x000A
#define AL_REVERB_AIR_ABSORPTION_GAINHF          0x000B
#define AL_REVERB_ROOM_ROLLOFF_FACTOR            0x000C
#define AL_REVERB_DECAY_HFLIMIT                  0x000D
#define AL_ECHO_DELAY                            0x0001
#define AL_ECHO_LRDELAY                          0x0002
#define AL_ECHO_DAMPING                          0x0003
#define AL_ECHO_FEEDBACK                         0x0004
#define AL_ECHO_SPREAD                           0x0005
#define AL_LOWPASS_GAIN                          0x0001
#define AL_LOWPASS_GAINHF                        0x0002
#define AL_EFFECT_TYPE                           0x8001
#define AL_EFFECT_NULL                           0x0000
#define AL_EFFECT_REVERB                         0x0001
#define AL_EFFECT_ECHO                           0x0004
#define AL_FILTER_TYPE                           0x8001
#define AL_FILTER_NULL                           0x0000
#define AL_FILTER_LOWPASS                        0x0001
#define AL_AUXILIARY_EFFECT_SLOT_EFFECT          0x0001
#define AL_AUXILIARY_EFFECT_SLOT_GAIN            0x0002
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
