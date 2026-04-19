#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <dlfcn.h>

#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#else
typedef void* EGLDisplay;
typedef void* EGLContext;
#endif

namespace subengine {

typedef void (*SubEngineInitFn)(EGLDisplay, EGLContext);
typedef void (*SubEngineUpdateFn)();
typedef void (*SubEngineLogicFn)();
typedef void (*SubEngineRenderFn)();
typedef void (*SubEngineShutdownFn)();
typedef void* (*SubEngineGetPropFn)(const char*);

class SubEngine {
public:
    static SubEngine& getInstance() {
        static SubEngine instance;
        return instance;
    }

    bool init(const std::string& libPath);
    void update();
    void logic();
    void render();
    void shutdown();

    // Registration for hooked properties
    void hookProperty(const std::string& name, void* target);
    void* getHookedProperty(const std::string& name);

private:
    SubEngine() : m_handle(nullptr) {}
    ~SubEngine() { shutdown(); }

    void* m_handle;
    std::map<std::string, void*> m_hookedProperties;

    SubEngineInitFn m_initFn = nullptr;
    SubEngineUpdateFn m_updateFn = nullptr;
    SubEngineLogicFn m_logicFn = nullptr;
    SubEngineRenderFn m_renderFn = nullptr;
    SubEngineShutdownFn m_shutdownFn = nullptr;
    SubEngineGetPropFn m_getPropFn = nullptr;
};

} // namespace subengine
