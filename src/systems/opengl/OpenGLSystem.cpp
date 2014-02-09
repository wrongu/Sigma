#include "systems/opengl/OpenGLSystem.h"
#include "systems/opengl/GLSLShader.h"
#include "systems/opengl/GLSixDOFView.h"
#include "controllers/FPSCamera.h"
#include "resources/GLSprite.h"
#include "resources/GLIcoSphere.h"
#include "resources/GLCubeSphere.h"
#include "resources/Mesh.h"
#include "resources/GLScreenQuad.h"
#include "components/PointLight.h"
#include "components/SpotLight.h"

#include "Sigma.h"

#ifdef __APPLE__
// Do not include <OpenGL/glu.h> because that will include gl.h which will mask all sorts of errors involving the use of deprecated GL APIs until runtime.
// gluErrorString (and all of glu) is deprecated anyway (TODO).
extern "C" const GLubyte * gluErrorString (GLenum error);
#else
#include "GL/glew.h"
#endif

#include "glm/glm.hpp"
#include "glm/ext.hpp"

namespace Sigma{

	OpenGLSystem::OpenGLSystem() : windowWidth(1024), windowHeight(768), deltaAccumulator(0.0),
		framerate(60.0f), pointQuad(1000), ambientQuad(1001), spotQuad(1002) {
			this->resSystem = resource::ResourceSystem::GetInstace();
			this->resSystem->CreateResourceLoader<resource::Mesh>();
			this->resSystem->CreateResourceLoader<resource::Texture>();
	}


	std::map<std::string, IFactory::FactoryFunction> OpenGLSystem::getFactoryFunctions() {
		using namespace std::placeholders;

		std::map<std::string, IFactory::FactoryFunction> retval;
		retval["GLSprite"] = std::bind(&OpenGLSystem::createGLSprite,this,_1,_2);
		retval["GLIcoSphere"] = std::bind(&OpenGLSystem::createGLIcoSphere,this,_1,_2);
		retval["GLCubeSphere"] = std::bind(&OpenGLSystem::createGLCubeSphere,this,_1,_2);
		retval["GLMesh"] = std::bind(&OpenGLSystem::createGLMesh,this,_1,_2);
		retval["FPSCamera"] = std::bind(&OpenGLSystem::createGLView,this,_1,_2);
		retval["GLSixDOFView"] = std::bind(&OpenGLSystem::createGLView,this,_1,_2);
		retval["PointLight"] = std::bind(&OpenGLSystem::createPointLight,this,_1,_2);
		retval["SpotLight"] = std::bind(&OpenGLSystem::createSpotLight,this,_1,_2);
		retval["GLScreenQuad"] = std::bind(&OpenGLSystem::createScreenQuad,this,_1,_2);

		return retval;
	}

	IComponent* OpenGLSystem::createGLView(const id_t entityID, const std::vector<Property> &properties) {
		this->views.push_back(new IGLView(entityID));

		float x=0.0f, y=0.0f, z=0.0f, rx=0.0f, ry=0.0f, rz=0.0f;

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &(*propitr);

			if (p->GetName() == "x") {
				x = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "y") {
				y = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "z") {
				z = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "rx") {
				rx = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "ry") {
				ry = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "rz") {
				rz = p->Get<float>();
				continue;
			}
		}

		PhysicalWorldLocation::GetTransform(entityID)->TranslateTo(x,y,z);
		PhysicalWorldLocation::GetTransform(entityID)->Rotate(rx,ry,rz);

		this->addComponent(entityID, this->views[this->views.size() - 1]);

		return this->views[this->views.size() - 1];
	}

	IComponent* OpenGLSystem::createGLSprite(const id_t entityID, const std::vector<Property> &properties) {
		GLSprite* spr = new GLSprite(entityID);
		float scale = 1.0f;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		int componentID = 0;
		std::string textureFilename;

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &(*propitr);
			if (p->GetName() == "scale") {
				scale = p->Get<float>();
			}
			else if (p->GetName() == "x") {
				x = p->Get<float>();
			}
			else if (p->GetName() == "y") {
				y = p->Get<float>();
			}
			else if (p->GetName() == "z") {
				z = p->Get<float>();
			}
			else if (p->GetName() == "id") {
				componentID = p->Get<int>();
			}
			else if (p->GetName() == "textureFilename"){
				textureFilename = p->Get<std::string>();
			}
		}

		std::shared_ptr<resource::Texture> texture = this->resSystem->Get<resource::Texture>(textureFilename);

		spr->SetTexture(texture);
		spr->LoadShader();
		spr->Transform()->Scale(glm::vec3(scale));
		spr->Transform()->Translate(x,y,z);
		spr->InitializeBuffers();
		this->addComponent(entityID,spr);
		return spr;
	}

	IComponent* OpenGLSystem::createGLIcoSphere(const id_t entityID, const std::vector<Property> &properties) {
		float scale = 1.0f;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		int componentID = 0;
		std::string shader_name = "shaders/icosphere";
		Renderable* renderable = new Renderable(entityID);

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &(*propitr);
			if (p->GetName() == "scale") {
				scale = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "x") {
				x = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "y") {
				y = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "z") {
				z = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "id") {
				componentID = p->Get<int>();
			}
			else if (p->GetName() == "shader"){
				shader_name = p->Get<std::string>();
			}
			else if (p->GetName() == "lightEnabled") {
				renderable->SetLightingEnabled(p->Get<bool>());
			}
		}
		std::shared_ptr<resource::GLIcoSphere> sphere(new resource::GLIcoSphere());
		sphere->InitializeBuffers();
		std::string meshname = "entity";
		meshname += entityID;
		meshname += "icosphere";
		this->resSystem->Add<resource::Mesh>(meshname, sphere);

		renderable->SetMesh(sphere);
		renderable->Transform()->Scale(scale,scale,scale);
		renderable->Transform()->Translate(x,y,z);
		renderable->LoadShader(shader_name);
		renderable->InitializeBuffers();
		renderable->SetCullFace("back");
		this->addComponent(entityID,renderable);
		return renderable;
	}

	IComponent* OpenGLSystem::createGLCubeSphere(const id_t entityID, const std::vector<Property> &properties) {
		Renderable* renderable = new Renderable(entityID);

		std::string texture_name = "";
		std::string shader_name = "shaders/cubesphere";
		std::string cull_face = "back";
		std::string depthFunc = "less";
		int subdivision_levels = 1;
		float rotation_speed = 0.0f;
		bool fix_to_camera = false;

		float scale = 1.0f;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float rx = 0.0f;
		float ry = 0.0f;
		float rz = 0.0f;
		int componentID = 0;

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &(*propitr);
			if (p->GetName() == "scale") {
				scale = p->Get<float>();
			}
			else if (p->GetName() == "x") {
				x = p->Get<float>();
			}
			else if (p->GetName() == "y") {
				y = p->Get<float>();
			}
			else if (p->GetName() == "z") {
				z = p->Get<float>();
			}
			else if (p->GetName() == "rx") {
				rx = p->Get<float>();
			}
			else if (p->GetName() == "ry") {
				ry = p->Get<float>();
			}
			else if (p->GetName() == "rz") {
				rz = p->Get<float>();
			}
			else if (p->GetName() == "subdivision_levels") {
				subdivision_levels = p->Get<int>();
			}
			else if (p->GetName() == "texture") {
				texture_name = p->Get<std::string>();
			}
			else if (p->GetName() == "shader") {
				shader_name = p->Get<std::string>();
			}
			else if (p->GetName() == "id") {
				componentID = p->Get<int>();
			}
			else if (p->GetName() == "cullface") {
				cull_face = p->Get<std::string>();
			}
			else if (p->GetName() == "fix_to_camera") {
				fix_to_camera = p->Get<bool>();
			}
			else if (p->GetName() == "lightEnabled") {
				renderable->SetLightingEnabled(p->Get<bool>());
			}
			else if (p->GetName() == "depthFunc") {
				depthFunc = p->Get<std::string>();
			}
		}

		std::shared_ptr<resource::GLCubeSphere> sphere(new resource::GLCubeSphere());
		sphere->SetSubdivisions(subdivision_levels);
		sphere->SetFixToCamera(fix_to_camera);
		sphere->LoadTexture(texture_name);
		sphere->InitializeBuffers();
		std::string meshname = "entity";
		meshname += entityID;
		meshname += "cubesphere";
		this->resSystem->Add<resource::Mesh>(meshname, sphere);

		renderable->SetMesh(sphere);
		renderable->SetCullFace(cull_face);
		renderable->SetDepthFunc(depthFunc);
		renderable->Transform()->Scale(scale,scale,scale);
		renderable->Transform()->Rotate(rx,ry,rz);
		renderable->Transform()->Translate(x,y,z);
		renderable->LoadShader(shader_name);
		renderable->InitializeBuffers();

		this->addComponent(entityID,renderable);
		return renderable;
	}

	IComponent* OpenGLSystem::createGLMesh(const id_t entityID, const std::vector<Property> &properties) {
		Renderable* renderable = new Renderable(entityID);

		float scale = 1.0f;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float rx = 0.0f;
		float ry = 0.0f;
		float rz = 0.0f;
		int componentID = 0;
		std::string cull_face = "back";
		std::string shaderfile = "";
		std::string meshFIlename = "";

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &*propitr;
			if (p->GetName() == "scale") {
				scale = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "x") {
				x = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "y") {
				y = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "z") {
				z = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "rx") {
				rx = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "ry") {
				ry = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "rz") {
				rz = p->Get<float>();
				continue;
			}
			else if (p->GetName() == "meshFile") {
				LOG << "Loading mesh: " << p->Get<std::string>();
				meshFIlename = p->Get<std::string>();
			}
			else if (p->GetName() == "shader") {
				shaderfile = p->Get<std::string>();
			}
			/*else if (p->GetName() == "textureReplace") {
				mesh->texReplace = p->Get<std::string>();
			}
			else if (p->GetName() == "replaceWith") {
				mesh->texReplaceWith = p->Get<std::string>();
			}*/
			else if (p->GetName() == "id") {
				componentID = p->Get<int>();
			}
			/*else if (p->GetName() == "parent") {
				GLTransform *th, *pr;
				int index = p->Get<int>();
				th = mesh->Transform();
				if(index == -1) {
					pr = this->GetView()->Transform();
				}
				else {
					pr = this->GetTransformFor(index);
				}
				if(th && pr) {
					th->SetParentTransform(pr);
				}
			}*/
			else if (p->GetName() == "cullface") {
				cull_face = p->Get<std::string>();
			}
			else if (p->GetName() == "lightEnabled") {
				renderable->SetLightingEnabled(p->Get<bool>());
			}
			else if (p->GetName() == "parent") {
				/* Only entities that have ControllableMove component can be parent */
				const id_t parentID = p->Get<int>();
				renderable->Transform()->SetParentID(parentID);
			}
		}

		std::shared_ptr<resource::Mesh> mesh = this->resSystem->Get<resource::Mesh>(meshFIlename);

		renderable->SetMesh(mesh);
		renderable->SetCullFace(cull_face);
		renderable->Transform()->Scale(scale,scale,scale);
		renderable->Transform()->Translate(x,y,z);
		renderable->Transform()->Rotate(rx,ry,rz);
		renderable->LoadShader(shaderfile);

		renderable->InitializeBuffers();
		this->addComponent(entityID,renderable);
		return renderable;
	}

	IComponent* OpenGLSystem::createScreenQuad(const id_t entityID, const std::vector<Property> &properties) {
		Renderable* renderable = new Renderable(entityID);

		float x = 0.0f;
		float y = 0.0f;
		float w = 0.0f;
		float h = 0.0f;

		int componentID = 0;
		std::string textureName;
		bool textureInMemory = false;

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &(*propitr);
			if (p->GetName() == "left") {
				x = p->Get<float>();
			}
			else if (p->GetName() == "top") {
				y = p->Get<float>();
			}
			else if (p->GetName() == "width") {
				w = p->Get<float>();
			}
			else if (p->GetName() == "height") {
				h = p->Get<float>();
			}
			else if (p->GetName() == "textureName") {
				textureName = p->Get<std::string>();
				textureInMemory = true;
			}
			else if (p->GetName() == "textureFileName") {
				textureName = p->Get<std::string>();
			}
		}

		std::shared_ptr<resource::Texture> texture = this->resSystem->Get<resource::Texture>(textureName);

		std::shared_ptr<resource::GLScreenQuad> quad(new resource::GLScreenQuad());

		quad->SetTexture(texture);
		quad->SetPosition(x, y);
		quad->SetSize(w, h);
		quad->InitializeBuffers();

		std::string meshname = "entity";
		meshname += entityID;
		meshname += "screenQuad";

		this->resSystem->Add<resource::Mesh>(meshname, quad);

		renderable->SetMesh(quad);
		renderable->LoadShader("shaders/quad");
		renderable->InitializeBuffers();
		renderable->SetLightingEnabled(false);
		this->screensSpaceComp.push_back(std::unique_ptr<Renderable>(renderable));

		return renderable;
	}

	IComponent* OpenGLSystem::createPointLight(const id_t entityID, const std::vector<Property> &properties) {
		Sigma::PointLight *light = new Sigma::PointLight(entityID);

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &*propitr;
			if (p->GetName() == "x") {
				light->position.x = p->Get<float>();
			}
			else if (p->GetName() == "y") {
				light->position.y = p->Get<float>();
			}
			else if (p->GetName() == "z") {
				light->position.z = p->Get<float>();
			}
			else if (p->GetName() == "intensity") {
				light->intensity = p->Get<float>();
			}
			else if (p->GetName() == "cr") {
				light->color.r = p->Get<float>();
			}
			else if (p->GetName() == "cg") {
				light->color.g = p->Get<float>();
			}
			else if (p->GetName() == "cb") {
				light->color.b = p->Get<float>();
			}
			else if (p->GetName() == "ca") {
				light->color.a = p->Get<float>();
			}
			else if (p->GetName() == "radius") {
				light->radius = p->Get<float>();
			}
			else if (p->GetName() == "falloff") {
				light->falloff = p->Get<float>();
			}
		}

		this->addComponent(entityID, light);
		return light;
	}

	IComponent* OpenGLSystem::createSpotLight(const id_t entityID, const std::vector<Property> &properties) {
		Sigma::SpotLight *light = new Sigma::SpotLight(entityID);

		float x=0.0f, y=0.0f, z=0.0f;
		float rx=0.0f, ry=0.0f, rz=0.0f;

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &*propitr;
			if (p->GetName() == "x") {
				x = p->Get<float>();
			}
			else if (p->GetName() == "y") {
				y = p->Get<float>();
			}
			else if (p->GetName() == "z") {
				z = p->Get<float>();
			}
			else if (p->GetName() == "rx") {
				rx = p->Get<float>();
			}
			else if (p->GetName() == "ry") {
				ry = p->Get<float>();
			}
			else if (p->GetName() == "rz") {
				rz = p->Get<float>();
			}
			else if (p->GetName() == "intensity") {
				light->intensity = p->Get<float>();
			}
			else if (p->GetName() == "cr") {
				light->color.r = p->Get<float>();
			}
			else if (p->GetName() == "cg") {
				light->color.g = p->Get<float>();
			}
			else if (p->GetName() == "cb") {
				light->color.b = p->Get<float>();
			}
			else if (p->GetName() == "ca") {
				light->color.a = p->Get<float>();
			}
			else if (p->GetName() == "innerAngle") {
				light->innerAngle = p->Get<float>();
				light->cosInnerAngle = glm::cos(light->innerAngle);
			}
			else if (p->GetName() == "outerAngle") {
				light->outerAngle = p->Get<float>();
				light->cosOuterAngle = glm::cos(light->outerAngle);
			}
			else if (p->GetName() == "parent") {
				/* Only entities that have ControllableMove component can be parent */
				const id_t parentID = p->Get<int>();
				light->transform.SetParentID(parentID);
			}
		}

		light->transform.TranslateTo(x, y, z);
		light->transform.Rotate(rx, ry, rz);

		this->addComponent(entityID, light);

		return light;
	}

	int OpenGLSystem::createRenderTarget(const unsigned int w, const unsigned int h, bool hasDepth) {
		std::unique_ptr<RenderTarget> newRT(new RenderTarget());

		newRT->width = w;
		newRT->height = h;
		newRT->hasDepth = hasDepth;

		this->renderTargets.push_back(std::move(newRT));
		return (this->renderTargets.size() - 1);
	}

	void OpenGLSystem::initRenderTarget(unsigned int rtID) {
		RenderTarget *rt = this->renderTargets[rtID].get();

		// Make sure we're on the back buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Get backbuffer depth bit width
		int depthBits;
#ifdef __APPLE__
		// The modern way.
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits);
#else
		glGetIntegerv(GL_DEPTH_BITS, &depthBits);
#endif

		// Create the depth render buffer
		if(rt->hasDepth) {
			glGenRenderbuffers(1, &rt->depth_id);
			glBindRenderbuffer(GL_RENDERBUFFER, rt->depth_id);

			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, rt->width, rt->height);
			printOpenGLError();

			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}

		// Create the frame buffer object
		glGenFramebuffers(1, &rt->fbo_id);
		printOpenGLError();

		glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo_id);

		for(unsigned int i=0; i < rt->texture_ids.size(); ++i) {
			//Attach 2D texture to this FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, rt->texture_ids[i], 0);
			printOpenGLError();
		}

		if(rt->hasDepth) {
			//Attach depth buffer to FBO
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth_id);
			printOpenGLError();
		}

		//Does the GPU support current FBO configuration?
		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		switch(status) {
		case GL_FRAMEBUFFER_COMPLETE:
			LOG << "Successfully created render target.";
			break;
		default:
			LOG_ERROR << "Error: Framebuffer format is not compatible.";
			assert (0 && "Error: Framebuffer format is not compatible.");
		}

		// Unbind objects
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLSystem::createRTBuffer(unsigned int rtID, GLint format, GLenum internalFormat, GLenum type) {
		RenderTarget *rt = this->renderTargets[rtID].get();

		// Create a texture for each requested target
		GLuint texture_id;

		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		// Texture params for full screen quad
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//NULL means reserve texture memory, but texels are undefined
		glTexImage2D(GL_TEXTURE_2D, 0, format,
					 (GLsizei)rt->width,
					 (GLsizei)rt->height,
					 0, internalFormat, type, NULL);

		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+rt->texture_ids.size(), GL_TEXTURE_2D, texture_id, 0);

		this->renderTargets[rtID]->texture_ids.push_back(texture_id);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	bool OpenGLSystem::Update(const double delta) {
		this->deltaAccumulator += delta;

		// Check if the deltaAccumulator is greater than 1/<framerate>th of a second.
		//  ..if so, it's time to render a new frame
		if (this->deltaAccumulator > (1.0 / this->framerate)) {

			/////////////////////
			// Rendering Setup //
			/////////////////////

			glm::vec3 viewPosition;
			glm::mat4 viewProjInv;

			// Setup the view matrix and position variables
			glm::mat4 viewMatrix;
			if (this->views.size() > 0) {
				viewMatrix = this->views[this->views.size() - 1]->GetViewMatrix();
				viewPosition = this->views[this->views.size() - 1]->Transform()->GetPosition();
			}

			// Setup the projection matrix
			glm::mat4 viewProj = this->ProjectionMatrix * viewMatrix;

			viewProjInv = glm::inverse(viewProj);

			// Calculate frustum for culling
			this->GetView(0)->CalculateFrustum(viewProj);

			// Clear the backbuffer and primary depth/stencil buffer
			glClearColor(0.0f,0.0f,0.0f,1.0f);
			glViewport(0, 0, this->windowWidth, this->windowHeight); // Set the viewport size to fill the window
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear required buffers

			//////////////////
			// GBuffer Pass //
			//////////////////

			// Bind the first buffer, which is the Geometry Buffer
			if(this->renderTargets.size() > 0) {
				this->renderTargets[0]->BindWrite();
			}

			// Disable blending
			glDisable(GL_BLEND);

			// Clear the GBuffer
			glClearColor(0.0f,0.0f,0.0f,1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear required buffers

			// Loop through and draw each GL Component component.
			for (auto eitr = this->_Components.begin(); eitr != this->_Components.end(); ++eitr) {
				for (auto citr = eitr->second.begin(); citr != eitr->second.end(); ++citr) {
					Renderable *glComp = dynamic_cast<Renderable *>(citr->second.get());

					if(glComp && glComp->IsLightingEnabled()) {
						glComp->GetShader()->Use();

						// Set view position
						//glUniform3f(glGetUniformBlockIndex(glComp->GetShader()->GetProgram(), "viewPosW"), viewPosition.x, viewPosition.y, viewPosition.z);

						// For now, turn on ambient intensity and turn off lighting
						glUniform1f(glGetUniformLocation(glComp->GetShader()->GetProgram(), "ambLightIntensity"), 0.05f);
						glUniform1f(glGetUniformLocation(glComp->GetShader()->GetProgram(), "diffuseLightIntensity"), 0.0f);
						glUniform1f(glGetUniformLocation(glComp->GetShader()->GetProgram(), "specularLightIntensity"), 0.0f);

						glComp->Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);
					}
				}
			}

			// Unbind the first buffer, which is the Geometry Buffer
			if(this->renderTargets.size() > 0) {
				this->renderTargets[0]->UnbindWrite();
			}

			// Copy gbuffer's depth buffer to the screen depth buffer
			// needed for non deferred rendering at the end of this method
			// NOTE: I'm sure there's a faster way to do this
			if(this->renderTargets.size() > 0) {
				this->renderTargets[0]->BindRead();
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, this->windowWidth, this->windowHeight, 0, 0, this->windowWidth, this->windowHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

			if(this->renderTargets.size() > 0) {
				this->renderTargets[0]->UnbindRead();
			}

			///////////////////
			// Lighting Pass //
			///////////////////

			// Disable depth testing
			glDepthFunc(GL_NONE);
			glDepthMask(GL_FALSE);

			// Bind the Geometry buffer for reading
			if(this->renderTargets.size() > 0) {
				this->renderTargets[0]->BindRead();
			}

			// Ambient light pass

			// Ensure that blending is disabled
			glDisable(GL_BLEND);

			// Currently simple constant ambient light, could use SSAO here
			glm::vec4 ambientLight(0.1f, 0.1f, 0.1f, 1.0f);

			GLSLShader &shader = (*this->ambientQuad.GetShader().get());
			shader.Use();

			// Load variables
			glUniform4f(shader("ambientColor"), ambientLight.r, ambientLight.g, ambientLight.b, ambientLight.a);
			glUniform1i(shader("colorBuffer"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[0]);

			this->ambientQuad.Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);

			shader.UnUse();

			// Dynamic light passes
			// Turn on additive blending
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			// Loop through each light, render a fullscreen quad if it is visible
			for(auto eitr = this->_Components.begin(); eitr != this->_Components.end(); ++eitr) {
				for (auto citr = eitr->second.begin(); citr != eitr->second.end(); ++citr) {
					// Check if this component is a point light
					PointLight *light = dynamic_cast<PointLight*>(citr->second.get());

					// If it is a point light, and it intersects the frustum, then render
					if(light && this->GetView(0)->CameraFrustum.intersectsSphere(light->position, light->radius) ) {

						GLSLShader &shader = (*this->pointQuad.GetShader().get());
						shader.Use();

						// Load variables
						glUniform3fv(shader("viewPosW"), 1, &viewPosition[0]);
						glUniformMatrix4fv(shader("viewProjInverse"), 1, false, &viewProjInv[0][0]);
						glUniform3fv(shader("lightPosW"), 1, &light->position[0]);
						glUniform1f(shader("lightRadius"), light->radius);
						glUniform4fv(shader("lightColor"), 1, &light->color[0]);

						glUniform1i(shader("diffuseBuffer"), 0);
						glUniform1i(shader("normalBuffer"), 1);
						glUniform1i(shader("depthBuffer"), 2);

						// Bind GBuffer textures
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[0]);
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[1]);
						glActiveTexture(GL_TEXTURE2);
						glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[2]);

						this->pointQuad.Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);

						shader.UnUse();

						continue;
					}

					SpotLight *spotLight = dynamic_cast<SpotLight *>(citr->second.get());

					if(spotLight && spotLight->IsEnabled()) {
						GLSLShader &shader = (*this->spotQuad.GetShader().get());
						shader.Use();

						glm::vec3 position = spotLight->transform.ExtractPosition();
						glm::vec3 direction = spotLight->transform.GetForward();

						// Load variables
						glUniform3fv(shader("viewPosW"), 1, &viewPosition[0]);
						glUniformMatrix4fv(shader("viewProjInverse"), 1, false, &viewProjInv[0][0]);
						glUniform3fv(shader("lightPosW"), 1, &position[0]);
						glUniform3fv(shader("lightDirW"), 1, &direction[0]);
						glUniform4fv(shader("lightColor"), 1, &spotLight->color[0]);
						glUniform1f(shader("lightCosInnerAngle"), spotLight->cosInnerAngle);
						glUniform1f(shader("lightCosOuterAngle"), spotLight->cosOuterAngle);

						glUniform1i(shader("diffuseBuffer"), 0);
						glUniform1i(shader("normalBuffer"), 1);
						glUniform1i(shader("depthBuffer"), 2);

						// Bind GBuffer textures
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[0]);
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[1]);
						glActiveTexture(GL_TEXTURE2);
						glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[2]);

						this->spotQuad.Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);

						shader.UnUse();

						continue;
					}
				}
			}

			// Unbind the Geometry buffer for reading
			if(this->renderTargets.size() > 0) {
				this->renderTargets[0]->UnbindRead();
			}

			// Remove blending
			glDisable(GL_BLEND);

			// Re-enabled depth test
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);

			////////////////////
			// Composite Pass //
			////////////////////

			// Not needed yet

			///////////////////////
			// Draw Unlit Objects
			///////////////////////

			// Loop through and draw each GL Component component.
			for (auto eitr = this->_Components.begin(); eitr != this->_Components.end(); ++eitr) {
				for (auto citr = eitr->second.begin(); citr != eitr->second.end(); ++citr) {
					Renderable *glComp = dynamic_cast<Renderable *>(citr->second.get());

					if(glComp && !glComp->IsLightingEnabled()) {
						glComp->GetShader()->Use();

						// Set view position
						glUniform3f(glGetUniformBlockIndex(glComp->GetShader()->GetProgram(), "viewPosW"), viewPosition.x, viewPosition.y, viewPosition.z);

						glComp->Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);
					}
				}
			}

			//////////////////
			// Overlay Pass //
			//////////////////

			// Enable transparent rendering
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			for (auto citr = this->screensSpaceComp.begin(); citr != this->screensSpaceComp.end(); ++citr) {
				citr->get()->Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);
			}

			// Remove blending
			glDisable(GL_BLEND);

			// Unbind frame buffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			this->deltaAccumulator = 0.0;
			return true;
		}
		return false;
	}

	GLTransform *OpenGLSystem::GetTransformFor(const unsigned int entityID) {
		auto entity = &(_Components[entityID]);

		// for now, just returns the first component's transform
		// bigger question: should entities be able to have multiple GLComponents?
		for(auto compItr = entity->begin(); compItr != entity->end(); compItr++) {
			Renderable *glComp = dynamic_cast<Renderable *>((*compItr).second.get());
			if(glComp) {
				GLTransform *transform = glComp->Transform();
				return transform;
			}
		}

		// no GL components
		return 0;
	}

	const int* OpenGLSystem::Start() {
		// Use the GL3 way to get the version number
		glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
		glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);

		// Sanity check to make sure we are at least in a good major version number.
		assert((OpenGLVersion[0] > 1) && (OpenGLVersion[0] < 5));

		// Determine the aspect ratio and sanity check it to a safe ratio
		float aspectRatio = static_cast<float>(this->windowWidth) / static_cast<float>(this->windowHeight);
		if (aspectRatio < 1.0f) {
			aspectRatio = 4.0f / 3.0f;
		}

		// Generate a projection matrix (the "view") based on basic window dimensions
		this->ProjectionMatrix = glm::perspective(
			45.0f, // field-of-view (height)
			aspectRatio, // aspect ratio
			0.1f, // near culling plane
			10000.0f // far culling plane
			);

		// App specific global gl settings
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
#if __APPLE__
		// GL_TEXTURE_CUBE_MAP_SEAMLESS and GL_MULTISAMPLE are Core.
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // allows for cube-mapping without seams
		glEnable(GL_MULTISAMPLE);
#else
		if (GLEW_AMD_seamless_cubemap_per_texture) {
			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // allows for cube-mapping without seams
		}
		if (GLEW_ARB_multisample) {
			glEnable(GL_MULTISAMPLE_ARB);
		}
#endif
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);

		// Setup a screen quad for deferred rendering
		std::shared_ptr<resource::GLScreenQuad> pQuad(new resource::GLScreenQuad());
		pQuad->SetPosition(0.0f, 0.0f);
		pQuad->SetSize(1.0f, 1.0f);
		pQuad->Inverted(true);
		pQuad->InitializeBuffers();
		std::string meshname = "pointQuad";

		this->resSystem->Add<resource::Mesh>(meshname, pQuad);

		this->pointQuad.SetMesh(pQuad);

		this->pointQuad.LoadShader("shaders/pointlight");
		this->pointQuad.InitializeBuffers();
		this->pointQuad.SetCullFace("none");

		this->pointQuad.GetShader()->Use();
		this->pointQuad.GetShader()->AddUniform("viewPosW");
		this->pointQuad.GetShader()->AddUniform("viewProjInverse");
		this->pointQuad.GetShader()->AddUniform("lightPosW");
		this->pointQuad.GetShader()->AddUniform("lightRadius");
		this->pointQuad.GetShader()->AddUniform("lightColor");
		this->pointQuad.GetShader()->AddUniform("diffuseBuffer");
		this->pointQuad.GetShader()->AddUniform("normalBuffer");
		this->pointQuad.GetShader()->AddUniform("depthBuffer");
		this->pointQuad.GetShader()->UnUse();

		std::shared_ptr<resource::GLScreenQuad> sQuad(new resource::GLScreenQuad());
		sQuad->SetPosition(0.0f, 0.0f);
		sQuad->SetSize(1.0f, 1.0f);
		sQuad->Inverted(true);
		sQuad->InitializeBuffers();
		meshname = "spotQuad";

		this->resSystem->Add<resource::Mesh>(meshname, sQuad);

		this->spotQuad.SetMesh(sQuad);

		this->spotQuad.LoadShader("shaders/spotlight");
		this->spotQuad.InitializeBuffers();
		this->spotQuad.SetCullFace("none");

		this->spotQuad.GetShader()->Use();
		this->spotQuad.GetShader()->AddUniform("viewPosW");
		this->spotQuad.GetShader()->AddUniform("viewProjInverse");
		this->spotQuad.GetShader()->AddUniform("lightPosW");
		this->spotQuad.GetShader()->AddUniform("lightDirW");
		this->spotQuad.GetShader()->AddUniform("lightColor");
		this->spotQuad.GetShader()->AddUniform("lightCosInnerAngle");
		this->spotQuad.GetShader()->AddUniform("lightCosOuterAngle");
		this->spotQuad.GetShader()->AddUniform("diffuseBuffer");
		this->spotQuad.GetShader()->AddUniform("normalBuffer");
		this->spotQuad.GetShader()->AddUniform("depthBuffer");
		this->spotQuad.GetShader()->UnUse();

		std::shared_ptr<resource::GLScreenQuad> aQuad(new resource::GLScreenQuad());
		aQuad->SetPosition(0.0f, 0.0f);
		aQuad->SetSize(1.0f, 1.0f);
		aQuad->Inverted(true);
		aQuad->InitializeBuffers();
		meshname = "ambientQuad";

		this->resSystem->Add<resource::Mesh>(meshname, aQuad);

		this->ambientQuad.SetMesh(aQuad);
		this->ambientQuad.LoadShader("shaders/ambient");
		this->ambientQuad.InitializeBuffers();
		this->ambientQuad.SetCullFace("none");

		this->ambientQuad.GetShader()->Use();
		this->ambientQuad.GetShader()->AddUniform("ambientColor");
		this->ambientQuad.GetShader()->AddUniform("colorBuffer");
		this->ambientQuad.GetShader()->UnUse();

		return OpenGLVersion;
	}

	void OpenGLSystem::SetViewportSize(const unsigned int width, const unsigned int height) {
		this->windowHeight = height;
		this->windowWidth = width;

		// Determine the aspect ratio and sanity check it to a safe ratio
		float aspectRatio = static_cast<float>(this->windowWidth) / static_cast<float>(this->windowHeight);
		if (aspectRatio < 1.0f) {
			aspectRatio = 4.0f / 3.0f;
		}

		// update projection matrix based on new aspect ratio
		this->ProjectionMatrix = glm::perspective(
			45.0f,
			aspectRatio,
			0.1f,
			10000.0f
			);
	}

} // namespace Sigma

//-----------------------------------------------------------------
// Print for OpenGL errors
//
// Returns 1 if an OpenGL error occurred, 0 otherwise.
//

int printOglError(const std::string &file, int line) {
	GLenum glErr;
	int retCode = 0;

	glErr = glGetError();
	if (glErr != GL_NO_ERROR) {
		LOG_ERROR << "glError in file " << file << " @ line " << line << ": " << gluErrorString(glErr);
		retCode = 1;
	}
	return retCode;
}
