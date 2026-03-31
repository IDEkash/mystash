# Run the myengine registry generator
find_package(Python3 REQUIRED)
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/myengine_registry.generated.h"
           "${CMAKE_CURRENT_SOURCE_DIR}/myengine_registry.generated.cpp"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/../util/gen_myengine_registry.py"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../util/gen_myengine_registry.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating myengine registry"
    VERBATIM
)
add_custom_target(GenerateMyEngineRegistry DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/myengine_registry.generated.h")

# Ensure GenerateMyEngineRegistry runs before EngineCommon
# add_dependencies(EngineCommon GenerateMyEngineRegistry) # Move this after EngineCommon is defined

# Add the generated source to common_SRCS
list(APPEND common_SRCS
    myengine_registry.generated.cpp
)
