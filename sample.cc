
#define OGLPP_USE_GLAD
#define OGLPP_AUTO_BIND
#include "ogl.hpp"

#include <cstdio>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec2 texcoord;
};

const char* testSource = R"(

#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

out vec2 uv;

uniform mat4 mvp;

void main()
{
	gl_Position = vec4(position, 1) * mvp;
	uv = texcoord;
}

#elif defined(FRAGMENT_SHADER)

in vec2 uv;

out vec3 outColour;

void main()
{
	outColour = vec3(1, 0, 1);
}

#endif
)";

namespace gl
{
	void setProgramUniform(GLuint index, const glm::vec2& v) 
	{
		glUniform2fv(index, 2, glm::value_ptr(v)); 
	}
	
	void setProgramUniform(GLuint index, const glm::vec3& v)
	{ 
		glUniform3fv(index, 3, glm::value_ptr(v)); 
	}
	
	void setProgramUniform(GLuint index, const glm::vec4& v)
	{ 
		glUniform4fv(index, 4, glm::value_ptr(v)); 
	}
	
	void setProgramUniform(GLuint index, const glm::mat4& m)
	{ 
		glUniformMatrix4fv(index, 1, GL_FALSE, glm::value_ptr(m)); 
	}
}

int main(int, const char**)
{
	glfwInit();

	if (auto* window = glfwCreateWindow(1280, 720, "test", nullptr, nullptr))
	{
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		gladLoadGL();

		glEnable(GL_DEPTH_TEST);

		const Vertex verts[] = {
			{ { -1,  1, 0  },	{ 0, 1 } },		// 0 top left
			{ {  1,  1, 0  },	{ 1, 1 } },		// 1 top right
			{ {  1, -1, 0  },	{ 1, 0 } },		// 2 bottom right
			{ { -1, -1, 0  },	{ 0, 0 } }		// 3 bottom left
		};

		const uint32_t indices[] = {
			3, 0, 1,
			3, 2, 1
		};

		gl::VertexArray vertBuffer(
			verts, 4,
			indices, 6,
			{
				{ 0, GL_FLOAT,	3, offsetof(Vertex, position) },
				{ 1, GL_INT,	2, offsetof(Vertex, texcoord) }
			}
		);

		gl::Program purpleShader({
			{ GL_VERTEX_SHADER,		testSource, { "#version 330", "#define VERTEX_SHADER" } },
			{ GL_FRAGMENT_SHADER,	testSource, { "#version 330", "#define FRAGMENT_SHADER" } },
		});

		if (!purpleShader.isValid())
			printf("Program error: %s\n", purpleShader.getLastError().c_str());

		glfwSetTime(0);

		while (!glfwWindowShouldClose(window))
		{	
			int w, h;

			glfwPollEvents();

			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

			glfwGetFramebufferSize(window, &w, &h);
			glViewport(0, 0, w, h);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 projection = glm::perspective(glm::radians(45.f), h / (float)w, 0.1f, 100.0f);
			glm::mat4 view = glm::lookAt(glm::vec3(0, 0, -10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
			glm::mat4 model{ 1 };
			
			vertBuffer.bind();
			purpleShader.bind();

			purpleShader.setUniform("mvp", projection * view * model);
			vertBuffer.draw();

			OGLPP_ERROR_CHECK();
			glfwSwapBuffers(window);
		}

		glfwDestroyWindow(window);
	}

	return 0;
}