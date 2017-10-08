get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${SELF_DIR}/../geCore/geCoreConfig.cmake)

include(CMakeFindDependencyMacro)
find_dependency(SDL2)
include("${CMAKE_CURRENT_LIST_DIR}/SDLWindowTargets.cmake")
