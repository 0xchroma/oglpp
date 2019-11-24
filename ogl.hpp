#pragma once

#ifdef OGLPP_USE_GLAD
#include <glad/glad.h>
#endif

#ifdef OGLPP_USE_GLM
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#define OGLPP_GLM_UNIFORM_METHODS \
void setUniform(const char* name, const glm::vec2& v) { glUniform2fv(glGetUniformLocation(handle, name), 2, glm::value_ptr(v)); } \
void setUniform(const char* name, const glm::vec3& v) { glUniform3fv(glGetUniformLocation(handle, name), 3, glm::value_ptr(v)); } \
void setUniform(const char* name, const glm::vec4& v) { glUniform4fv(glGetUniformLocation(handle, name), 4, glm::value_ptr(v)); } \
void setUniform(const char* name, const glm::mat4& m) { glUniformMatrix4fv(glGetUniformLocation(handle, name), 1, GL_FALSE, glm::value_ptr(m)); }

#endif

#ifndef OGLPP_GLM_UNIFORM_METHODS
#define OGLPP_GLM_UNIFORM_METHODS 
#endif

#include <algorithm>
#include <initializer_list>
#include <string>

#define NOT_COPYABLE(Name) \
Name(const Name& other) = delete; \
void operator=(const Name& other) = delete;

#define MOVEABLE(Name, MOVER) \
Name(Name&& other) noexcept { std::swap(*this, other); } \
void operator=(Name&& other) noexcept MOVER 

#define NOT_COPYABLE_BUT_MOVEABLE(Name, MOVER) \
NOT_COPYABLE(Name) \
MOVEABLE(Name, MOVER)

namespace gl
{
	struct Object
	{
	protected:
		GLuint handle = 0;
		virtual ~Object() = default;

	public:
		virtual void destroy() = 0;
		virtual void bind() = 0;
		virtual void release() = 0;
		GLuint getHandleID() const { return handle; }
		virtual bool isValid() const { return handle != 0; }
	};

	struct VertexArray : Object
	{
		struct Layout 
		{ 
			int index = 0;
			GLenum type = GL_FLOAT;
			int size = 0; 
			int offset = 0;
		};

	private:
		struct
		{
			int numVerts = 0;
			GLuint handle = 0;
		} vbo;

		struct ElementBuffer
		{
			int numElems = 0;
			GLuint handle = 0;
		} ebo;

	public:
		VertexArray()
		{
			glGenVertexArrays(1, &handle);
			bind();
		}

		template<typename VertexT>
		VertexArray(const VertexT* verts, int numVerts, std::initializer_list<Layout> layout) :
			VertexArray()
		{
			setVertexData(verts, numVerts, std::move(layout));
		}

		template<typename VertexT>
		VertexArray(const VertexT* verts, int numVerts, const uint32_t* indices, int numIndices, std::initializer_list<Layout> layout) :
			VertexArray()
		{
			setVertexData(verts, numVerts, std::move(layout));
			setElementData(indices, numIndices);
		}

		~VertexArray()
		{
			destroy();
		}

		template<typename VertexT>
		void setVertexData(const VertexT* verts, int numVerts, std::initializer_list<Layout> layout)
		{
			if (!vbo.handle)
				glGenBuffers(1, &vbo.handle);

			glBindBuffer(GL_VERTEX_ARRAY, vbo.handle);

			vbo.numVerts = numVerts;
			glBufferData(GL_VERTEX_ARRAY, numVerts * sizeof(VertexT), verts, GL_STATIC_DRAW);

			for (auto elem : layout)
			{
				glEnableVertexAttribArray(elem.index);
				glVertexAttribPointer(elem.index, elem.size, elem.type, GL_FALSE, sizeof(Vertex), (void*)elem.offset);
				glDisableVertexAttribArray(elem.index);
			}
		}

		void setElementData(const uint32_t* indices, int numIndices)
		{
			if (!ebo.handle)
				glGenBuffers(1, &ebo.handle);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.handle);

			ebo.numElems = numIndices;
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint32_t), indices, GL_STATIC_DRAW);
		}

		NOT_COPYABLE_BUT_MOVEABLE(VertexArray, 
		{
			std::swap(handle, other.handle);
			std::swap(vbo, other.vbo);
			std::swap(ebo, other.ebo);
		});

		void destroy() override 
		{
			glDeleteVertexArrays(1, &handle);
			handle = 0;
		}

		void bind() override 
		{
			glBindVertexArray(handle);
		}

		void release() override 
		{
			glBindVertexArray(0);
		}

		void draw(GLenum mode = GL_TRIANGLES)
		{
			if (ebo.handle)
			{
				glDrawElements(mode, ebo.numElems, GL_UNSIGNED_INT, nullptr);
			}
			else
			{
				glDrawArrays(mode, 0, vbo.numVerts);
			}
		}
	};

	struct Texture2D : Object
	{
	private:
	public:
		Texture2D()
		{
			glGenTextures(1, &handle);
			bind();
		}

		~Texture2D()
		{
			destroy();
		}

		void destroy() override
		{
			glDeleteTextures(1, &handle);
			handle = 0;
		}

		void bind() override { glBindTexture(GL_TEXTURE_2D, handle); }
		void release() override { glBindTexture(GL_TEXTURE_2D, 0); }
	};

	struct Program : Object
	{
		struct Shader 
		{
			GLenum type;
			const char* source;
			std::initializer_list<const char*> defines;
		};
	private:
		std::string errorString;
		bool status = false;
	public:
		Program()
		{
			reset();
		}

		// Compiles, attaches and links a set of shaders
		// check the isValid() method to determine if this failed
		Program(std::initializer_list<Shader> shaders) :
			Program()
		{
			for (const auto& shader : shaders)
			{
				if (!compileShader(shader.type, shader.source, shader.defines))
				{
					reset();
					return;
				}
			}

			if (!link())
				reset();
		}

		~Program()
		{
			destroy();
		}

		bool isValid() const override { return status; }
		const std::string& getLastError() const { return errorString; }

		// compiles a shader, and attaches if successful. 
		// This assumes the source is null terminated.
		// you can define defines for this compile pass, useful for single file, multi shader sources
		// defines are injected into the source buffer before the source text
		// so define like you would typically:
		// #define MY_DEFINE
		bool compileShader(GLenum type, const char* source, std::initializer_list<const char*> defines)
		{
			std::string sourceString;
			//const char* sourceArr[32] = {};

			GLuint shaderHandle = glCreateShader(type);

			// fill source array with defines and the source
			/*sourceArr[defines.size()] = source;
			for (auto* define : defines)
				sourceArr[*defines.begin() - define] = define;*/

			for (auto* define : defines)
			{
				sourceString += define;
				sourceString += "\n";
			}

			sourceString += source;

			const char* src[] = { sourceString.c_str() };
			glShaderSource(shaderHandle, 1, src, nullptr);

			glCompileShader(shaderHandle);

			GLint success;
			glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &success);

			if (success == GL_FALSE)
			{
				GLint errLength = 2048;

				errorString.clear();
				errorString.resize(errLength);

				glGetShaderInfoLog(shaderHandle, errLength, &errLength, (char*)errorString.c_str());

				errorString.resize(errLength+1);
				
				glDeleteShader(shaderHandle);
				return false;
			}

			glAttachShader(handle, shaderHandle);
			glDeleteShader(shaderHandle);

			return true;
		}

		bool link()
		{
			status = false;
			glLinkProgram(handle);

			GLint success;
			glGetProgramiv(handle, GL_LINK_STATUS, &success);

			if (success == GL_FALSE)
			{
				GLint errLength = 2048;

				errorString.clear();
				errorString.resize(errLength);

				glGetShaderInfoLog(handle, errLength, &errLength, (char*)errorString.c_str());

				errorString.resize(errLength);

				return false;
			}

			status = true;

			return true;
		}

		NOT_COPYABLE_BUT_MOVEABLE(Program,
		{
			std::swap(handle, other.handle);
		});

		void reset()
		{
			destroy();
			handle = glCreateProgram();
		}

		void destroy() override
		{
			glDeleteProgram(handle);
			handle = 0;
		}

		void bind() override
		{
			glUseProgram(handle);
		}

		void release() override
		{
			glUseProgram(0);
		}

		void setUniform(const char* name, float v) { glUniform1f(glGetUniformLocation(handle, name), v); }
		void setUniform(const char* name, float v1, float v2) { glUniform2f(glGetUniformLocation(handle, name), v1, v2); }
		void setUniform(const char* name, float v1, float v2, float v3) { glUniform3f(glGetUniformLocation(handle, name), v1, v2, v3); }
		void setUniform(const char* name, float v1, float v2, float v3, float v4) { glUniform4f(glGetUniformLocation(handle, name), v1, v2, v3, v4); }

		OGLPP_GLM_UNIFORM_METHODS;
	};
}