#include "systems/opengl/RenderTarget.h"

namespace Sigma{
	RenderTarget::~RenderTarget() {
		glDeleteTextures(this->texture_ids.size(), &this->texture_ids[0]); // Perhaps should check if texture was created for this RT or is used elsewhere
		glDeleteRenderbuffers(1, &this->depth_id);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &this->fbo_id);
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
