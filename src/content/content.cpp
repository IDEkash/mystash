// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2018 rubenwardy <rw@rubenwardy.com>

#include "content/content.h"
#include "filesys.h"
#include "settings.h"

ContentType getContentType(const std::string &path)
{
	if (fs::IsFile(path + DIR_DELIM "modpack.txt") || fs::IsFile(path + DIR_DELIM "modpack.conf"))
		return ContentType::MODPACK;

	if (fs::IsFile(path + DIR_DELIM "init.lua") || fs::IsFile(path + DIR_DELIM "external.conf") ||
			fs::PathExists(path + DIR_DELIM "ServerSideService") || fs::PathExists(path + DIR_DELIM "ClientSideService") ||
			fs::PathExists(path + DIR_DELIM "Workspace"))
		return ContentType::MOD;

	if (fs::IsFile(path + DIR_DELIM "game.conf"))
		return ContentType::GAME;

	if (fs::IsFile(path + DIR_DELIM "texture_pack.conf"))
		return ContentType::TXP;

	return ContentType::UNKNOWN;
}

void parseContentInfo(ContentSpec &spec)
{
	std::string conf_path;

	switch (getContentType(spec.path)) {
	case ContentType::MOD:
		spec.type = "mod";
		if (fs::IsFile(spec.path + DIR_DELIM "external.conf")) {
			conf_path = spec.path + DIR_DELIM "external.conf";
		} else {
			conf_path = spec.path + DIR_DELIM "mod.conf";
		}
		break;
	case ContentType::MODPACK:
		spec.type = "modpack";
		conf_path = spec.path + DIR_DELIM "modpack.conf";
		break;
	case ContentType::GAME:
		spec.type = "game";
		conf_path = spec.path + DIR_DELIM "game.conf";
		break;
	case ContentType::TXP:
		spec.type = "txp";
		conf_path = spec.path + DIR_DELIM "texture_pack.conf";
		break;
	default:
		spec.type = "unknown";
		break;
	}

	Settings conf;
	if (!conf_path.empty() && conf.readConfigFile(conf_path.c_str())) {
		if (conf.exists("title"))
			spec.title = conf.get("title");
		else if (conf.exists("Name"))
			spec.title = conf.get("Name");
		else if (conf.exists("name"))
			spec.title = conf.get("name");
		else if (spec.type == "game" && conf.exists("name"))
			spec.title = conf.get("name");

		if (spec.type != "game") {
			if (conf.exists("ID"))
				spec.name = conf.get("ID");
			else if (conf.exists("id"))
				spec.name = conf.get("id");
			else if (conf.exists("name"))
				spec.name = conf.get("name");
		}

		if (spec.type == "game") {
			if (spec.title.empty())
				spec.title = spec.name;
			spec.name = "";
		}

		if (conf.exists("description"))
			spec.desc = conf.get("description");

		if (conf.exists("author"))
			spec.author = conf.get("author");

		if (conf.exists("release"))
			spec.release = conf.getS32("release");

		if (conf.exists("textdomain"))
			spec.textdomain = conf.get("textdomain");
	}

	if (spec.name.empty())
		spec.name = fs::GetFilenameFromPath(spec.path.c_str());

	if (spec.textdomain.empty())
		spec.textdomain = spec.name;

	if (spec.desc.empty()) {
		fs::ReadFile(spec.path + DIR_DELIM + "description.txt", spec.desc);
	}
}
