cmake_minimum_required(VERSION 3.12)
project(example)

add_subdirectory(glfw)

add_executable(${PROJECT_NAME}
	example.cc
	glad/src/glad.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
	glad/include
	glfw/include
	glm/
	../
)

target_link_libraries(${PROJECT_NAME} PUBLIC
	glfw
)