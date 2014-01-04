#include "systems/opengl/RenderPass.h"

#include <memory>
#include <map>
#include <vector>
#include <list>
#include <string>
#include "IGLComponent.h"
#include "systems/opengl/GLSLShader.h"

namespace Sigma{
	RenderPass::RenderPass() : passEnabled(true), depthEnabled(false), blendEnabled(false) {}

	RenderPass::~RenderPass() {}

	void RenderPass::AddProgram(std::shared_ptr<GLSLProgram> program){
		this->programs.push_back(program);
		this->batches.push_back(Batch());
	}

	void RenderPass::SetReadBuffer(std::shared_ptr<RenderTarget> &target){
		this->readFBO = target;
	}

	void RenderPass::AddReadAttachment(const GLuint attachment, const GLuint tex_num){
		this->readAttachments.push_back(attachment);
		this->readAttachments.push_back(tex_num);
	}

	void RenderPass::SetWriteBuffer(std::shared_ptr<RenderTarget> &target){
		this->writeFBO = target;
	}

	void RenderPass::AddWriteAttachment(const GLuint attachment, const GLuint tex_num){
		this->writeAttachments.push_back(attachment);
		this->writeAttachments.push_back(tex_num);
	}

	void RenderPass::AddCopyTarget(GLuint fbo_target, GLuint channel, GLuint interpolation){
		this->copyTargets.push_back(fbo_target);
		this->copyTargets.push_back(channel);
		this->copyTargets.push_back(interpolation);
	}

	/** Add a component to one of the shader programs used by this Pass */
	void RenderPass::AddToBatch(const std::string &program_name, std::weak_ptr<IGLComponent> component){
		for(int i=0; i<this->programs.size(); ++i){
			if(this->programs[i]->UniqueName() == program_name){
				this->batches[i].push_back(component);
				break;
			}
		}
	}

	/** Perform this render pass. Called once per frame. */
	void RenderPass::Execute(IGLView *view, glm::mat4 *projection){
		if(!this->passEnabled) return;

		//////////////////
		// BIND BUFFERS //
		//////////////////

		if(this->readFBO.get() != nullptr){
			this->readFBO->BindRead();
			for(int i=0; i<this->readAttachments.size(); i+=2){
				glActiveTexture(GL_TEXTURE0 + this->readAttachments[i]);
				// TODO allow other texture types
				glBindTexture(GL_TEXTURE_2D, this->readFBO->texture_ids[this->readAttachments[i+1]]);
			}
		}
		if(this->writeFBO.get() != nullptr){
			// TODO drawBuffers only on specified targets
			this->writeFBO->BindWrite();
		}

		////////////////////////
		// LOOP GLSL PROGRAMS //
		////////////////////////

		for(int i=0; i<this->programs.size(); ++i){
			GLSLProgram &program = *this->programs[i];
			Batch &batch = this->batches[i];
			program.Use();
			{
				program.FrameUpdateUniforms(view, projection);
				std::list<std::weak_ptr<IGLComponent>>::iterator itr = batch.begin();
				while(itr != batch.end()){
					// std::list was used because weak_ptrs may be invalid at any time, and erase() is O(1)
					std::shared_ptr<IGLComponent> comp;
					// if component is gone, remove this reference from the list
					if(!(comp = (*itr).lock())){
						itr = batch.erase(itr);
						continue;
					}
					// component is still valid;
					else{
						program.ComponentUpdateUniforms(comp.get());
						// TODO completely remove components handling matrices (can be done in FrameUpdateUniforms)
						comp->Render(nullptr, nullptr);
						itr++;
					}
				}
			}
			program.UnUse();
		}
	}
} // namespace Sigma
