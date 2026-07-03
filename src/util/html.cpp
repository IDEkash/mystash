// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "util/html.h"
#include "util/string.h"
#include <stack>
#include <unordered_set>
#include <algorithm>

namespace html {

static std::string decode_entities(std::string_view text)
{
	std::string res;
	res.reserve(text.size());
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == '&') {
			if (text.substr(i, 4) == "&lt;") {
				res += '<'; i += 3;
			} else if (text.substr(i, 4) == "&gt;") {
				res += '>'; i += 3;
			} else if (text.substr(i, 5) == "&amp;") {
				res += '&'; i += 4;
			} else if (text.substr(i, 6) == "&quot;") {
				res += '"'; i += 5;
			} else if (text.substr(i, 6) == "&apos;") {
				res += '\''; i += 5;
			} else {
				res += '&';
			}
		} else {
			res += text[i];
		}
	}
	return res;
}

Parser::Parser()
{
	m_root = std::make_shared<Node>(NodeType::ELEMENT);
	m_root->name = "root";
	m_current = m_root;
}

void Parser::feed(std::string_view data)
{
	if (m_buffer.size() + data.size() > MAX_BUFFER_SIZE) {
		// Drop data if buffer is too large
		return;
	}
	m_buffer.append(data);

	size_t pos = 0;
	while (pos < m_buffer.size()) {
		if (m_state == STATE_TEXT) {
			size_t next_tag = m_buffer.find('<', pos);
			if (next_tag == std::string::npos) {
				break;
			}

			// Check if we have enough data to determine if it's a comment or tag
			if (next_tag + 3 >= m_buffer.size() && m_buffer.substr(next_tag, 2) == "<!") {
				break;
			}

			// Handle text before tag
			if (next_tag > pos) {
				handleText(std::string_view(m_buffer).substr(pos, next_tag - pos));
			}

			pos = next_tag + 1;
			if (pos + 2 < m_buffer.size() && m_buffer[pos] == '!' &&
					m_buffer[pos + 1] == '-' && m_buffer[pos + 2] == '-') {
				m_state = STATE_COMMENT;
				pos += 3;
			} else {
				m_state = STATE_TAG;
			}
		} else if (m_state == STATE_TAG) {
			size_t end_tag = m_buffer.find('>', pos);
			if (end_tag == std::string::npos) {
				break;
			}

			parseTag(std::string_view(m_buffer).substr(pos, end_tag - pos));
			pos = end_tag + 1;

			std::string lower_name = lowercase(m_current->name);
			if (lower_name == "script") {
				m_state = STATE_SCRIPT;
			} else if (lower_name == "style") {
				m_state = STATE_STYLE;
			} else {
				m_state = STATE_TEXT;
			}
		} else if (m_state == STATE_COMMENT) {
			size_t end_comment = m_buffer.find("-->", pos);
			if (end_comment == std::string::npos) {
				break;
			}

			handleComment(std::string_view(m_buffer).substr(pos, end_comment - pos));
			pos = end_comment + 3;
			m_state = STATE_TEXT;
		} else if (m_state == STATE_SCRIPT || m_state == STATE_STYLE) {
			std::string end_tag = (m_state == STATE_SCRIPT) ? "</script>" : "</style>";
			size_t end_pos = 0;
			// Case insensitive search for end tag
			auto it = std::search(m_buffer.begin() + pos, m_buffer.end(),
				end_tag.begin(), end_tag.end(),
				[](char a, char b) { return my_tolower(a) == my_tolower(b); });

			if (it == m_buffer.end()) {
				break;
			}
			end_pos = std::distance(m_buffer.begin(), it);

			handleText(std::string_view(m_buffer).substr(pos, end_pos - pos));
			pos = end_pos + end_tag.size();
			m_state = STATE_TEXT;

			auto p = m_current->parent.lock();
			if (p) m_current = p;
		}
	}

	if (pos > 0) {
		m_buffer.erase(0, pos);
	}
}

void Parser::end()
{
	if (!m_buffer.empty()) {
		if (m_state == STATE_TEXT || m_state == STATE_SCRIPT || m_state == STATE_STYLE) {
			handleText(m_buffer);
		}
		m_buffer.clear();
	}
}

void Parser::handleText(std::string_view text)
{
	if (text.empty()) return;
	auto node = std::make_shared<Node>(NodeType::TEXT);
	node->text = decode_entities(text);
	node->parent = m_current;
	m_current->children.push_back(node);
}

void Parser::handleComment(std::string_view text)
{
	auto node = std::make_shared<Node>(NodeType::COMMENT);
	node->text = std::string(text);
	node->parent = m_current;
	m_current->children.push_back(node);
}

void Parser::parseTag(std::string_view content)
{
	if (content.empty()) return;

	if (content[0] == '/') {
		// Closing tag
		std::string name(content.substr(1));
		name = trim(name);
		std::string lower_name = lowercase(name);

		auto p = m_current->parent.lock();
		if (p && lowercase(m_current->name) == lower_name) {
			m_current = p;
		}
		return;
	}

	bool self_closing = false;
	if (content.back() == '/') {
		self_closing = true;
		content.remove_suffix(1);
	}

	std::string_view name_view;
	size_t space = content.find_first_of(" \t\r\n");
	if (space == std::string_view::npos) {
		name_view = content;
	} else {
		name_view = content.substr(0, space);
	}

	auto node = std::make_shared<Node>(NodeType::ELEMENT);
	node->name = std::string(name_view);
	node->parent = m_current;

	if (space != std::string_view::npos) {
		std::string_view attrs = content.substr(space + 1);
		// Very simple attribute parser
		while (!attrs.empty()) {
			attrs = trim(attrs);
			if (attrs.empty()) break;
			size_t eq = attrs.find('=');
			if (eq == std::string_view::npos) {
				node->attributes[std::string(attrs)] = "";
				break;
			}
			std::string attr_name(trim(attrs.substr(0, eq)));
			attrs.remove_prefix(eq + 1);
			attrs = trim(attrs);
			if (attrs.empty()) break;
			if (attrs[0] == '"' || attrs[0] == '\'') {
				char quote = attrs[0];
				attrs.remove_prefix(1);
				size_t end_quote = attrs.find(quote);
				if (end_quote == std::string_view::npos) {
					node->attributes[attr_name] = decode_entities(attrs);
					break;
				}
				node->attributes[attr_name] = decode_entities(attrs.substr(0, end_quote));
				attrs.remove_prefix(end_quote + 1);
			} else {
				size_t next_space = attrs.find_first_of(" \t\r\n");
				if (next_space == std::string_view::npos) {
					node->attributes[attr_name] = decode_entities(attrs);
					break;
				}
				node->attributes[attr_name] = decode_entities(attrs.substr(0, next_space));
				attrs.remove_prefix(next_space + 1);
			}
		}
	}

	m_current->children.push_back(node);
	if (!self_closing) {
		// Some tags are always self-closing in HTML even if not marked so
		static const std::unordered_set<std::string> void_elements = {
			"area", "base", "br", "col", "embed", "hr", "img", "input",
			"link", "meta", "param", "source", "track", "wbr"
		};
		std::string lower_name = lowercase(node->name);
		if (void_elements.find(lower_name) == void_elements.end()) {
			m_current = node;
		}
	}
}

std::shared_ptr<Node> parse(std::string_view html)
{
	Parser p;
	p.feed(html);
	p.end();
	return p.getRoot();
}

} // namespace html
