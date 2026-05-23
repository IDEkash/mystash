file(GLOB content_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set(content_SRCS
	${content_HDRS}
	${CMAKE_CURRENT_SOURCE_DIR}/content.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/mod_configuration.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/mods.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/subgames.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/world_metadata.cpp
	PARENT_SCOPE
)
