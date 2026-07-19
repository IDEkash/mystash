// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irrlichttypes.h"
#include <IVideoDriver.h>
#include <S3DVertex.h>
#include <SMaterial.h>
#include <IEventReceiver.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Event.h>
#include <vector>
#include <string>
#include <unordered_map>

struct lua_State;

struct RmlCompiledGeometry {
	std::vector<video::S3DVertex> vertices;
	std::vector<u16> indices;
};

class RmlUiIrrlichtRenderer : public Rml::RenderInterface {
public:
	RmlUiIrrlichtRenderer(video::IVideoDriver *driver);
	virtual ~RmlUiIrrlichtRenderer();

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

private:
	video::IVideoDriver *m_driver;
	video::SMaterial m_material;
	bool m_scissor_enabled;
	Rml::Rectanglei m_scissor_region;
};

class RmlUiSystemInterface : public Rml::SystemInterface {
public:
	RmlUiSystemInterface();
	virtual ~RmlUiSystemInterface();

	double GetElapsedTime() override;
	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

private:
	u64 m_start_time;
};

class RmlUiFileInterface : public Rml::FileInterface {
public:
	RmlUiFileInterface();
	virtual ~RmlUiFileInterface();

	Rml::FileHandle Open(const Rml::String& path) override;
	void Close(Rml::FileHandle file) override;
	size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;
	bool Seek(Rml::FileHandle file, long offset, int origin) override;
	size_t Tell(Rml::FileHandle file) override;
};

class RmlUiEventListener : public Rml::EventListener {
public:
	RmlUiEventListener(const std::string &doc_id);
	virtual ~RmlUiEventListener();
	void ProcessEvent(Rml::Event &event) override;
private:
	std::string m_doc_id;
};

class RmlUiDocument {
public:
	RmlUiDocument(Rml::Context *context, const std::string &id);
	~RmlUiDocument();

	void load(const std::string &path);
	void load_string(const std::string &rml);
	void show();
	void hide();
	void close();
	void reload();

	Rml::ElementDocument *get_doc() const { return m_doc; }
	const std::string &get_id() const { return m_id; }

private:
	Rml::Context *m_context;
	std::string m_id;
	std::string m_path;
	Rml::ElementDocument *m_doc;
	RmlUiEventListener *m_listener;
};

class RmlUiManager {
public:
	static RmlUiManager *get_instance();
	static void destroy_instance();

	RmlUiManager(video::IVideoDriver *driver);
	~RmlUiManager();

	void update_and_render();
	void resize(int w, int h);

	// Event handling
	bool handle_mouse_event(const SEvent &event);
	bool handle_key_event(const SEvent &event);

	// Document management
	RmlUiDocument *create_document(const std::string &id);
	void destroy_document(const std::string &id);
	RmlUiDocument *get_document(const std::string &id);

	// Lua registration
	void register_lua_state(const std::string &doc_id, lua_State *L);
	void unregister_lua_state(const std::string &doc_id);
	void dispatch_lua_event(const std::string &doc_id, const std::string &event_type, const std::string &element_id, Rml::Event &event);

private:
	static RmlUiManager *s_instance;

	video::IVideoDriver *m_driver;
	RmlUiIrrlichtRenderer m_renderer;
	RmlUiSystemInterface m_system;
	RmlUiFileInterface m_file;
	Rml::Context *m_context;
	std::unordered_map<std::string, RmlUiDocument*> m_documents;
	std::unordered_map<std::string, lua_State*> m_lua_states;
};

std::string resolve_rmlui_path(const std::string &path);
