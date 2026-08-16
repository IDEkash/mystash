// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>

namespace unapi {

class ExtensionStorage {
private:
	std::string m_extension_id;
	std::string m_base_path;

	std::string resolvePath(const std::string &relative_path) const;
	bool isPathSafe(const std::string &full_path) const;

public:
	ExtensionStorage(const std::string &extension_id, const std::string &storage_root);

	bool exists(const std::string &path) const;
	bool createDirectory(const std::string &path) const;
	std::string readFile(const std::string &path) const;
	bool writeFile(const std::string &path, const std::string &content) const;
	bool appendFile(const std::string &path, const std::string &content) const;
	bool deleteFile(const std::string &path) const;
	bool renameFile(const std::string &old_path, const std::string &new_path) const;
	std::vector<std::string> listDirectory(const std::string &path) const;

	const std::string &getExtensionId() const { return m_extension_id; }
	const std::string &getBasePath() const { return m_base_path; }
};

} // namespace unapi
