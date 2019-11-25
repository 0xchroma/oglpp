
#define OGLPP_USE_GLAD
#define OGLPP_USE_GLM
#define OGLPP_AUTO_BIND
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

		std::vector<uint32_t> pixels(512 * 512);

		for (auto& p : pixels)
			p = 0x00FF00FF;

		gl::Texture2D emptyTex((const char*)pixels.data(), 512, 512, GL_RGBA, GL_UNSIGNED_BYTE);

		if (!purpleShader.isValid())
			printf("Program error: %s\n", purpleShader.getLastError().c_str());

		gl::FrameBuffer fbo({
			{ 0, new gl::Texture2D(1280, 720, GL_RGB, GL_UNSIGNED_BYTE) },
			//{ 0, new gl::DepthTexture(1280, 720, GL_DEPTH_COMPONENT16) }
		});
		fbo.release();

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			{
				int w, h;
				glfwGetFramebufferSize(window, &w, &h);
				glViewport(0, 0, w, h);
			}

			fbo.bind();
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			purpleShader.bind();
			vertBuffer.bind();
			purpleShader.setSampler("inputTexture", 0, &emptyTex);

			vertBuffer.draw();
			fbo.release();

			gl::FrameBuffer::bindDefaultBuffer();

			glClearColor(1, 1, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			purpleShader.bind();
			vertBuffer.bind();

			purpleShader.setSampler("inputTexture", 0, fbo.getColourTarget(0));

			vertBuffer.draw();

			OGLPP_ERROR_CHECK();
			glfwSwapBuffers(window);
		}

		glfwDestroyWindow(window);
	}

	return 0;
}