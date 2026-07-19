// SPDX-License-Identifier: LGPL-2.1-or-later

#include "rmlui_backend.h"
#include "porting.h"
#include "filesys.h"
#include "settings.h"
#include "log.h"
#include "client/renderingengine.h"
#include <RmlUi/Core.h>
#include <IOSOperator.h>
#include <IGUIEnvironment.h>
#include <algorithm>
#include <cstdio>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

RmlUiManager *RmlUiManager::s_instance = nullptr;

RmlUiManager *RmlUiManager::get_instance()
{
	if (!s_instance) {
		IrrlichtDevice *dev = RenderingEngine::get_raw_device();
		if (dev) {
			s_instance = new RmlUiManager(dev->getVideoDriver());
		}
	}
	return s_instance;
}

void RmlUiManager::destroy_instance()
{
	delete s_instance;
	s_instance = nullptr;
}

std::string resolve_rmlui_path(const std::string &path)
{
	std::string resolved = path;
	std::replace(resolved.begin(), resolved.end(), '\\', '/');

	if (resolved.rfind("mod://", 0) == 0) {
		std::string rest = resolved.substr(6);
		auto slash_pos = rest.find('/');
		if (slash_pos != std::string::npos) {
			std::string mod_name = rest.substr(0, slash_pos);
			std::string sub_path = rest.substr(slash_pos + 1);

			std::vector<std::string> search_bases = {
				porting::path_user + DIR_DELIM "mods" + DIR_DELIM + mod_name + DIR_DELIM + sub_path,
				porting::path_share + DIR_DELIM "mods" + DIR_DELIM + mod_name + DIR_DELIM + sub_path,
			};

			std::string w1 = g_settings->get("world_path");
			std::string w2 = g_settings->get("map-dir");
			if (!w1.empty()) {
				search_bases.push_back(w1 + DIR_DELIM "worldmods" + DIR_DELIM + mod_name + DIR_DELIM + sub_path);
			}
			if (!w2.empty()) {
				search_bases.push_back(w2 + DIR_DELIM "worldmods" + DIR_DELIM + mod_name + DIR_DELIM + sub_path);
			}

			std::vector<std::string> game_paths = {
				porting::path_user + DIR_DELIM "games",
				porting::path_share + DIR_DELIM "games",
			};
			for (const auto &gp : game_paths) {
				std::vector<fs::DirListNode> list = fs::GetDirListing(gp);
				for (const auto &node : list) {
					if (node.dir) {
						search_bases.push_back(gp + DIR_DELIM + node.name + DIR_DELIM "mods" + DIR_DELIM + mod_name + DIR_DELIM + sub_path);
					}
				}
			}

			for (const auto &p : search_bases) {
				std::string clean_p = fs::RemoveRelativePathComponents(p);
				if (fs::PathExists(clean_p)) {
					return clean_p;
				}
			}
		}
	} else if (resolved.rfind("builtin://", 0) == 0) {
		std::string rest = resolved.substr(10);
		std::string p1 = porting::path_share + DIR_DELIM "builtin" + DIR_DELIM + rest;
		std::string p2 = porting::path_user + DIR_DELIM "builtin" + DIR_DELIM + rest;
		if (fs::PathExists(p1)) return p1;
		if (fs::PathExists(p2)) return p2;
	} else if (resolved.rfind("game://", 0) == 0) {
		std::string rest = resolved.substr(7);
		std::vector<std::string> search_bases = {
			porting::path_user + DIR_DELIM "games" + DIR_DELIM + rest,
			porting::path_share + DIR_DELIM "games" + DIR_DELIM + rest,
		};
		for (const auto &p : search_bases) {
			if (fs::PathExists(p)) return p;
		}
	} else if (resolved.rfind("world://", 0) == 0) {
		std::string rest = resolved.substr(8);
		std::string w1 = g_settings->get("world_path");
		std::string w2 = g_settings->get("map-dir");
		if (!w1.empty()) {
			std::string p = w1 + DIR_DELIM + rest;
			if (fs::PathExists(p)) return p;
		}
		if (!w2.empty()) {
			std::string p = w2 + DIR_DELIM + rest;
			if (fs::PathExists(p)) return p;
		}
	} else {
		if (fs::IsPathAbsolute(resolved) && fs::PathExists(resolved)) {
			return resolved;
		}
		std::string p1 = porting::path_user + DIR_DELIM + resolved;
		std::string p2 = porting::path_share + DIR_DELIM + resolved;
		if (fs::PathExists(p1)) return p1;
		if (fs::PathExists(p2)) return p2;
	}

	return fs::RemoveRelativePathComponents(resolved);
}

RmlUiIrrlichtRenderer::RmlUiIrrlichtRenderer(video::IVideoDriver *driver) :
	m_driver(driver), m_scissor_enabled(false)
{
	m_material.ZWriteEnable = video::EZW_OFF;
	m_material.ZBuffer = video::ECFN_NEVER;
	m_material.BackfaceCulling = false;
	m_material.BlendOperation = video::EBO_ADD;
	m_material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
}

RmlUiIrrlichtRenderer::~RmlUiIrrlichtRenderer()
{
}

Rml::CompiledGeometryHandle RmlUiIrrlichtRenderer::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	RmlCompiledGeometry *geom = new RmlCompiledGeometry();
	geom->vertices.reserve(vertices.size());
	for (const auto &v : vertices) {
		video::SColor color(v.colour.alpha, v.colour.red, v.colour.green, v.colour.blue);
		geom->vertices.push_back(video::S3DVertex(v.position.x, v.position.y, 0.0f, 0, 0, -1, color, v.tex_coord.x, v.tex_coord.y));
	}
	geom->indices.reserve(indices.size());
	for (int idx : indices) {
		geom->indices.push_back(static_cast<u16>(idx));
	}
	return reinterpret_cast<Rml::CompiledGeometryHandle>(geom);
}

void RmlUiIrrlichtRenderer::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	RmlCompiledGeometry *geom = reinterpret_cast<RmlCompiledGeometry*>(geometry);
	if (!geom) return;

	video::ITexture *tex = reinterpret_cast<video::ITexture*>(texture);
	m_material.setTexture(0, tex);

	core::matrix4 mat;
	mat.setTranslation(core::vector3df(translation.x, translation.y, 0.0f));
	m_driver->setTransform(video::ETS_WORLD, mat);

	m_driver->setMaterial(m_material);
	m_driver->drawIndexedTriangleList(geom->vertices.data(), geom->vertices.size(), geom->indices.data(), geom->indices.size() / 3);
}

void RmlUiIrrlichtRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	RmlCompiledGeometry *geom = reinterpret_cast<RmlCompiledGeometry*>(geometry);
	delete geom;
}

Rml::TextureHandle RmlUiIrrlichtRenderer::LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source)
{
	std::string resolved = resolve_rmlui_path(source);
	video::ITexture *tex = m_driver->getTexture(resolved.c_str());
	if (!tex) {
		return 0;
	}
	core::dimension2d<u32> size = tex->getOriginalSize();
	texture_dimensions.x = size.Width;
	texture_dimensions.y = size.Height;
	return reinterpret_cast<Rml::TextureHandle>(tex);
}

Rml::TextureHandle RmlUiIrrlichtRenderer::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	video::IImage *img = m_driver->createImage(video::ECF_A8R8G8B8, core::dimension2d<u32>(source_dimensions.x, source_dimensions.y));
	if (!img) return 0;

	u8 *dest_data = static_cast<u8*>(img->getData());
	const u8 *src_data = reinterpret_cast<const u8*>(source.data());
	int num_pixels = source_dimensions.x * source_dimensions.y;
	for (int i = 0; i < num_pixels; ++i) {
		u8 r = src_data[i * 4 + 0];
		u8 g = src_data[i * 4 + 1];
		u8 b = src_data[i * 4 + 2];
		u8 a = src_data[i * 4 + 3];
		dest_data[i * 4 + 0] = b;
		dest_data[i * 4 + 1] = g;
		dest_data[i * 4 + 2] = r;
		dest_data[i * 4 + 3] = a;
	}

	static int texture_id = 0;
	std::string tex_name = "rmlui_gen_tex_" + std::to_string(++texture_id);
	video::ITexture *tex = m_driver->addTexture(tex_name.c_str(), img);
	img->drop();

	return reinterpret_cast<Rml::TextureHandle>(tex);
}

void RmlUiIrrlichtRenderer::ReleaseTexture(Rml::TextureHandle texture)
{
	video::ITexture *tex = reinterpret_cast<video::ITexture*>(texture);
	if (tex) {
		m_driver->removeTexture(tex);
	}
}

void RmlUiIrrlichtRenderer::EnableScissorRegion(bool enable)
{
	m_scissor_enabled = enable;
	if (m_scissor_enabled) {
		core::rect<s32> vp(m_scissor_region.Left(), m_scissor_region.Top(), m_scissor_region.Right(), m_scissor_region.Bottom());
		m_driver->setViewPort(vp);
	} else {
		core::dimension2d<u32> size = m_driver->getCurrentRenderTargetSize();
		m_driver->setViewPort(core::rect<s32>(0, 0, size.Width, size.Height));
	}
}

void RmlUiIrrlichtRenderer::SetScissorRegion(Rml::Rectanglei region)
{
	m_scissor_region = region;
	if (m_scissor_enabled) {
		core::rect<s32> vp(m_scissor_region.Left(), m_scissor_region.Top(), m_scissor_region.Right(), m_scissor_region.Bottom());
		m_driver->setViewPort(vp);
	}
}

RmlUiSystemInterface::RmlUiSystemInterface()
{
	m_start_time = porting::getTimeMs();
}

RmlUiSystemInterface::~RmlUiSystemInterface()
{
}

double RmlUiSystemInterface::GetElapsedTime()
{
	return (double)(porting::getTimeMs() - m_start_time) / 1000.0;
}

bool RmlUiSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String &message)
{
	switch (type) {
		case Rml::Log::LT_ERROR:
			errorstream << "[RmlUi] " << message << std::endl;
			break;
		case Rml::Log::LT_ASSERT:
			errorstream << "[RmlUi Assert] " << message << std::endl;
			break;
		case Rml::Log::LT_WARNING:
			warningstream << "[RmlUi] " << message << std::endl;
			break;
		case Rml::Log::LT_INFO:
			infostream << "[RmlUi] " << message << std::endl;
			break;
		case Rml::Log::LT_DEBUG:
			verbosestream << "[RmlUi] " << message << std::endl;
			break;
		default:
			break;
	}
	return true;
}

void RmlUiSystemInterface::SetClipboardText(const Rml::String &text)
{
	IrrlichtDevice *dev = RenderingEngine::get_raw_device();
	if (dev) {
		gui::IGUIEnvironment *env = dev->getGUIEnvironment();
		if (env) {
			IOSOperator *op = env->getOSOperator();
			if (op) {
				op->copyToClipboard(text.c_str());
			}
		}
	}
}

void RmlUiSystemInterface::GetClipboardText(Rml::String &text)
{
	IrrlichtDevice *dev = RenderingEngine::get_raw_device();
	if (dev) {
		gui::IGUIEnvironment *env = dev->getGUIEnvironment();
		if (env) {
			IOSOperator *op = env->getOSOperator();
			if (op) {
				const c8 *clip = op->getTextFromClipboard();
				if (clip) {
					text = clip;
				}
			}
		}
	}
}

RmlUiFileInterface::RmlUiFileInterface()
{
}

RmlUiFileInterface::~RmlUiFileInterface()
{
}

Rml::FileHandle RmlUiFileInterface::Open(const Rml::String &path)
{
	std::string resolved = resolve_rmlui_path(path);
	FILE *f = std::fopen(resolved.c_str(), "rb");
	return reinterpret_cast<Rml::FileHandle>(f);
}

void RmlUiFileInterface::Close(Rml::FileHandle file)
{
	if (file) {
		std::fclose(reinterpret_cast<FILE*>(file));
	}
}

size_t RmlUiFileInterface::Read(void *buffer, size_t size, Rml::FileHandle file)
{
	if (!file) return 0;
	return std::fread(buffer, 1, size, reinterpret_cast<FILE*>(file));
}

bool RmlUiFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
	if (!file) return false;
	return std::fseek(reinterpret_cast<FILE*>(file), offset, origin) == 0;
}

size_t RmlUiFileInterface::Tell(Rml::FileHandle file)
{
	if (!file) return 0;
	return std::ftell(reinterpret_cast<FILE*>(file));
}

RmlUiEventListener::RmlUiEventListener(const std::string &doc_id) :
	m_doc_id(doc_id)
{
}

RmlUiEventListener::~RmlUiEventListener()
{
}

void RmlUiEventListener::ProcessEvent(Rml::Event &event)
{
	std::string event_type = event.GetType();
	std::string element_id = event.GetTargetElement() ? event.GetTargetElement()->GetId() : "";
	RmlUiManager::get_instance()->dispatch_lua_event(m_doc_id, event_type, element_id, event);
}

RmlUiDocument::RmlUiDocument(Rml::Context *context, const std::string &id) :
	m_context(context), m_id(id), m_doc(nullptr)
{
	m_listener = new RmlUiEventListener(id);
}

RmlUiDocument::~RmlUiDocument()
{
	close();
	delete m_listener;
}

void RmlUiDocument::load(const std::string &path)
{
	close();
	m_path = path;
	std::string resolved = resolve_rmlui_path(path);
	m_doc = m_context->LoadDocument(resolved.c_str());
	if (m_doc) {
		m_doc->AddEventListener("click", m_listener);
		m_doc->AddEventListener("mouseenter", m_listener);
		m_doc->AddEventListener("mouseleave", m_listener);
		m_doc->AddEventListener("keydown", m_listener);
		m_doc->AddEventListener("keyup", m_listener);
		m_doc->AddEventListener("submit", m_listener);
		m_doc->AddEventListener("change", m_listener);
		m_doc->AddEventListener("input", m_listener);
	}
}

void RmlUiDocument::load_string(const std::string &rml)
{
	close();
	m_path = "";
	m_doc = m_context->LoadDocumentFromMemory(rml.c_str());
	if (m_doc) {
		m_doc->AddEventListener("click", m_listener);
		m_doc->AddEventListener("mouseenter", m_listener);
		m_doc->AddEventListener("mouseleave", m_listener);
		m_doc->AddEventListener("keydown", m_listener);
		m_doc->AddEventListener("keyup", m_listener);
		m_doc->AddEventListener("submit", m_listener);
		m_doc->AddEventListener("change", m_listener);
		m_doc->AddEventListener("input", m_listener);
	}
}

void RmlUiDocument::reload()
{
	if (!m_path.empty()) {
		load(m_path);
	}
}

void RmlUiDocument::show()
{
	if (m_doc) {
		m_doc->Show();
	}
}

void RmlUiDocument::hide()
{
	if (m_doc) {
		m_doc->Hide();
	}
}

void RmlUiDocument::close()
{
	if (m_doc) {
		m_doc->Close();
		m_doc = nullptr;
	}
}

RmlUiManager::RmlUiManager(video::IVideoDriver *driver) :
	m_driver(driver), m_renderer(driver), m_context(nullptr)
{
	Rml::SetRenderInterface(&m_renderer);
	Rml::SetSystemInterface(&m_system);
	Rml::SetFileInterface(&m_file);
	Rml::Initialise();

	Rml::LoadFontFace("fonts/Arimo-Regular.ttf", true);
	Rml::LoadFontFace("fonts/Arimo-Bold.ttf", true);
	Rml::LoadFontFace("fonts/Arimo-Italic.ttf", true);
	Rml::LoadFontFace("fonts/Cousine-Regular.ttf", true);

	core::dimension2d<u32> size = m_driver->getCurrentRenderTargetSize();
	m_context = Rml::CreateContext("default", Rml::Vector2i(size.Width, size.Height));
}

RmlUiManager::~RmlUiManager()
{
	for (auto &pair : m_documents) {
		delete pair.second;
	}
	m_documents.clear();
	if (m_context) {
		Rml::RemoveContext("default");
	}
	Rml::Shutdown();
}

void RmlUiManager::update_and_render()
{
	if (m_context) {
		m_context->Update();
		m_context->Render();
	}
}

void RmlUiManager::resize(int w, int h)
{
	if (m_context) {
		m_context->SetDimensions(Rml::Vector2i(w, h));
	}
}

bool RmlUiManager::handle_mouse_event(const SEvent &event)
{
	if (!m_context) return false;

	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		int modifiers = 0;
		if (event.MouseInput.Control) modifiers |= Rml::Input::KM_CTRL;
		if (event.MouseInput.Shift) modifiers |= Rml::Input::KM_SHIFT;

		switch (event.MouseInput.Event) {
			case EMIE_MOUSE_MOVED:
				return m_context->ProcessMouseMove(event.MouseInput.X, event.MouseInput.Y, modifiers);
			case EMIE_LMOUSE_PRESSED_DOWN:
				return m_context->ProcessMouseButtonDown(0, modifiers);
			case EMIE_LMOUSE_LEFT_UP:
				return m_context->ProcessMouseButtonUp(0, modifiers);
			case EMIE_RMOUSE_PRESSED_DOWN:
				return m_context->ProcessMouseButtonDown(1, modifiers);
			case EMIE_RMOUSE_LEFT_UP:
				return m_context->ProcessMouseButtonUp(1, modifiers);
			case EMIE_MMOUSE_PRESSED_DOWN:
				return m_context->ProcessMouseButtonDown(2, modifiers);
			case EMIE_MMOUSE_LEFT_UP:
				return m_context->ProcessMouseButtonUp(2, modifiers);
			case EMIE_MOUSE_WHEEL:
				return m_context->ProcessMouseWheel(-event.MouseInput.Wheel, modifiers);
			default:
				break;
		}
	}
	return false;
}

inline Rml::Input::KeyIdentifier irr_key_to_rml(EKEY_CODE key) {
	switch (key) {
		case KEY_SPACE: return Rml::Input::KI_SPACE;
		case KEY_KEY_0: return Rml::Input::KI_0;
		case KEY_KEY_1: return Rml::Input::KI_1;
		case KEY_KEY_2: return Rml::Input::KI_2;
		case KEY_KEY_3: return Rml::Input::KI_3;
		case KEY_KEY_4: return Rml::Input::KI_4;
		case KEY_KEY_5: return Rml::Input::KI_5;
		case KEY_KEY_6: return Rml::Input::KI_6;
		case KEY_KEY_7: return Rml::Input::KI_7;
		case KEY_KEY_8: return Rml::Input::KI_8;
		case KEY_KEY_9: return Rml::Input::KI_9;
		case KEY_KEY_A: return Rml::Input::KI_A;
		case KEY_KEY_B: return Rml::Input::KI_B;
		case KEY_KEY_C: return Rml::Input::KI_C;
		case KEY_KEY_D: return Rml::Input::KI_D;
		case KEY_KEY_E: return Rml::Input::KI_E;
		case KEY_KEY_F: return Rml::Input::KI_F;
		case KEY_KEY_G: return Rml::Input::KI_G;
		case KEY_KEY_H: return Rml::Input::KI_H;
		case KEY_KEY_I: return Rml::Input::KI_I;
		case KEY_KEY_J: return Rml::Input::KI_J;
		case KEY_KEY_K: return Rml::Input::KI_K;
		case KEY_KEY_L: return Rml::Input::KI_L;
		case KEY_KEY_M: return Rml::Input::KI_M;
		case KEY_KEY_N: return Rml::Input::KI_N;
		case KEY_KEY_O: return Rml::Input::KI_O;
		case KEY_KEY_P: return Rml::Input::KI_P;
		case KEY_KEY_Q: return Rml::Input::KI_Q;
		case KEY_KEY_R: return Rml::Input::KI_R;
		case KEY_KEY_S: return Rml::Input::KI_S;
		case KEY_KEY_T: return Rml::Input::KI_T;
		case KEY_KEY_U: return Rml::Input::KI_U;
		case KEY_KEY_V: return Rml::Input::KI_V;
		case KEY_KEY_W: return Rml::Input::KI_W;
		case KEY_KEY_X: return Rml::Input::KI_X;
		case KEY_KEY_Y: return Rml::Input::KI_Y;
		case KEY_KEY_Z: return Rml::Input::KI_Z;
		case KEY_BACK: return Rml::Input::KI_BACK;
		case KEY_TAB: return Rml::Input::KI_TAB;
		case KEY_RETURN: return Rml::Input::KI_RETURN;
		case KEY_ESCAPE: return Rml::Input::KI_ESCAPE;
		case KEY_PRIOR: return Rml::Input::KI_PRIOR;
		case KEY_NEXT: return Rml::Input::KI_NEXT;
		case KEY_END: return Rml::Input::KI_END;
		case KEY_HOME: return Rml::Input::KI_HOME;
		case KEY_LEFT: return Rml::Input::KI_LEFT;
		case KEY_UP: return Rml::Input::KI_UP;
		case KEY_RIGHT: return Rml::Input::KI_RIGHT;
		case KEY_DOWN: return Rml::Input::KI_DOWN;
		case KEY_INSERT: return Rml::Input::KI_INSERT;
		case KEY_DELETE: return Rml::Input::KI_DELETE;
		case KEY_F1: return Rml::Input::KI_F1;
		case KEY_F2: return Rml::Input::KI_F2;
		case KEY_F3: return Rml::Input::KI_F3;
		case KEY_F4: return Rml::Input::KI_F4;
		case KEY_F5: return Rml::Input::KI_F5;
		case KEY_F6: return Rml::Input::KI_F6;
		case KEY_F7: return Rml::Input::KI_F7;
		case KEY_F8: return Rml::Input::KI_F8;
		case KEY_F9: return Rml::Input::KI_F9;
		case KEY_F10: return Rml::Input::KI_F10;
		case KEY_F11: return Rml::Input::KI_F11;
		case KEY_F12: return Rml::Input::KI_F12;
		case KEY_LSHIFT: return Rml::Input::KI_LSHIFT;
		case KEY_RSHIFT: return Rml::Input::KI_RSHIFT;
		case KEY_LCONTROL: return Rml::Input::KI_LCONTROL;
		case KEY_RCONTROL: return Rml::Input::KI_RCONTROL;
		default: return Rml::Input::KI_UNKNOWN;
	}
}

bool RmlUiManager::handle_key_event(const SEvent &event)
{
	if (!m_context) return false;

	if (event.EventType == EET_KEY_INPUT_EVENT) {
		int modifiers = 0;
		if (event.KeyInput.Control) modifiers |= Rml::Input::KM_CTRL;
		if (event.KeyInput.Shift) modifiers |= Rml::Input::KM_SHIFT;

		Rml::Input::KeyIdentifier key = irr_key_to_rml(event.KeyInput.Key);

		if (event.KeyInput.PressedDown) {
			bool processed = m_context->ProcessKeyDown(key, modifiers);
			if (event.KeyInput.Char != 0) {
				processed |= m_context->ProcessTextInput(event.KeyInput.Char);
			}
			return processed;
		} else {
			return m_context->ProcessKeyUp(key, modifiers);
		}
	}
	return false;
}

RmlUiDocument *RmlUiManager::create_document(const std::string &id)
{
	destroy_document(id);
	RmlUiDocument *doc = new RmlUiDocument(m_context, id);
	m_documents[id] = doc;
	return doc;
}

void RmlUiManager::destroy_document(const std::string &id)
{
	auto it = m_documents.find(id);
	if (it != m_documents.end()) {
		delete it->second;
		m_documents.erase(it);
	}
}

RmlUiDocument *RmlUiManager::get_document(const std::string &id)
{
	auto it = m_documents.find(id);
	if (it != m_documents.end()) {
		return it->second;
	}
	return nullptr;
}

void RmlUiManager::register_lua_state(const std::string &doc_id, lua_State *L)
{
	m_lua_states[doc_id] = L;
}

void RmlUiManager::unregister_lua_state(const std::string &doc_id)
{
	m_lua_states.erase(doc_id);
}

void RmlUiManager::dispatch_lua_event(const std::string &doc_id, const std::string &event_type, const std::string &element_id, Rml::Event &event)
{
	auto it = m_lua_states.find(doc_id);
	if (it != m_lua_states.end()) {
		lua_State *L = it->second;
		lua_getglobal(L, "core");
		if (lua_istable(L, -1)) {
			lua_getfield(L, -1, "rmlui");
			if (lua_istable(L, -1)) {
				lua_getfield(L, -1, "dispatch_event");
				if (lua_isfunction(L, -1)) {
					lua_pushstring(L, doc_id.c_str());
					lua_pushstring(L, event_type.c_str());
					lua_pushstring(L, element_id.c_str());
					if (lua_pcall(L, 3, 0, 0) != 0) {
						errorstream << "Error dispatching RmlUi Lua event: " << lua_tostring(L, -1) << std::endl;
						lua_pop(L, 1);
					}
				} else {
					lua_pop(L, 1);
				}
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
}
