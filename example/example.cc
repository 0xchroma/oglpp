

#define OGLPP_AUTO_BIND
#include <glad/glad.h>
#include "ogl.hpp"

#include <cstdio>
#include <memory>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec2 texcoord;
};

const char* textureShaderSource = R"(

#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

out vec2 uv;

uniform mat4 mvp;

void main()
{
	gl_Position = mvp * vec4(position, 1);
	uv = texcoord;
}

#elif defined(FRAGMENT_SHADER)

in vec2 uv;

uniform float textureEnabled;
uniform sampler2D inputTex;

out vec3 outColour;

void main()
{
	outColour = mix(vec3(1, 0, 1), texture(inputTex, uv).rgb, textureEnabled);
}

#endif
)";

// uniform handlers for gl::Program
namespace gl
{
	void setProgramUniform(GLuint index, const glm::vec2& v) { glUniform2fv(index, 2, glm::value_ptr(v)); }
	void setProgramUniform(GLuint index, const glm::vec3& v) { glUniform3fv(index, 3, glm::value_ptr(v)); }
	void setProgramUniform(GLuint index, const glm::vec4& v) { glUniform4fv(index, 4, glm::value_ptr(v)); }
	void setProgramUniform(GLuint index, const glm::mat4& m) { glUniformMatrix4fv(index, 1, GL_FALSE, glm::value_ptr(m)); }
}

struct Object;

static GLFWwindow* window{};
static gl::Program* textureShader{};
static gl::VertexArray* quadVBO{};
static std::vector<Object*> objects;
static glm::ivec2 windowRes{};

static glm::vec2 getMousePosition()
{
	double x, y;

	glfwGetCursorPos(window, &x, &y);

	return { x, y};
}

struct Object
{
	virtual ~Object() = default;

	struct {
	public:
		glm::vec3 position{};
		float rotation{ 0 };
		glm::vec3 scale{ 1, 1, 1 };

		const glm::mat4& getMatrix() const
		{
			glm::mat4 model{1};

			model = glm::translate(model, glm::vec3(position));
			
			// center for rotation
			model = glm::translate(model, 0.5f * scale);
			model = glm::rotate(model, rotation, { 0, 0, 1 });
			model = glm::translate(model, -0.5f * scale);
			model = glm::scale(model, scale);
			
			return model;
		}

	} transform;

	virtual void update() {}
	virtual void render(const glm::mat4& pvMat) {}
};

struct Ball : Object
{
	virtual void update()
	{
		
	}

	virtual void render(const glm::mat4& pvMat) override
	{
		quadVBO->bind();
		textureShader->bind();

		textureShader->setUniform("mvp", pvMat * transform.getMatrix());
		quadVBO->draw();
	}
};

void initGLStuff()
{
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

	quadVBO = new gl::VertexArray(
		verts, 4,
		indices, 6,
		{
			{ 0, GL_FLOAT,	3, offsetof(Vertex, position) },
			{ 1, GL_INT,	2, offsetof(Vertex, texcoord) }
		}
	);

	textureShader = new gl::Program({
		{ GL_VERTEX_SHADER,		textureShaderSource, { "#version 330", "#define VERTEX_SHADER" } },
		{ GL_FRAGMENT_SHADER,	textureShaderSource, { "#version 330", "#define FRAGMENT_SHADER" } }
	});

	if (!textureShader->isValid())
	{
		printf("Program error: %s\n", textureShader->getLastError().c_str());
		assert(false);
	}
}

int main(int, const char**)
{
	glfwInit();

	if ((window = glfwCreateWindow(1280, 720, "example", nullptr, nullptr)))
	{
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		initGLStuff();
		
		glfwSetTime(0);

		objects.push_back(new Ball());

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

			glfwGetFramebufferSize(window, &windowRes.x, &windowRes.y);
			glViewport(0, 0, windowRes.x, windowRes.y);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			const float ratioW = windowRes.x;
			const float ratioH = windowRes.y;

			glm::mat4 proj = glm::ortho<float>(0, 1, 1, 0, -1.f, 1.f);
			
			for (auto* object : objects)
			{
				//object->update();
				object->render(proj);
			}

			OGLPP_ERROR_CHECK();
			glfwSwapBuffers(window);
		}

		glfwDestroyWindow(window);
	}

	return 0;
}