// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_html.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"

static constexpr u16 HTML_MAX_PUSH_DEPTH = 64;

static void push_html_node(lua_State *L, const std::shared_ptr<html::Node> &node, u16 depth)
{
	if (depth >= HTML_MAX_PUSH_DEPTH) {
		lua_pushnil(L);
		return;
	}
	lua_newtable(L);

	std::string type_str;
	switch (node->type) {
	case html::NodeType::ELEMENT: type_str = "element"; break;
	case html::NodeType::TEXT:    type_str = "text"; break;
	case html::NodeType::COMMENT: type_str = "comment"; break;
	}
	lua_pushstring(L, type_str.c_str());
	lua_setfield(L, -2, "type");

	if (node->type == html::NodeType::ELEMENT) {
		lua_pushstring(L, node->name.c_str());
		lua_setfield(L, -2, "name");

		if (!node->attributes.empty()) {
			lua_newtable(L);
			for (const auto &attr : node->attributes) {
				lua_pushstring(L, attr.second.c_str());
				lua_setfield(L, -2, attr.first.c_str());
			}
			lua_setfield(L, -2, "attributes");
		}

		if (!node->children.empty()) {
			lua_newtable(L);
			for (size_t i = 0; i < node->children.size(); i++) {
				push_html_node(L, node->children[i], depth + 1);
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "children");
		}
	} else {
		lua_pushstring(L, node->text.c_str());
		lua_setfield(L, -2, "text");
	}
}

int ModApiHTML::l_parse(lua_State *L)
{
	std::string_view text = readParam<std::string_view>(L, 1);
	auto root = html::parse(text);
	push_html_node(L, root, 0);
	return 1;
}

int ModApiHTML::l_stream(lua_State *L)
{
	return LuaHTMLStream::create_object(L);
}

void ModApiHTML::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int tbl = lua_gettop(L);

	registerFunction(L, "parse", l_parse, tbl);
	registerFunction(L, "stream", l_stream, tbl);

	lua_setfield(L, top, "html");
}

// LuaHTMLStream

const char LuaHTMLStream::className[] = "HTMLStream";
const luaL_Reg LuaHTMLStream::methods[] = {
	luamethod(LuaHTMLStream, feed),
	luamethod(LuaHTMLStream, end),
	luamethod(LuaHTMLStream, get_root),
	{0, 0}
};

LuaHTMLStream::LuaHTMLStream()
{
	m_parser = std::make_unique<html::Parser>();
}

int LuaHTMLStream::gc_object(lua_State *L)
{
	LuaHTMLStream *o = *(LuaHTMLStream **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

int LuaHTMLStream::create_object(lua_State *L)
{
	LuaHTMLStream *o = new LuaHTMLStream();
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

void LuaHTMLStream::Register(lua_State *L)
{
	static const luaL_Reg metamethods[] = {
		{"__gc", gc_object},
		{0, 0}
	};
	registerClass<LuaHTMLStream>(L, methods, metamethods);
}

int LuaHTMLStream::l_feed(lua_State *L)
{
	LuaHTMLStream *o = checkObject<LuaHTMLStream>(L, 1);
	std::string_view text = readParam<std::string_view>(L, 2);
	o->m_parser->feed(text);
	return 0;
}

int LuaHTMLStream::l_end(lua_State *L)
{
	LuaHTMLStream *o = checkObject<LuaHTMLStream>(L, 1);
	o->m_parser->end();
	return 0;
}

int LuaHTMLStream::l_get_root(lua_State *L)
{
	LuaHTMLStream *o = checkObject<LuaHTMLStream>(L, 1);
	push_html_node(L, o->m_parser->getRoot(), 0);
	return 1;
}
