// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 DS
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2011 Sebastian 'Bahamada' Rühl
// Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
// Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

#include "playing_sound.h"

#include "al_extensions.h"
#include "sound_constants.h"
#include "efx_presets.h"
#include <cassert>
#include <cmath>

namespace sound {

PlayingSound::PlayingSound(ALuint source_id, std::shared_ptr<ISoundDataOpen> data,
		bool loop, f32 volume, f32 pitch, f32 start_time,
		const std::optional<std::pair<v3f, v3f>> &pos_vel_opt,
		const ALExtensions &exts)
	: m_source_id(source_id), m_data(std::move(data)), m_looping(loop),
	m_is_positional(pos_vel_opt.has_value()), m_exts(exts)
{
	// Calculate actual start_time (see lua_api.txt for specs)
	f32 len_seconds = m_data->m_decode_info.length_seconds;
	f32 len_samples = m_data->m_decode_info.length_samples;
	if (!m_looping) {
		if (start_time < 0.0f) {
			start_time = std::fmax(start_time + len_seconds, 0.0f);
		} else if (start_time >= len_seconds) {
			// No sound
			m_next_sample_pos = len_samples;
			return;
		}
	} else {
		// Modulo offset to be within looping time
		start_time = start_time - std::floor(start_time / len_seconds) * len_seconds;
	}

	// Queue first buffers

	m_next_sample_pos = std::min((start_time / len_seconds) * len_samples, len_samples);

	if (m_looping && m_next_sample_pos == len_samples)
		m_next_sample_pos = 0;

	if (!m_data->isStreaming()) {
		// If m_next_sample_pos >= len_samples, buf will be 0, and setting it as
		// AL_BUFFER is a NOP (source stays AL_UNDETERMINED). => No sound will be
		// played.

		auto [buf, buf_end, offset_in_buf] = m_data->getOrLoadBufferAt(m_next_sample_pos);
		m_next_sample_pos = buf_end;

		alSourcei(m_source_id, AL_BUFFER, buf);
		alSourcei(m_source_id, AL_SAMPLE_OFFSET, offset_in_buf);

		alSourcei(m_source_id, AL_LOOPING, m_looping ? AL_TRUE : AL_FALSE);

		warn_if_al_error("when creating non-streaming sound");

	} else {
		// Start with first buffer

		// If m_next_sample_pos >= len_samples (happens only if not looped), buf0
		// will be 0. Queuing 0 is a NOP.

		auto [buf0, buf0_end, offset_in_buf0] = m_data->getOrLoadBufferAt(m_next_sample_pos);
		m_next_sample_pos = buf0_end;

		alSourceQueueBuffers(m_source_id, 1, &buf0);
		alSourcei(m_source_id, AL_SAMPLE_OFFSET, offset_in_buf0);

		// We can't use AL_LOOPING because more buffers are queued later.
		// Looping is therefore done manually.

		// Sound is not dead if queue runs empty prematurely
		m_stopped_means_dead = false;

		warn_if_al_error("when creating streaming sound");

		// Enqueue more buffers
		stepStream(true);
	}

	// Set initial pos, volume, pitch
	if (m_is_positional) {
		updatePosVel(pos_vel_opt->first, pos_vel_opt->second);
	} else {
		// Make position-less
		alSourcei(m_source_id, AL_SOURCE_RELATIVE, true);
		alSource3f(m_source_id, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(m_source_id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		warn_if_al_error("PlayingSound::PlayingSound at making position-less");

#ifdef AL_SOFT_direct_channels_remix
		// Play directly on stereo output channels if possible. Improves sound quality.
		if (exts.have_ext_AL_SOFT_direct_channels_remix
				&& m_data->m_decode_info.is_stereo) {
			alSourcei(m_source_id, AL_DIRECT_CHANNELS_SOFT, AL_REMIX_UNMATCHED_SOFT);
			warn_if_al_error("PlayingSound::PlayingSound at setting AL_DIRECT_CHANNELS_SOFT");
		}
#endif
	}
	setGain(volume);
	setPitch(pitch);
}

PlayingSound::~PlayingSound() noexcept
{
	alSourceStop(m_source_id);
	alDeleteSources(1, &m_source_id);
	if (m_filter) m_exts.alDeleteFilters(1, &m_filter);
	if (m_effect) m_exts.alDeleteEffects(1, &m_effect);
	if (m_slot) m_exts.alDeleteAuxiliaryEffectSlots(1, &m_slot);
}

bool PlayingSound::stepStream(bool playback_speed_changed)
{
	if (isDead())
		return false;

	// Unqueue finished buffers
	ALint num_processed_bufs = 0;
	alGetSourcei(m_source_id, AL_BUFFERS_PROCESSED, &num_processed_bufs);
	if (num_processed_bufs == 0 && !playback_speed_changed)
		return true; // Nothing to do
	if (num_processed_bufs > 0) {
		ALint num_to_unqueue = num_processed_bufs;
		ALuint unqueued_buffer_ids[8];
		while (num_to_unqueue > 8) {
			alSourceUnqueueBuffers(m_source_id, 8, unqueued_buffer_ids);
			num_to_unqueue -= 8;
		}
		alSourceUnqueueBuffers(m_source_id, num_to_unqueue, unqueued_buffer_ids);
	}

	// Find out how many buffers we want to enqueue
	f32 pitch = 1.0f;
	alGetSourcef(m_source_id, AL_PITCH, &pitch);
	ALint num_queued_bufs = 0;
	alGetSourcei(m_source_id, AL_BUFFERS_QUEUED, &num_queued_bufs);
	// Min. length of untouched buffers
	const f32 playback_left = MIN_STREAM_BUFFER_LENGTH * std::max(0, num_queued_bufs - 1);
	// Max. time until next stepStream() call, see also [Streaming of sounds] in
	// sound_constants.h.
	// Multiplied by pitch because pitch makes playback faster than real time.
	// (Does not account for doppler effect, if we had that.)
	// +0.1 seconds to accommodate hickups.
	const f32 playback_until_next_check = (2.0f * STREAM_BIGSTEP_TIME + 0.1f) * pitch;
	const f32 playback_to_fill_up = std::max(0.0f, playback_until_next_check - playback_left);
	const int num_bufs_to_enqueue = std::ceil(playback_to_fill_up / MIN_STREAM_BUFFER_LENGTH);

	// Fill up
	for (int i = 0; i < num_bufs_to_enqueue; ++i) {
		if (m_next_sample_pos == m_data->m_decode_info.length_samples) {
			// Reached end
			if (m_looping) {
				m_next_sample_pos = 0;
			} else {
				m_stopped_means_dead = true;
				return false;
			}
		}

		auto [buf, buf_end, offset_in_buf] = m_data->getOrLoadBufferAt(m_next_sample_pos);
		m_next_sample_pos = buf_end;
		assert(offset_in_buf == 0);

		alSourceQueueBuffers(m_source_id, 1, &buf);

		// Start again if queue was empty and resulted in stop
		if (getState() == AL_STOPPED) {
			play();
			warningstream << "PlayingSound::stepStream: Sound queue ran empty for \""
					<< m_data->m_decode_info.name_for_logging << "\"" << std::endl;
		}
	}

	return true;
}

bool PlayingSound::fade(f32 step, f32 target_gain) noexcept
{
	bool already_fading = m_fade_state.has_value();

	target_gain = MYMAX(target_gain, 0.0f); // 0.0f if nan
	step = target_gain - getGain() > 0.0f ? std::abs(step) : -std::abs(step);

	m_fade_state = FadeState{step, target_gain};

	return !already_fading;
}

bool PlayingSound::doFade(f32 dtime) noexcept
{
	if (!m_fade_state || isDead())
		return false;

	if (getState() == AL_PAUSED)
		return true;

	FadeState &fade = *m_fade_state;
	assert(fade.step != 0.0f);

	f32 current_gain = getGain();
	current_gain += fade.step * dtime;

	if (fade.step < 0.0f)
		current_gain = std::max(current_gain, fade.target_gain);
	else
		current_gain = std::min(current_gain, fade.target_gain);

	if (current_gain <= 0.0f) {
		// stop sound
		m_stopped_means_dead = true;
		alSourceStop(m_source_id);

		m_fade_state = std::nullopt;
		return false;
	}

	setGain(current_gain);

	if (current_gain == fade.target_gain) {
		m_fade_state = std::nullopt;
		return false;
	} else {
		return true;
	}
}

void PlayingSound::updatePosVel(const v3f &pos, const v3f &vel) noexcept
{
	alSourcei(m_source_id, AL_SOURCE_RELATIVE, false);
	alSource3f(m_source_id, AL_POSITION, pos.X, pos.Y, pos.Z);
	alSource3f(m_source_id, AL_VELOCITY, vel.X, vel.Y, vel.Z);
	// Using alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED) and setting reference
	// distance to clamp gain at <1 node distance avoids excessive volume when
	// closer.
	alSourcef(m_source_id, AL_REFERENCE_DISTANCE, 1.0f);

	warn_if_al_error("PlayingSound::updatePosVel");
}

void PlayingSound::setGain(f32 gain) noexcept
{
	// AL_REFERENCE_DISTANCE was once reduced from 3 nodes to 1 node.
	// We compensate this by multiplying the volume by 3.
	if (m_is_positional)
		gain *= 3.0f;

	alSourcef(m_source_id, AL_GAIN, gain);
}

f32 PlayingSound::getGain() noexcept
{
	ALfloat gain;
	alGetSourcef(m_source_id, AL_GAIN, &gain);
	// Same as above, but inverse.
	if (m_is_positional)
		gain *= 1.0f/3.0f;
	return gain;
}

void PlayingSound::setPitch(f32 pitch)
{
	alSourcef(m_source_id, AL_PITCH, pitch);
	if (isStreaming())
		stepStream(true);
}

void PlayingSound::setLowpass(f32 gain)
{
	if (!m_exts.have_ext_ALC_EXT_EFX) return;

	if (gain >= 1.0f) {
		alSourcei(m_source_id, AL_DIRECT_FILTER, AL_FILTER_NULL);
		return;
	}

	if (!m_filter) m_exts.alGenFilters(1, &m_filter);
	m_exts.alFilteri(m_filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
	m_exts.alFilterf(m_filter, AL_LOWPASS_GAIN, 1.0f);
	m_exts.alFilterf(m_filter, AL_LOWPASS_GAINHF, gain);
	alSourcei(m_source_id, AL_DIRECT_FILTER, m_filter);
}

void PlayingSound::setReverb(const std::string &preset_name)
{
	if (!m_exts.have_ext_ALC_EXT_EFX) return;

	const ReverbPreset *p = nullptr;
	for (const auto &preset : reverb_presets) {
		if (preset_name == preset.name) {
			p = &preset;
			break;
		}
	}

	if (!p || preset_name == "none") {
		alSource3i(m_source_id, AL_AUXILIARY_SEND_FILTER, AL_AUXILIARY_EFFECT_SLOT_NULL, 0, AL_FILTER_NULL);
		return;
	}

	if (!m_effect) m_exts.alGenEffects(1, &m_effect);
	if (!m_slot) m_exts.alGenAuxiliaryEffectSlots(1, &m_slot);

	m_exts.alEffecti(m_effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
	m_exts.alEffectf(m_effect, AL_REVERB_DENSITY, p->density);
	m_exts.alEffectf(m_effect, AL_REVERB_DIFFUSION, p->diffusion);
	m_exts.alEffectf(m_effect, AL_REVERB_GAIN, p->gain);
	m_exts.alEffectf(m_effect, AL_REVERB_GAINHF, p->gainhf);
	m_exts.alEffectf(m_effect, AL_REVERB_DECAY_TIME, p->decay_time);
	m_exts.alEffectf(m_effect, AL_REVERB_DECAY_HFRATIO, p->decay_hfratio);
	m_exts.alEffectf(m_effect, AL_REVERB_REFLECTIONS_GAIN, p->reflections_gain);
	m_exts.alEffectf(m_effect, AL_REVERB_REFLECTIONS_DELAY, p->reflections_delay);
	m_exts.alEffectf(m_effect, AL_REVERB_LATE_REVERB_GAIN, p->late_reverb_gain);
	m_exts.alEffectf(m_effect, AL_REVERB_LATE_REVERB_DELAY, p->late_reverb_delay);
	m_exts.alEffectf(m_effect, AL_REVERB_AIR_ABSORPTION_GAINHF, p->air_absorption_gainhf);
	m_exts.alEffectf(m_effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, p->room_rolloff_factor);
	m_exts.alEffecti(m_effect, AL_REVERB_DECAY_HFLIMIT, p->decay_hflimit);

	m_exts.alAuxiliaryEffectSloti(m_slot, AL_AUXILIARY_EFFECT_SLOT_EFFECT, m_effect);
	alSource3i(m_source_id, AL_AUXILIARY_SEND_FILTER, m_slot, 0, AL_FILTER_NULL);
}

void PlayingSound::setEcho(f32 delay, f32 decay)
{
	if (!m_exts.have_ext_ALC_EXT_EFX) return;

	if (delay <= 0.0f) {
		alSource3i(m_source_id, AL_AUXILIARY_SEND_FILTER, AL_AUXILIARY_EFFECT_SLOT_NULL, 0, AL_FILTER_NULL);
		return;
	}

	if (!m_effect) m_exts.alGenEffects(1, &m_effect);
	if (!m_slot) m_exts.alGenAuxiliaryEffectSlots(1, &m_slot);

	m_exts.alEffecti(m_effect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);
	m_exts.alEffectf(m_effect, AL_ECHO_DELAY, delay);
	m_exts.alEffectf(m_effect, AL_ECHO_LRDELAY, delay);
	m_exts.alEffectf(m_effect, AL_ECHO_FEEDBACK, decay);

	m_exts.alAuxiliaryEffectSloti(m_slot, AL_AUXILIARY_EFFECT_SLOT_EFFECT, m_effect);
	alSource3i(m_source_id, AL_AUXILIARY_SEND_FILTER, m_slot, 0, AL_FILTER_NULL);
}

} // namespace sound
