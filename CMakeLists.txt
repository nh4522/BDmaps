cmake_minimum_required(VERSION 3.18)
project(BDmaps CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Download and setup ImGui
include(FetchContent)

# Clean up any previous fetch
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)
endif()

# Create a static library for ImGui
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(imgui PUBLIC 
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    IMGUI_DEFINE_MATH_OPERATORS
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/Camera.cpp
    src/MapData.cpp
    src/Shader.cpp
    src/Renderer.cpp
    src/InputHandler.cpp
    src/UIOverlay.cpp
)

add_executable(BDmaps ${SOURCES})

target_include_directories(BDmaps PRIVATE 
    include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(BDmaps PRIVATE
    glfw
    glm::glm
    glad::glad
    nlohmann_json::nlohmann_json
    OpenGL::GL
    imgui  # Link the imgui library
)

# Add compile definitions for the main executable
target_compile_definitions(BDmaps PRIVATE 
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    _CRT_SECURE_NO_WARNINGS
)

add_custom_command(TARGET BDmaps POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:BDmaps>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:BDmaps>/data
    COMMENT "Copying shaders and data..."
)