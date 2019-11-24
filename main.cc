
#define OGLPP_USE_GLAD
#define OGLPP_USE_GLM
#include "ogl.hpp"

#include <cstdio>
#include <GLFW/glfw3.h>
#include <vector>

struct Vertex
{
	glm::vec3 position;
	//glm::vec3 normal;
	glm::vec2 texcoord;
};

const char* testSource = R"(

#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 position;
//layout(location = 1) in vec3 normal;
layout(location = 1) in vec2 texcoord;

out vec2 uv;

void main()
{
	gl_Position = vec4(position, 1);
	uv = texcoord;
}

#elif defined(FRAGMENT_SHADER)

in vec2 uv;
uniform sampler2D inputTexture;

out vec3 outColour;

void main()
{
	outColour = texture(inputTexture, uv).xyz;
	//outColour = vec3(1, 0, 1);
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

		const Vertex verts[] = {
			{ { -1, -1, 0 },	{ 0, 0 } },
			{ { 1, -1, 0 },		{ 1, 0 } },
			{ { 1, 1, 0 },		{ 1, 1 } }
		};

		gl::VertexArray vertBuffer(verts, 3, {
			{ 0, GL_FLOAT, 3, offsetof(Vertex, position) },
			//{ 1, GL_FLOAT, 3, offsetof(Vertex, normal) },
			{ 2, GL_INT, 2, offsetof(Vertex, texcoord) },
		});

		gl::Program purpleShader({
			{ GL_VERTEX_SHADER, testSource, { "#version 330", "#define VERTEX_SHADER" } },
			{ GL_FRAGMENT_SHADER, testSource, { "#version 330", "#define FRAGMENT_SHADER" } },
		});

		std::vector<char> testPixels(512 * 512 * 3, 0x255);

		gl::Texture2D emptyTex(testPixels.data(), 512, 512, GL_RGB, GL_UNSIGNED_BYTE);

		if (!purpleShader.isValid())
			printf("Program error: %s\n", purpleShader.getLastError().c_str());

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			{
				int w, h;
				glfwGetFramebufferSize(window, &w, &h);
				glViewport(0, 0, w, h);
			}
			
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			vertBuffer.bind();

			purpleShader.bind();
			purpleShader.setSampler("inputTexture", 0, &emptyTex);
			
			vertBuffer.draw();

			OGL_ERROR_CHECK();
			glfwSwapBuffers(window);
		}

		glfwDestroyWindow(window);
	}

	return 0;
}