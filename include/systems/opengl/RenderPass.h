#pragma once
#ifndef RENDERPASS_H
#define RENDERPASS_H

#include <memory>
#include <map>
#include <vector>
#include <list>
#include <string>
#include "IGLComponent.h"
#include "systems/opengl/RenderTarget.h"
#include "systems/opengl/IGLSLShader.h"
#include "systems/opengl/GLSLProgram.h"

namespace Sigma{

	class RenderPass
	{
	public:
		typedef std::list<std::weak_ptr<IGLComponent>> Batch;

		RenderPass();
		virtual ~RenderPass();

		// INIT METHODS
		// (called once)

		void AddProgram(const std::shared_ptr<GLSLProgram> program);

		void SetReadBuffer(std::shared_ptr<RenderTarget> &target);
		void AddReadAttachment(const GLuint attachment, const GLuint tex_num);

		void SetWriteBuffer(std::shared_ptr<RenderTarget> &target);
		void AddWriteAttachment(const GLuint attachment, const GLuint tex_num);

		void AddCopyTarget(GLuint fbo_target, GLuint channel, GLuint interpolation);
		void SetViewport(int x0, int y0, int x1, int y1) { vpx0 = x0; vpy0 = y0; vpx1 = x1; vpy1 = y1; }

		void SetDepthTestEnabled(bool enabled) { depthEnabled = enabled; }
		void SetBlendEnabled(bool enabled) { blendEnabled = enabled; }
		void SetBlendFunction(GLuint srcFunc, GLuint dstFunc) { blendSrc = srcFunc; blendDst = dstFunc; }

		// UPDATE METHODS
		// (called often)

		/** Add a component to one of the shader programs used by this Pass */
		void AddToBatch(const std::string &program_name, std::weak_ptr<IGLComponent> component);

		/** Perform this render pass. Called once per frame. */
		void Execute(IGLView *view, glm::mat4 *projection);

		void Disable() { passEnabled = false; }
		void Enable() { passEnabled = true; }
		bool IsEnabled() { return passEnabled; }

	private:
		bool passEnabled, depthEnabled, blendEnabled;
		GLuint blendSrc, blendDst;
		int vpx0, vpy0, vpx1, vpy1; // viewport coordinates
		// each read and write attachment uses 2 values:
		// [2i+0] for the attachment point (i.e. GL_TEXTURE0+attachments[2i])
		// [2i+1] to specify which texture (i.e. renderTarget->texture_ids[attachments[2i+1]])
		std::vector<GLuint> readAttachments;
		std::vector<GLuint> writeAttachments;
		// programs and batches are parallel arrays (that is, programs[i] uses batches[i])
		std::vector<std::shared_ptr<GLSLProgram>> programs;
		std::vector<Batch> batches;
		// each copyTarget uses 3 values:
		// [3i+0] for target fbo, [3i+1] for channel and [3i+2] for interpolation method
		std::vector<GLuint> copyTargets;
		std::shared_ptr<RenderTarget> readFBO, writeFBO;
	}; // class RenderPass
} // namespace Sigma
#endif // RENDERPASS_H
