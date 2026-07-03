// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string_view>

namespace html {

enum class NodeType {
	ELEMENT,
	TEXT,
	COMMENT
};

struct Node {
	NodeType type;
	std::string name;
	std::unordered_map<std::string, std::string> attributes;
	std::string text;
	std::vector<std::shared_ptr<Node>> children;
	std::weak_ptr<Node> parent;

	Node(NodeType t) : type(t) {}
};

class Parser {
public:
	Parser();
	void feed(std::string_view data);
	void end();

	std::shared_ptr<Node> getRoot() const { return m_root; }

private:
	void parseTag(std::string_view content);
	void handleText(std::string_view text);
	void handleComment(std::string_view text);

	std::shared_ptr<Node> m_root;
	std::shared_ptr<Node> m_current;
	std::string m_buffer;
	enum State {
		STATE_TEXT,
		STATE_TAG,
		STATE_COMMENT,
		STATE_SCRIPT,
		STATE_STYLE
	} m_state = STATE_TEXT;

	static constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024; // 1MB limit
};

std::shared_ptr<Node> parse(std::string_view html);

} // namespace html
