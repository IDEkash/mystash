// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_rmlui.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "client/rmlui_backend.h"
#include "log.h"

int ModApiRmlUi::l_create(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	RmlUiManager::get_instance()->create_document(doc_id);
	RmlUiManager::get_instance()->register_lua_state(doc_id, L);
	return 0;
}

int ModApiRmlUi::l_destroy(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	RmlUiManager::get_instance()->destroy_document(doc_id);
	RmlUiManager::get_instance()->unregister_lua_state(doc_id);
	return 0;
}

int ModApiRmlUi::l_load(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string path = luaL_checkstring(L, 2);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc) {
		doc->load(path);
	}
	return 0;
}

int ModApiRmlUi::l_load_string(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string rml = luaL_checkstring(L, 2);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc) {
		doc->load_string(rml);
	}
	return 0;
}

int ModApiRmlUi::l_show(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc) {
		doc->show();
	}
	return 0;
}

int ModApiRmlUi::l_hide(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc) {
		doc->hide();
	}
	return 0;
}

int ModApiRmlUi::l_close(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc) {
		doc->close();
	}
	return 0;
}

int ModApiRmlUi::l_set_text(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	std::string text = luaL_checkstring(L, 3);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *el = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (el) {
			el->SetInnerRML(text.c_str());
		}
	}
	return 0;
}

int ModApiRmlUi::l_set_html(lua_State *L)
{
	return l_set_text(L);
}

int ModApiRmlUi::l_set_style(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	std::string style = luaL_checkstring(L, 3);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *el = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (el) {
			el->SetAttribute("style", style.c_str());
		}
	}
	return 0;
}

int ModApiRmlUi::l_set_attribute(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	std::string key = luaL_checkstring(L, 3);
	std::string val = luaL_checkstring(L, 4);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *el = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (el) {
			el->SetAttribute(key.c_str(), val.c_str());
		}
	}
	return 0;
}

int ModApiRmlUi::l_add_class(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	std::string class_name = luaL_checkstring(L, 3);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *el = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (el) {
			el->SetClass(class_name.c_str(), true);
		}
	}
	return 0;
}

int ModApiRmlUi::l_remove_class(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	std::string class_name = luaL_checkstring(L, 3);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *el = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (el) {
			el->SetClass(class_name.c_str(), false);
		}
	}
	return 0;
}

int ModApiRmlUi::l_find(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	std::string selector = luaL_checkstring(L, 3);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *root = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (root) {
			Rml::Element *target = root->QuerySelector(selector.c_str());
			if (target) {
				lua_pushstring(L, target->GetId().c_str());
				return 1;
			}
		}
	}
	lua_pushnil(L);
	return 1;
}

int ModApiRmlUi::l_find_all(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	std::string selector = luaL_checkstring(L, 3);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *root = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (root) {
			Rml::ElementList elements;
			root->QuerySelectorAll(elements, selector.c_str());
			lua_newtable(L);
			int index = 1;
			for (auto *el : elements) {
				lua_pushstring(L, el->GetId().c_str());
				lua_rawseti(L, -2, index++);
			}
			return 1;
		}
	}
	lua_newtable(L);
	return 1;
}

int ModApiRmlUi::l_focus(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *el = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (el) {
			el->Focus();
		}
	}
	return 0;
}

int ModApiRmlUi::l_blur(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	std::string el_id = luaL_checkstring(L, 2);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		Rml::Element *el = el_id.empty() ? doc->get_doc() : doc->get_doc()->GetElementById(el_id.c_str());
		if (el) {
			el->Blur();
		}
	}
	return 0;
}

int ModApiRmlUi::l_reload(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc) {
		doc->reload();
	}
	return 0;
}

int ModApiRmlUi::l_bring_to_front(lua_State *L)
{
	std::string doc_id = luaL_checkstring(L, 1);
	RmlUiDocument *doc = RmlUiManager::get_instance()->get_document(doc_id);
	if (doc && doc->get_doc()) {
		doc->get_doc()->PullToFront();
	}
	return 0;
}

int ModApiRmlUi::l_capture(lua_State *L)
{
	return 0;
}

int ModApiRmlUi::l_call_js(lua_State *L)
{
	return 0;
}

int ModApiRmlUi::l_set_position(lua_State *L)
{
	return 0;
}

int ModApiRmlUi::l_set_size(lua_State *L)
{
	return 0;
}

void ModApiRmlUi::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int rmlui_tbl = lua_gettop(L);

	lua_newtable(L);
	int internal_tbl = lua_gettop(L);

	registerFunction(L, "create", l_create, internal_tbl);
	registerFunction(L, "destroy", l_destroy, internal_tbl);
	registerFunction(L, "load", l_load, internal_tbl);
	registerFunction(L, "load_string", l_load_string, internal_tbl);
	registerFunction(L, "show", l_show, internal_tbl);
	registerFunction(L, "hide", l_hide, internal_tbl);
	registerFunction(L, "close", l_close, internal_tbl);
	registerFunction(L, "set_text", l_set_text, internal_tbl);
	registerFunction(L, "set_html", l_set_html, internal_tbl);
	registerFunction(L, "set_style", l_set_style, internal_tbl);
	registerFunction(L, "set_attribute", l_set_attribute, internal_tbl);
	registerFunction(L, "add_class", l_add_class, internal_tbl);
	registerFunction(L, "remove_class", l_remove_class, internal_tbl);
	registerFunction(L, "query_selector", l_find, internal_tbl);
	registerFunction(L, "query_selector_all", l_find_all, internal_tbl);
	registerFunction(L, "focus", l_focus, internal_tbl);
	registerFunction(L, "blur", l_blur, internal_tbl);
	registerFunction(L, "reload", l_reload, internal_tbl);
	registerFunction(L, "bring_to_front", l_bring_to_front, internal_tbl);

	lua_pushvalue(L, internal_tbl);
	lua_setfield(L, rmlui_tbl, "_internal");

	lua_pop(L, 1); // internal_tbl

	// Now execute our Lua-side wrapper script
	std::string lua_script = R"LUA(
		local rmlui = core.rmlui
		rmlui.documents = {}

		function rmlui.create(doc_id)
			rmlui._internal.create(doc_id)
			local doc = {
				id = doc_id,
				callbacks = {}
			}
			setmetatable(doc, rmlui.doc_meta)
			rmlui.documents[doc_id] = doc
			return doc
		end

		function rmlui.destroy(doc_id)
			rmlui._internal.destroy(doc_id)
			rmlui.documents[doc_id] = nil
		end

		function rmlui.dispatch_event(doc_id, event_type, element_id)
			local doc = rmlui.documents[doc_id]
			if doc and doc.callbacks[event_type] then
				for _, cb in ipairs(doc.callbacks[event_type]) do
					cb(element_id)
				end
			end
		end

		rmlui.doc_meta = {
			__index = {
				load = function(self, path)
					rmlui._internal.load(self.id, path)
				end,
				load_string = function(self, rml)
					rmlui._internal.load_string(self.id, rml)
				end,
				show = function(self)
					rmlui._internal.show(self.id)
				end,
				hide = function(self)
					rmlui._internal.hide(self.id)
				end,
				close = function(self)
					rmlui._internal.close(self.id)
					rmlui.documents[self.id] = nil
				end,
				set_position = function(self, x, y)
					rmlui._internal.set_style(self.id, "", "left: " .. x .. "px; top: " .. y .. "px;")
				end,
				set_size = function(self, w, h)
					rmlui._internal.set_style(self.id, "", "width: " .. w .. "px; height: " .. h .. "px;")
				end,
				set_text = function(self, el_id, text)
					rmlui._internal.set_text(self.id, el_id, text)
				end,
				set_html = function(self, el_id, html)
					rmlui._internal.set_html(self.id, el_id, html)
				end,
				set_style = function(self, el_id, style)
					rmlui._internal.set_style(self.id, el_id, style)
				end,
				set_attribute = function(self, el_id, key, val)
					rmlui._internal.set_attribute(self.id, el_id, key, tostring(val))
				end,
				add_class = function(self, el_id, class)
					rmlui._internal.add_class(self.id, el_id, class)
				end,
				remove_class = function(self, el_id, class)
					rmlui._internal.remove_class(self.id, el_id, class)
				end,
				focus = function(self, el_id)
					rmlui._internal.focus(self.id, el_id or "")
				end,
				blur = function(self, el_id)
					rmlui._internal.blur(self.id, el_id or "")
				end,
				reload = function(self)
					rmlui._internal.reload(self.id)
				end,
				bring_to_front = function(self)
					rmlui._internal.bring_to_front(self.id)
				end,
				capture = function(self)
				end,
				call_js = function(self, script)
				end,
				on = function(self, event, cb)
					self.callbacks[event] = self.callbacks[event] or {}
					table.insert(self.callbacks[event], cb)
				end,
				find = function(self, el_id)
					local res = rmlui._internal.query_selector(self.id, "", "#" .. el_id)
					if res then
						return rmlui.wrap_element(self.id, res)
					end
					return nil
				end,
				find_all = function(self, selector)
					local list = rmlui._internal.query_selector_all(self.id, "", selector)
					local ret = {}
					if list then
						for i, el_id in ipairs(list) do
							table.insert(ret, rmlui.wrap_element(self.id, el_id))
						end
					end
					return ret
				end
			}
		}

		rmlui.el_meta = {
			__index = {
				set_text = function(self, text)
					rmlui._internal.set_text(self.doc_id, self.id, text)
				end,
				set_html = function(self, html)
					rmlui._internal.set_html(self.doc_id, self.id, html)
				end,
				set_style = function(self, style)
					rmlui._internal.set_style(self.doc_id, self.id, style)
				end,
				set_attribute = function(self, key, val)
					rmlui._internal.set_attribute(self.doc_id, self.id, key, tostring(val))
				end,
				add_class = function(self, class)
					rmlui._internal.add_class(self.doc_id, self.id, class)
				end,
				remove_class = function(self, class)
					rmlui._internal.remove_class(self.doc_id, self.id, class)
				end,
				focus = function(self)
					rmlui._internal.focus(self.doc_id, self.id)
				end,
				blur = function(self)
					rmlui._internal.blur(self.doc_id, self.id)
				end,
				find = function(self, el_id)
					local res = rmlui._internal.query_selector(self.doc_id, self.id, "#" .. el_id)
					if res then
						return rmlui.wrap_element(self.doc_id, res)
					end
					return nil
				end,
				find_all = function(self, selector)
					local list = rmlui._internal.query_selector_all(self.doc_id, self.id, selector)
					local ret = {}
					if list then
						for i, el_id in ipairs(list) do
							table.insert(ret, rmlui.wrap_element(self.doc_id, el_id))
						end
					end
					return ret
				end
			}
		}

		function rmlui.wrap_element(doc_id, el_id)
			local el = {
				doc_id = doc_id,
				id = el_id
			}
			setmetatable(el, rmlui.el_meta)
			return el
		end
	)LUA";

	lua_pushvalue(L, rmlui_tbl);
	lua_setglobal(L, "rmlui_tbl_tmp");

	std::string bootstrap_script = "local core = ...\ncore.rmlui = rmlui_tbl_tmp\n" + lua_script + "\nlocal core = nil\nrmlui_tbl_tmp = nil";
	if (luaL_loadstring(L, bootstrap_script.c_str()) == 0) {
		lua_pushvalue(L, top);
		if (lua_pcall(L, 1, 0, 0) != 0) {
			errorstream << "Error bootstrapping RmlUi Lua API: " << lua_tostring(L, -1) << std::endl;
			lua_pop(L, 1);
		}
	} else {
		errorstream << "Error compiling RmlUi Lua API bootstrap: " << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
	}

	lua_pushvalue(L, rmlui_tbl);
	lua_setfield(L, top, "rmlui");

	lua_pop(L, 1); // rmlui_tbl
}
