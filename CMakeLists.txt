cmake_minimum_required(VERSION 3.14)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(DOOM_OpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- GLFW ---
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# --- GLM ---
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# --- Source and Headers ---
include_directories(include)
file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.c)

# --- Executable ---
find_package(OpenGL REQUIRED)
add_executable(${PROJECT_NAME} ${SRC_FILES})

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME} glfw glm::glm OpenGL::GL)

# Note: We still need Glad files in src/ and include/
