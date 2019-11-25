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
#include <vector>

#define OGLPP_NOT_COPYABLE(Name) \
Name(const Name& other) = delete; \
void operator=(const Name& other) = delete;

#define OGLPP_DEFAULT_MOVER { std::swap(handle, other.handle); }

#define OGLPP_MOVEABLE(Name, MOVER) \
Name(Name&& other) noexcept { std::swap(*this, other); } \
void operator=(Name&& other) noexcept MOVER 

#define OGLPP_NOT_COPYABLE_BUT_MOVEABLE(Name, MOVER) \
OGLPP_NOT_COPYABLE(Name) \
OGLPP_MOVEABLE(Name, MOVER)

#ifdef WIN32
void _debugbreak();
#define OGLPP_ASSERT(expr) if (!(expr)) __debugbreak();
#endif

#ifdef _DEBUG || defined(OGL_ASSERT_ON_ERROR)
#define OGLPP_ERROR_CHECK() OGLPP_ASSERT(!gl::getError())
#else
#define OGLPP_ERROR_CHECK() void
#endif

namespace gl
{
	// returns the last error string
	// or nullptr if no error
	static const char* getError()
	{
#define GL_ERROR_CASE(glenum) case glenum: return #glenum;
		switch (glGetError())
		{
		default:
		case GL_NO_ERROR: break;
		GL_ERROR_CASE(GL_INVALID_ENUM);
		GL_ERROR_CASE(GL_INVALID_VALUE);
		GL_ERROR_CASE(GL_INVALID_OPERATION);
		GL_ERROR_CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
		GL_ERROR_CASE(GL_OUT_OF_MEMORY);
		GL_ERROR_CASE(GL_STACK_UNDERFLOW);
		GL_ERROR_CASE(GL_STACK_OVERFLOW);
		}

		return nullptr;
	}

	struct Object
	{
	protected:
		GLuint handle = 0;
		virtual ~Object() = default;

	public:
		virtual void bind() const = 0;
		virtual void release() const = 0;
		GLuint getID() const { return handle; }
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
			OGLPP_ERROR_CHECK();
		}

		template<typename VertexT>
		VertexArray(const VertexT* verts, int numVerts, std::initializer_list<Layout> layout) :
			VertexArray()
		{
			setVertexData(verts, numVerts, std::move(layout));
			OGLPP_ERROR_CHECK();
		}

		template<typename VertexT>
		VertexArray(const VertexT* verts, int numVerts, const uint32_t* indices, int numIndices, std::initializer_list<Layout> layout) :
			VertexArray()
		{
			setVertexData(verts, numVerts, std::move(layout));
			setElementData(indices, numIndices);
			OGLPP_ERROR_CHECK();
		}

		~VertexArray()
		{
			glDeleteVertexArrays(1, &handle);
			handle = 0;
		}

		template<typename VertexT>
		void setVertexData(const VertexT* verts, int numVerts, std::initializer_list<Layout> layout)
		{
			bind();

			if (!vbo.handle)
				glGenBuffers(1, &vbo.handle);
			OGLPP_ERROR_CHECK();

			glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);
			OGLPP_ERROR_CHECK();

			vbo.numVerts = numVerts;
			glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(VertexT), verts, GL_STATIC_DRAW);
			OGLPP_ERROR_CHECK();

			for (auto elem : layout)
			{
				glEnableVertexAttribArray(elem.index);
				glVertexAttribPointer(elem.index, elem.size, elem.type, GL_FALSE, sizeof(Vertex), (void*)elem.offset);
				OGLPP_ERROR_CHECK();
			}
		}

		void setElementData(const uint32_t* indices, int numIndices)
		{
			bind();

			if (!ebo.handle)
				glGenBuffers(1, &ebo.handle);
			OGLPP_ERROR_CHECK();

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.handle);
			OGLPP_ERROR_CHECK();

			ebo.numElems = numIndices;
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint32_t), indices, GL_STATIC_DRAW);
			OGLPP_ERROR_CHECK();
		}

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(VertexArray, 
		{
			std::swap(handle, other.handle);
			std::swap(vbo, other.vbo);
			std::swap(ebo, other.ebo);
		});

		void bind() override 
		{
			OGLPP_ASSERT(isValid());
			glBindVertexArray(handle);
			OGLPP_ERROR_CHECK();
		}

		void release() override 
		{
			glBindVertexArray(0);
		}

		void draw(GLenum mode = GL_TRIANGLES)
		{
			OGLPP_ASSERT(isValid());

			if (ebo.handle)
			{
				glDrawElements(mode, ebo.numElems, GL_UNSIGNED_INT, nullptr);
			}
			else
			{
				glDrawArrays(mode, 0, vbo.numVerts);
			}

			OGLPP_ERROR_CHECK();
		}
	};

	struct Texture : virtual Object
	{
		virtual ~Texture() = default;
	};

	struct Texture2D : Texture
	{
	private:
	public:
		Texture2D()
		{
			glGenTextures(1, &handle);
			bind();
		}

		Texture2D(const void* data, int width, int height, GLenum imageFormat, GLenum dataType) :
			Texture2D()
		{
			setTextureData(data, width, height, imageFormat, dataType);
			setFilterMode(GL_NEAREST);
		}

		~Texture2D()
		{
			glDeleteTextures(1, &handle);
			handle = 0;
		}

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(Texture2D,
		{
			std::swap(handle, other.handle);
		});

		void setTextureData(const void* data, int width, int height, GLenum imageFormat, GLenum dataType)
		{
			bind();

			glTexImage2D(GL_TEXTURE_2D, 0, imageFormat, width, height, 0, imageFormat, dataType, data);
			OGLPP_ERROR_CHECK();
		}

		void setWrapMode(GLenum mode)
		{
			bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);
		}

		void setFilterMode(GLenum mode)
		{
			bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode);
		}

		void bind() override 
		{ 
			OGLPP_ASSERT(isValid());
			glBindTexture(GL_TEXTURE_2D, handle); 
		}
		
		void release() override 
		{
			OGLPP_ASSERT(isValid());
			glBindTexture(GL_TEXTURE_2D, 0); 
		}

		int getWidth() const 
		{ 
			bind();

			int w;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			return w;
		}

		int getHeight() const 
		{
			bind();

			int h;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			return h;
		}
	};

	struct DepthTexture : Texture
	{
		DepthTexture()
		{
			glGenRenderbuffers(1, &handle);
			bind();
		}

		~DepthTexture()
		{
			glDeleteRenderbuffers(1, &handle);
		}

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(DepthTexture, {  });

		void bind() override
		{
			OGLPP_ASSERT(isValid());
			glBindRenderbuffer(GL_RENDERBUFFER, handle); 
		}
		
		void release() override 
		{ 
			OGLPP_ASSERT(isValid());
			glBindRenderbuffer(GL_RENDERBUFFER, 0); 
		}
	};

	struct FrameBuffer : Object
	{
	private:
		Texture* colourTargets[8]{ nullptr };
		Texture* depthTarget{ nullptr };
	public:
		FrameBuffer()
		{
			glGenFramebuffers(1, &handle);
			bind();
		}

		FrameBuffer(std::initializer_list<std::pair<int, Texture*>>&& list) :
			FrameBuffer()
		{
			for (const auto& elem : list)
				attachTarget(elem.second, elem.first);
		}

		~FrameBuffer()
		{
			glDeleteFramebuffers(1, &handle);
		}

		Texture* getColourTarget(int index) const { return colourTargets[index]; }
		Texture* getDepthTarget() const { return depthTarget; }

		void attachTarget(Texture* target, int index = -1)
		{
			bind();
			
			if (auto* tex2d = dynamic_cast<Texture2D*>(target))
			{
				// check we don't have index collisions for colour targets
				if (index >= 0)
				{
					for (const auto& targetEntry : colourTargets)
						OGLPP_ASSERT(targetEntry.index != index);
				}

				glFramebufferTexture(
					GL_FRAMEBUFFER,
					GL_COLOR_ATTACHMENT0 + index,
					tex2d->getID(),
					0
				);
			}
			else if (auto* depthTex = dynamic_cast<DepthTexture*>(target))
			{
				// check we don't already have a depth target attached to this framebuffer
				for (const auto& depthTarget : targets)
					OGLPP_ASSERT(!dynamic_cast<const DepthTexture*>(depthTarget.target));

				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER,
					GL_DEPTH_ATTACHMENT,
					GL_RENDERBUFFER,
					depthTex->getID()
				);
			}

			targets.push_back({ index, target });
			OGLPP_ERROR_CHECK();
		}

		void bind() override
		{
			OGLPP_ASSERT(isValid());
			glBindFramebuffer(GL_FRAMEBUFFER, handle);
		}

		void release() override
		{
			OGLPP_ASSERT(isValid());
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
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
			glDeleteProgram(handle);
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
			
			errorString.clear();

			GLuint shaderHandle = glCreateShader(type);

			for (auto* define : defines)
			{
				sourceString += define;
				sourceString += "\n";
			}

			sourceString += source;

			const char* src[] = { sourceString.c_str() };
			glShaderSource(shaderHandle, 1, src, nullptr);
			OGLPP_ERROR_CHECK();

			glCompileShader(shaderHandle);
			OGLPP_ERROR_CHECK();

			GLint success;
			glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &success);
			OGLPP_ERROR_CHECK();

			if (success == GL_FALSE)
			{
				GLint errLength = 0;

				errorString.resize(4096);

				glGetShaderInfoLog(shaderHandle, 4096-1, &errLength, (char*)errorString.c_str());

				errorString.resize(errLength+1);
				
				glDeleteShader(shaderHandle);
				return false;
			}

			glAttachShader(handle, shaderHandle);
			glDeleteShader(shaderHandle);
			OGLPP_ERROR_CHECK();

			return true;
		}

		bool link()
		{
			status = false;
			errorString.clear();

			glLinkProgram(handle);
			OGLPP_ERROR_CHECK();

			GLint success;
			glGetProgramiv(handle, GL_LINK_STATUS, &success);

			if (success == GL_FALSE)
			{
				GLint errLength = 0;

				errorString.resize(4096);

				glGetShaderInfoLog(handle, 4096-1, &errLength, (char*)errorString.c_str());

				errorString.resize(errLength);

				return false;
			}

			status = true;

			return true;
		}

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(Program,
		{
			std::swap(handle, other.handle);
		});

		void reset()
		{
			glDeleteProgram(handle);
			handle = glCreateProgram();
		}

		void bind() override
		{
			glUseProgram(handle);
		}

		void release() override
		{
			glUseProgram(0);
		}

		void setSampler(const char* name, int loc, Texture2D* tex)
		{
			glActiveTexture(GL_TEXTURE0 + loc);
			tex->bind();
			setUniform(name, loc);
		}

		void setUniform(const char* name, int i) { glUniform1i(glGetUniformLocation(handle, name), i); }
		void setUniform(const char* name, float v) { glUniform1f(glGetUniformLocation(handle, name), v); }
		void setUniform(const char* name, float v1, float v2) { glUniform2f(glGetUniformLocation(handle, name), v1, v2); }
		void setUniform(const char* name, float v1, float v2, float v3) { glUniform3f(glGetUniformLocation(handle, name), v1, v2, v3); }
		void setUniform(const char* name, float v1, float v2, float v3, float v4) { glUniform4f(glGetUniformLocation(handle, name), v1, v2, v3, v4); }
		
		OGLPP_GLM_UNIFORM_METHODS;
	};
}