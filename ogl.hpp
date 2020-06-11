#pragma once

#include <initializer_list>
#include <string>
#include <vector>
#include <cassert>

#define OGLPP_ASSERT(expr) assert(expr);

#ifdef _DEBUG
#define OGLPP_ERROR_CHECK() OGLPP_ASSERT(!(gl::getError()))
#else
#define OGLPP_ERROR_CHECK()
#endif

#define OGLPP_NOT_COPYABLE(Name) \
Name(const Name& other) = delete; \
void operator=(const Name& other) = delete;

#define OGLPP_MOVEABLE(Name, MOVER) \
Name(Name&& other) noexcept { std::swap(*this, other); } \
void operator=(Name&& other) noexcept MOVER 

#define OGLPP_NOT_COPYABLE_BUT_MOVEABLE(Name, MOVER) \
OGLPP_NOT_COPYABLE(Name) \
OGLPP_MOVEABLE(Name, MOVER)

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
#undef GL_ERROR_CASE

		return nullptr;
	}

	struct VertexArray
	{
		struct Layout 
		{ 
			int index{};
			GLenum type = GL_FLOAT;
			int count{};
			int offset{};

			Layout(int cIndex, GLenum cType, int cCount, int cOffset = 0) :
				index(cIndex), type(cType), count(cCount), offset(cOffset) {}
		};

	private:
		struct VertexBuffer
		{
			int numVerts{};
			GLuint handle{};
		} vbo;

		struct ElementBuffer
		{
			int numElems{};
			GLuint handle{};
		} ebo;

		GLuint handle{};

	public:
		VertexArray()
		{
			glGenVertexArrays(1, &handle);
			bind();
			OGLPP_ERROR_CHECK();
		}

		template<typename VertexT>
		VertexArray(const VertexT* verts, int numVerts, std::initializer_list<Layout>&& layout) :
			VertexArray()
		{
			setVertexData(verts, numVerts, std::move(layout));
			OGLPP_ERROR_CHECK();
		}

		template<typename VertexT>
		VertexArray(const VertexT* verts, int numVerts, const uint32_t* indices, int numIndices, std::initializer_list<Layout>&& layout) :
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

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(VertexArray,
			{
				std::swap(handle, other.handle);
				std::swap(vbo, other.vbo);
				std::swap(ebo, other.ebo);
			});

		auto getHandle() const { return handle; }
		bool isValid() const { return handle; }

		template<typename VertexT>
		void setVertexData(const VertexT* verts, int numVerts, std::initializer_list<Layout>&& layout)
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
				const uintptr_t offset = elem.offset;
				glVertexAttribPointer(elem.index, elem.count, elem.type, GL_FALSE, sizeof(VertexT), (void*)offset);
				OGLPP_ERROR_CHECK();
			}
		}

		template<typename VertexT>
		void updateVertexData(const VertexT* verts, int numVerts, int offset = 0)
		{
			bind();

			OGLPP_ASSERT(vbo.handle);

			if (offset + numVerts > vbo.numVerts)
			{
				// TODO: resize buffer
				OGLPP_ASSERT(false);
			}

			glBufferSubData(GL_ARRAY_BUFFER, offset, numVerts * sizeof(VertexT), verts);
			OGLPP_ERROR_CHECK();
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

		void bind() const
		{
			OGLPP_ASSERT(isValid());
			glBindVertexArray(handle);
			OGLPP_ERROR_CHECK();
		}

		void release() const
		{
			glBindVertexArray(0);
		}

		void draw(GLenum mode = GL_TRIANGLES)
		{
			bind();
			
			if (ebo.handle)
				glDrawElements(mode, ebo.numElems, GL_UNSIGNED_INT, nullptr);
			else
				glDrawArrays(mode, 0, vbo.numVerts);

			OGLPP_ERROR_CHECK();
		}
	};

	struct Texture
	{
		virtual ~Texture() = default;
		virtual void resize(int width, int height) = 0;
	};

	struct Texture2D : Texture
	{
	private:
		GLuint handle{};
	public:
		Texture2D()
		{
			glGenTextures(1, &handle);
			bind();
			OGLPP_ERROR_CHECK();
		}

		Texture2D(const void* data, int width, int height, GLenum imageFormat, GLenum dataType) :
			Texture2D()
		{
			setTextureData(data, width, height, imageFormat, dataType);
			setFilterMode(GL_NEAREST);
			OGLPP_ERROR_CHECK();
		}

		Texture2D(int width, int height, GLenum imageFormat, GLenum dataType) :
			Texture2D(nullptr, width, height, imageFormat, dataType)
		{
		}

		~Texture2D()
		{
			glDeleteTextures(1, &handle);
			handle = 0;
		}

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(Texture2D, { std::swap(handle, other.handle); });

		auto getHandle() const { return handle; }
		bool isValid() const { return handle; }

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
			OGLPP_ERROR_CHECK();
		}

		void setFilterMode(GLenum mode)
		{
			bind();

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode);
			OGLPP_ERROR_CHECK();
		}

		void resize(int width, int height) override
		{
			OGLPP_ASSERT(false);
		}

		void bind() const
		{ 
			OGLPP_ASSERT(isValid());
			glBindTexture(GL_TEXTURE_2D, handle); 
			OGLPP_ERROR_CHECK();
		}
		
		void release() const
		{
			glBindTexture(GL_TEXTURE_2D, 0); 
		}

		int getWidth() const 
		{
			int w;

			bind();

			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			OGLPP_ERROR_CHECK();
			
			return w;
		}

		int getHeight() const 
		{
			int h;

			bind();

			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			OGLPP_ERROR_CHECK();
			
			return h;
		}
	};

	struct DepthTexture : Texture
	{
	private:
		GLuint handle{};
	public:
		DepthTexture()
		{
			glGenRenderbuffers(1, &handle);
			bind();
		}

		DepthTexture(int width, int height, GLenum format) :
			DepthTexture()
		{
			glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
			OGLPP_ERROR_CHECK();
		}

		~DepthTexture()
		{
			glDeleteRenderbuffers(1, &handle);
		}

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(DepthTexture, { std::swap(handle, other.handle); });

		auto getHandle() const { return handle; }
		bool isValid() const { return handle; }

		void resize(int width, int height) override
		{
			GLint format;
			bind();

			glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &format);
			OGLPP_ERROR_CHECK();
			
			glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)format, width, height);
			OGLPP_ERROR_CHECK();
		}

		void bind() const
		{
			OGLPP_ASSERT(isValid());
			glBindRenderbuffer(GL_RENDERBUFFER, handle);
			OGLPP_ERROR_CHECK();
		}
		
		void release() const
		{ 
			glBindRenderbuffer(GL_RENDERBUFFER, 0); 
		}
	};

	struct FrameBuffer
	{
	private:
		GLuint handle{};

		std::vector<Texture2D*> colourTargets;
		DepthTexture* depthTarget{};
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
				if (auto* tex2d = dynamic_cast<Texture2D*>(elem.second))
					attachColourTarget(tex2d, elem.first);
				else
					attachDepthTarget((DepthTexture*)elem.second);

			isReady();
		}

		~FrameBuffer()
		{
			glDeleteFramebuffers(1, &handle);
		}

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(FrameBuffer,
		{
			std::swap(handle, other.handle);
			std::swap(colourTargets, other.colourTargets);
			std::swap(depthTarget, other.depthTarget);
		});

		auto getHandle() const { return handle; }
		auto isValid() const { return handle; }

		Texture2D* getColourTarget(int index) const { return colourTargets[index]; }
		DepthTexture* getDepthTarget() const { return depthTarget; }

		void attachColourTarget(Texture2D* target, int index = -1)
		{
			bind();
	
			glFramebufferTexture(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + index,
				target->getHandle(),
				0
			);

			colourTargets.push_back(target);
		}

		void attachDepthTarget(DepthTexture* depth)
		{
			bind();

			// check we don't already have a depth target attached to this framebuffer
			OGLPP_ASSERT(!depthTarget);

			glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				GL_RENDERBUFFER,
				depth->getHandle()
			);

			depthTarget = depth;

			OGLPP_ERROR_CHECK();
		}

		void resize(int width, int height)
		{
			for (auto* target : colourTargets)
					target->resize(width, height);

			if (depthTarget)
				depthTarget->resize(width, height);
		}

		void bind() const
		{
			OGLPP_ASSERT(isValid());
			glBindFramebuffer(GL_FRAMEBUFFER, handle);
			OGLPP_ERROR_CHECK();
		}

		void release() const
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		bool isReady() const
		{
			return glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE;
		}

		void draw() const
		{
			int usedSlots = 0;
			GLenum targets[32]{};

			for (int i = 0; i < colourTargets.size(); i++)
				targets[i] = GL_COLOR_ATTACHMENT0 + i;

			if (depthTarget)
				targets[colourTargets.size()] = GL_DEPTH_ATTACHMENT;

			glDrawBuffers(usedSlots, targets);
			OGLPP_ERROR_CHECK();
		}

		static void bindDefaultBuffer()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	};

	struct Program
	{
		struct Shader 
		{
			GLenum type;
			const char* source;
			std::initializer_list<const char*> defines;
		};
	private:
		GLuint handle{};
		std::string errorString;

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

		OGLPP_NOT_COPYABLE_BUT_MOVEABLE(Program,
			{
				std::swap(handle, other.handle);
				std::swap(errorString, other.errorString);
			});

		auto getHandle() const { return handle; }
		auto isValid() const { return handle; }

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

				errorString.resize(errLength + 1);
				
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

				errorString.resize(errLength + 1);

				glDeleteProgram(handle);
				handle = 0;

				return false;
			}

			OGLPP_ERROR_CHECK();

			return true;
		}

		void reset()
		{
			glDeleteProgram(handle);
			handle = glCreateProgram();
			OGLPP_ERROR_CHECK();
		}

		void bind() const
		{
			OGLPP_ASSERT(isValid());
			glUseProgram(handle);
			OGLPP_ERROR_CHECK();
		}

		void release() const
		{
			glUseProgram(0);
		}

		void setSampler(const char* name, int loc, Texture2D* tex)
		{
			glActiveTexture(GL_TEXTURE0 + loc);
			tex->bind();
			setUniform(name, loc);
		}

		void setSampler(const char* name, int loc, DepthTexture* tex)
		{
			glActiveTexture(GL_TEXTURE0 + loc);
			tex->bind();
			setUniform(name, loc);
		}

		auto getUniformLocation(const char* name) const 
		{ 
			const auto index = glGetUniformLocation(handle, name);

			return index;
		}

		template<typename T>
		void setUniform(const char* name, const T& value) { int index{}; if ((index = getUniformLocation(name)) >= 0) gl::setProgramUniform(index, value); }
	};

	static inline void setProgramUniform(GLuint index, GLuint i) { glUniform1i(index, i); }
	static inline void setProgramUniform(GLuint index, int i) { glUniform1i(index, i); }
	static inline void setProgramUniform(GLuint index, float v) { glUniform1f(index, v); }
}