// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "unapi_storage.h"
#include "filesys.h"
#include "log.h"
#include "porting.h"
#include <fstream>

namespace unapi {

ExtensionStorage::ExtensionStorage(const std::string &extension_id, const std::string &storage_root)
	: m_extension_id(extension_id) {
	std::string root = storage_root;
	if (root.empty()) {
		root = porting::path_user + DIR_DELIM + "extension_data";
	}
	m_base_path = fs::AbsolutePathPartial(root + DIR_DELIM + extension_id);
	fs::CreateAllDirs(m_base_path);
}

std::string ExtensionStorage::resolvePath(const std::string &relative_path) const {
	std::string full = fs::AbsolutePathPartial(m_base_path + DIR_DELIM + relative_path);
	return full;
}

bool ExtensionStorage::isPathSafe(const std::string &full_path) const {
	if (full_path.empty()) return false;
	return fs::PathStartsWith(full_path, m_base_path);
}

bool ExtensionStorage::exists(const std::string &path) const {
	std::string full = resolvePath(path);
	if (!isPathSafe(full)) return false;
	return fs::PathExists(full);
}

bool ExtensionStorage::createDirectory(const std::string &path) const {
	std::string full = resolvePath(path);
	if (!isPathSafe(full)) return false;
	return fs::CreateAllDirs(full);
}

std::string ExtensionStorage::readFile(const std::string &path) const {
	std::string full = resolvePath(path);
	if (!isPathSafe(full)) return "";
	std::string content;
	fs::ReadFile(full, content, true);
	return content;
}

bool ExtensionStorage::writeFile(const std::string &path, const std::string &content) const {
	std::string full = resolvePath(path);
	if (!isPathSafe(full)) return false;
	std::string dir = fs::RemoveLastPathComponent(full);
	fs::CreateAllDirs(dir);
	return fs::safeWriteToFile(full, content);
}

bool ExtensionStorage::appendFile(const std::string &path, const std::string &content) const {
	std::string full = resolvePath(path);
	if (!isPathSafe(full)) return false;
	std::string dir = fs::RemoveLastPathComponent(full);
	fs::CreateAllDirs(dir);
	std::ofstream ofs = open_ofstream(full.c_str(), true, std::ios::app);
	if (!ofs.good()) return false;
	ofs << content;
	return true;
}

bool ExtensionStorage::deleteFile(const std::string &path) const {
	std::string full = resolvePath(path);
	if (!isPathSafe(full)) return false;
	return fs::DeleteSingleFileOrEmptyDirectory(full, true) || fs::RecursiveDelete(full);
}

bool ExtensionStorage::renameFile(const std::string &old_path, const std::string &new_path) const {
	std::string full_old = resolvePath(old_path);
	std::string full_new = resolvePath(new_path);
	if (!isPathSafe(full_old) || !isPathSafe(full_new)) return false;
	return fs::Rename(full_old, full_new);
}

std::vector<std::string> ExtensionStorage::listDirectory(const std::string &path) const {
	std::vector<std::string> result;
	std::string full = resolvePath(path);
	if (!isPathSafe(full)) return result;
	auto listing = fs::GetDirListing(full);
	for (const auto &item : listing) {
		result.push_back(item.name);
	}
	return result;
}

} // namespace unapi
