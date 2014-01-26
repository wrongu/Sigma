#include "systems/opengl/RenderTarget.h"
#include "systems/opengl/OpenGLSystem.h"
#include "Sigma.h"
#include <iostream>

namespace Sigma{
	RenderTarget::RenderTarget(const int w, const int h, const bool depth) : width(w), height(h), hasDepth(depth)
	{
		this->InitBuffers();
	}

	RenderTarget::~RenderTarget()
	{
		glDeleteTextures(this->texture_ids.size(), &this->texture_ids[0]); // Perhaps should check if texture was created for this RT or is used elsewhere
		if(this->hasDepth){
			glDeleteRenderbuffers(1, &this->depth_id);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &this->fbo_id);
	}

	void RenderTarget::InitBuffers(){
		// Create the frame buffer object
		glGenFramebuffers(1, &this->fbo_id);
		printOpenGLError();

		// Create the depth render buffer
		if(this->hasDepth) {
			this->CreateDepthBuffer();
		}

		//Does the GPU support current FBO configuration?
		GLenum status;
		status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

		switch(status) {
			case GL_FRAMEBUFFER_COMPLETE:
				LOG << "Successfully created render target.";
				break;
			default:
				LOG_ERROR << "Error: Framebuffer format is not compatible.";
				assert(0 && "Error: Framebuffer format is not compatible.");
		}

		// Unbind objects to get them out of the way
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	void RenderTarget::CreateDepthBuffer(){
		// Make sure we're on the back buffer to test depth bits
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Get backbuffer depth bit width
		int depthBits;
#ifdef __APPLE__
		// The modern way.
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits);
#else
		glGetIntegerv(GL_DEPTH_BITS, &depthBits);
#endif

		glGenRenderbuffers(1, &this->depth_id);
		glBindRenderbuffer(GL_RENDERBUFFER, this->depth_id);
		{
			// set bit width for this depth buffer
			// tests indicated that DEPTH_24_STENCIL8 is the most compatible cross-platform value
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->width, this->height);
			printOpenGLError();
		}
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		//Attach depth buffer to this FBO
		glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
		{
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->depth_id);
			printOpenGLError();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLuint RenderTarget::CreateTexture(GLint format, GLenum internalFormat, GLenum type){
		GLuint texture_id;

		// bind this RT's frame buffer object so that the new texture is attached properly
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->fbo_id);
		{
			// create the texture
			glGenTextures(1, &texture_id);
			glBindTexture(GL_TEXTURE_2D, texture_id);
			{
				// Configure texture params for full screen quad
				// TODO make these editable
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				// Configure texture as a 2D image with the given bit formats
				// and the same size as its FBO
				glTexImage2D(GL_TEXTURE_2D, 0, format,
										 (GLsizei)this->width,
										 (GLsizei)this->height,
										 0, internalFormat, type,
										 NULL); // NULL means reserve texture memory, but texels are undefined

				//Attach 2D texture to this FBO
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+this->texture_ids.size(), GL_TEXTURE_2D, texture_id, 0);
				printOpenGLError();

				this->texture_ids.push_back(texture_id);
			}
			glBindTexture(GL_TEXTURE_2D, 0); // clear binding
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // clear binding

		return texture_id;
	}

	void RenderTarget::BindWrite() {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->fbo_id);

		std::vector<GLenum> buffers;

		for(unsigned int i=0; i < this->texture_ids.size(); i++) {
			buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
		}

		glDrawBuffers(this->texture_ids.size(), &buffers[0]);
}

	void RenderTarget::BindRead() {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, this->fbo_id);
	}

	void RenderTarget::UnbindWrite() {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		std::vector<GLenum> buffers;
		buffers.push_back(GL_COLOR_ATTACHMENT0);
		glDrawBuffers(1, &buffers[0]);
	}

	void RenderTarget::UnbindRead() {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
}
