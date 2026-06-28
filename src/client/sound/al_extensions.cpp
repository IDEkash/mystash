// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023 DS

#include "al_extensions.h"

#include "settings.h"
#include "util/string.h"
#include <unordered_set>

namespace sound {

ALExtensions::ALExtensions(const ALCdevice *deviceHandle [[maybe_unused]])
{
	auto blacklist_vec = str_split(g_settings->get("sound_extensions_blacklist"), ',');
	for (auto &s : blacklist_vec) {
		s = trim(s);
	}
	std::unordered_set<std::string> blacklist;
	blacklist.insert(blacklist_vec.begin(), blacklist_vec.end());

	{
		constexpr const char *ext_name = "AL_SOFT_direct_channels_remix";
		bool blacklisted = blacklist.find(ext_name) != blacklist.end();
		if (blacklisted)
			infostream << "ALExtensions: Blacklisted: " << ext_name << std::endl;
#ifndef AL_SOFT_direct_channels_remix
		infostream << "ALExtensions: Not compiled with: " << ext_name << std::endl;
#else
		bool found = alIsExtensionPresent(ext_name);
		if (found)
			infostream << "ALExtensions: Found: " << ext_name << std::endl;
		else
			infostream << "ALExtensions: Not found: " << ext_name << std::endl;

		if (found && !blacklisted) {
			have_ext_AL_SOFT_direct_channels_remix = true;
		}
#endif
	}

	{
		constexpr const char *ext_name = ALC_EXT_EFX_NAME;
		bool blacklisted = blacklist.find(ext_name) != blacklist.end();
		if (blacklisted)
			infostream << "ALExtensions: Blacklisted: " << ext_name << std::endl;

		bool found = alcIsExtensionPresent(const_cast<ALCdevice*>(deviceHandle), ext_name);
		if (found)
			infostream << "ALExtensions: Found: " << ext_name << std::endl;
		else
			infostream << "ALExtensions: Not found: " << ext_name << std::endl;

		if (found && !blacklisted) {
			alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
			alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
			alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
			alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
			alGenFilters = (LPALGENFILTERS)alGetProcAddress("alGenFilters");
			alDeleteFilters = (LPALDELETEFILTERS)alGetProcAddress("alDeleteFilters");
			alFilteri = (LPALFILTERI)alGetProcAddress("alFilteri");
			alFilterf = (LPALFILTERF)alGetProcAddress("alFilterf");
			alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
			alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
			alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");

			if (alGenEffects && alDeleteEffects && alEffecti && alEffectf &&
					alGenFilters && alDeleteFilters && alFilteri && alFilterf &&
					alGenAuxiliaryEffectSlots && alDeleteAuxiliaryEffectSlots && alAuxiliaryEffectSloti) {
				have_ext_ALC_EXT_EFX = true;
			} else {
				warningstream << "ALExtensions: EFX extension found but some functions are missing!" << std::endl;
			}
		}
	}
}

}
