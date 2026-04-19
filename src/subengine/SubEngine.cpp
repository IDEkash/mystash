#include "SubEngine.h"
#include "log.h"
#include <dlfcn.h>

namespace subengine {

bool SubEngine::init(const std::string& libPath) {
    if (m_handle) {
        warningstream << "SubEngine: Library already loaded." << std::endl;
        return true;
    }

    m_handle = dlopen(libPath.c_str(), RTLD_NOW);
    if (!m_handle) {
        errorstream << "SubEngine: Failed to load library " << libPath << ": " << dlerror() << std::endl;
        return false;
    }

    m_initFn = (SubEngineInitFn)dlsym(m_handle, "subengine_init");
    m_updateFn = (SubEngineUpdateFn)dlsym(m_handle, "subengine_update");
    m_logicFn = (SubEngineLogicFn)dlsym(m_handle, "subengine_logic");
    m_renderFn = (SubEngineRenderFn)dlsym(m_handle, "subengine_render");
    m_shutdownFn = (SubEngineShutdownFn)dlsym(m_handle, "subengine_shutdown");

    if (!m_initFn) {
        errorstream << "SubEngine: Missing subengine_init in " << libPath << std::endl;
        dlclose(m_handle);
        m_handle = nullptr;
        return false;
    }

    infostream << "SubEngine: Loaded " << libPath << " successfully." << std::endl;

#ifdef __ANDROID__
    EGLDisplay display = eglGetCurrentDisplay();
    EGLContext context = eglGetCurrentContext();
    infostream << "SubEngine: Sharing EGL context " << context << " from display " << display << std::endl;
    m_initFn(display, context);
#endif

    return true;
}

void SubEngine::update() {
    if (m_updateFn) {
        m_updateFn();
    }
}

void SubEngine::logic() {
    if (m_logicFn) {
        m_logicFn();
    }
}

void SubEngine::render() {
    if (m_renderFn) {
        m_renderFn();
    }
}

void SubEngine::shutdown() {
    if (m_shutdownFn) {
        m_shutdownFn();
    }
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
    }
    m_initFn = nullptr;
    m_updateFn = nullptr;
    m_logicFn = nullptr;
    m_renderFn = nullptr;
    m_shutdownFn = nullptr;
    m_hookedProperties.clear();
    infostream << "SubEngine: Shutdown and unloaded." << std::endl;
}

void SubEngine::hookProperty(const std::string& name, void* target) {
    m_hookedProperties[name] = target;
}

void* SubEngine::getHookedProperty(const std::string& name) {
    auto it = m_hookedProperties.find(name);
    if (it != m_hookedProperties.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace subengine
