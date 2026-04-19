add_library(subengine STATIC
    SubEngine.cpp
    LuaSubEngine.cpp
)

target_include_directories(subengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(subengine PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Create a separate shared library for the Bullet Bridge
add_library(bullet_bridge SHARED bridges/BulletBridge.cpp)
target_include_directories(bullet_bridge PRIVATE
    ${bullet_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(bullet_bridge
    BulletDynamics BulletCollision LinearMath
    log
)

if(ANDROID)
    target_link_libraries(bullet_bridge GLESv3)
endif()
