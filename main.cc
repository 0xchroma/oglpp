
#define OGLPP_USE_GLAD
#define OGLPP_USE_GLM
#include "gl++.h"

#include <cstdio>
#include <GLFW/glfw3.h>

struct vec2
{
	float x, y;
};

struct vec3 
{
	float x, y, z;
};

struct Vertex
{
	vec3 position{0, 0, 0};
	vec3 normals{ 0, 0, 0 };
	vec2 texcoord{ 0, 0 };
};

const Vertex verts[] = {
	{ { -1, -1, 0 } },
	{ { 1, -1, 0 } },
	{ { 1, 1, 0 } },
};

const char* testSource = R"(

#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

void main()
{
}

#elif defined(FRAGMENT_SHADER)

out vec3 outColour;

void main()
{
	outColour = vec3(1, 0, 1);
}

#endif
)";

int main(int, const char**)
{
	glfwInit();

	if (auto* window = glfwCreateWindow(1280, 720, "test", nullptr, nullptr))
	{
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		gladLoadGL();

		gl::VertexArray vao(verts, 3, {
			{ 0, GL_FLOAT, 3, offsetof(Vertex, position) },
			{ 1, GL_FLOAT, 3, offsetof(Vertex, normals) },
			{ 2, GL_FLOAT, 2, offsetof(Vertex, texcoord) },
		});

		gl::Program prog({
			{ GL_VERTEX_SHADER, testSource, { "#version 330", "#define VERTEX_SHADER" } },
			{ GL_FRAGMENT_SHADER, testSource, { "#version 330", "#define FRAGMENT_SHADER" } },
		});

		if (!prog.isValid())
		{
			printf("Program error: %s\n", prog.getLastError().c_str());
		}

		while (!glfwWindowShouldClose(window))
		{
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			
			vao.bind();
			prog.bind();

			vao.draw();

			glfwPollEvents();
			glfwSwapBuffers(window);
		}

		glfwDestroyWindow(window);
	}

	return 0;
}