#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../ogl.hpp"

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

namespace gl
{
	void setProgramUniform(GLuint index, const glm::vec2& v) { glUniform2fv(index, 1, glm::value_ptr(v)); }
	void setProgramUniform(GLuint index, const glm::vec3& v) { glUniform3fv(index, 1, glm::value_ptr(v)); }
	void setProgramUniform(GLuint index, const glm::vec4& v) { glUniform4fv(index, 1, glm::value_ptr(v)); }
	void setProgramUniform(GLuint index, const glm::mat4& m) { glUniformMatrix4fv(index, 1, GL_FALSE, glm::value_ptr(m)); }
}

static const glm::vec3 triangle[3]{
	{ -1, -1, 0 },
	{ 0, 1, 0 },
	{ 1, -1, 0 }
};

static const char* shaderCode = R"(

#ifdef VERT_SHADER

layout (location = 0) in vec3 pos;

uniform mat4 mvp;

void main()
{
	gl_Position = mvp * vec4(pos.xyz, 1);
}

#endif

#ifdef FRAG_SHADER

uniform vec3 diffuse;

out vec3 outColour;

void main()
{
	outColour = diffuse;
}

#endif

)";

static gl::Program* purpleShader{};
static gl::VertexArray* triangleBuffer{};

static void init()
{
	purpleShader = new gl::Program(
		{
			{ GL_VERTEX_SHADER, shaderCode,		{ "#version 330", "#define VERT_SHADER" } },
			{ GL_FRAGMENT_SHADER, shaderCode,	{ "#version 330", "#define FRAG_SHADER" } }
		}
	);

	triangleBuffer = new gl::VertexArray(triangle, 3,
		{
			{ 0, GL_FLOAT, 3, 0 }
		}
	);
	
	if (!purpleShader->isValid())
	{
		printf("shader error: %s\n", purpleShader->getLastError().c_str());
		getchar();
	}
}

static void render(double time, int w, int h)
{
	auto perspective = glm::perspectiveFov<float>(glm::degrees<float>(45.f), w, h, 0.1, 100);
	auto view = glm::lookAt<float>(glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	auto model = glm::identity<glm::mat4>();

	glClearColor(0.1, 0.1, 0.1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	triangleBuffer->bind();
	purpleShader->bind();

	purpleShader->setUniform<glm::vec3>("diffuse", { 0.3, 0.3, 0.3 });
	purpleShader->setUniform<glm::mat4>("mvp", perspective * view * model);

	triangleBuffer->draw();
}

int main(int argc, const char** argv)
{
	glfwInit();

	auto* window = glfwCreateWindow(512, 512, "", nullptr, nullptr);

	glfwMakeContextCurrent(window);
	gladLoadGL();

	init();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		render(glfwGetTime(), w, h);

		glfwSwapBuffers(window);

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			break;
	}

	glfwTerminate();

	return 0;
}