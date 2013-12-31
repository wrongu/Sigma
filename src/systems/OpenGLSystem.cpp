#include "systems/OpenGLSystem.h"
#include "systems/GLSLShader.h"
#include "systems/GLSixDOFView.h"
#include "controllers/FPSCamera.h"
#include "components/GLSprite.h"
#include "components/GLIcoSphere.h"
#include "components/GLCubeSphere.h"
#include "components/GLMesh.h"
#include "components/GLScreenQuad.h"

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
	// RenderTarget methods
	RenderTarget::RenderTarget(const int w, const int h, const bool depth) : width(w), height(h), hasDepth(depth)
	{
		this->InitBuffers();
	}

	RenderTarget::~RenderTarget() {
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
				std::cout << "Successfully created render target.";
				break;
			default:
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
		std::cout << "Depth buffer bits: " << depthBits << std::endl;

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

			// Configure texture params for full screen quad
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

			//Does the GPU support current FBO configuration?
			GLenum status;
			status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

			switch(status) {
				case GL_FRAMEBUFFER_COMPLETE:
					std::cout << "Successfully created render target." << std::endl;
					break;
				default:
					assert(0 && "Error: Framebuffer format is not compatible.");
			}

			// reset fbo and texture bindings
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

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

	std::map<std::string, Sigma::resource::GLTexture> OpenGLSystem::textures;

	OpenGLSystem::OpenGLSystem() : windowWidth(1024), windowHeight(768), deltaAccumulator(0.0),
		framerate(60.0f), pointQuad(1000), ambientQuad(1001), spotQuad(1002), viewMode("") {}

	//std::map<std::string, resource::GLTexture> OpenGLSystem::textures = std::map<std::string, resource::GLTexture>();

	std::map<std::string, Sigma::IFactory::FactoryFunction>
		OpenGLSystem::getFactoryFunctions() {
		using namespace std::placeholders;

		std::map<std::string, Sigma::IFactory::FactoryFunction> retval;
		retval["GLSprite"] = std::bind(&OpenGLSystem::createGLSprite,this,_1,_2);
		retval["GLIcoSphere"] = std::bind(&OpenGLSystem::createGLIcoSphere,this,_1,_2);
		retval["GLCubeSphere"] = std::bind(&OpenGLSystem::createGLCubeSphere,this,_1,_2);
		retval["GLMesh"] = std::bind(&OpenGLSystem::createGLMesh,this,_1,_2);
		retval["FPSCamera"] = std::bind(&OpenGLSystem::createGLView,this,_1,_2, "FPSCamera");
		retval["GLSixDOFView"] = std::bind(&OpenGLSystem::createGLView,this,_1,_2, "GLSixDOFView");
		retval["PointLight"] = std::bind(&OpenGLSystem::createPointLight,this,_1,_2);
		retval["SpotLight"] = std::bind(&OpenGLSystem::createSpotLight,this,_1,_2);
		retval["GLScreenQuad"] = std::bind(&OpenGLSystem::createScreenQuad,this,_1,_2);

		return retval;
	}

	IComponent* OpenGLSystem::createGLView(const unsigned int entityID, const std::vector<Property> &properties, std::string mode) {
		viewMode = mode;

		if(mode=="FPSCamera") {
			this->views.push_back(new Sigma::event::handler::FPSCamera(entityID));
		}
		else if(mode=="GLSixDOFView") {
			this->views.push_back(new GLSixDOFView(entityID));
		}
		else {
			std::cerr << "Invalid view type!" << std::endl;
			return nullptr;
		}

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

		this->views[this->views.size() - 1]->Transform()->TranslateTo(x,y,z);
		this->views[this->views.size() - 1]->Transform()->Rotate(rx,ry,rz);

		this->addComponent(entityID, this->views[this->views.size() - 1]);

		return this->views[this->views.size() - 1];
	}

	IComponent* OpenGLSystem::createGLSprite(const unsigned int entityID, const std::vector<Property> &properties) {
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

		// Check if the texture is loaded and load it if not.
		if (textures.find(textureFilename) == textures.end()) {
			Sigma::resource::GLTexture texture;
			texture.LoadDataFromFile(textureFilename);
			if (texture.GetID() != 0) {
				Sigma::OpenGLSystem::textures[textureFilename] = texture;
			}
		}

		// It should be loaded, but in case an error occurred double check for it.
		if (textures.find(textureFilename) != textures.end()) {
			spr->SetTexture(&Sigma::OpenGLSystem::textures[textureFilename]);
		}
		spr->LoadShader();
		spr->Transform()->Scale(glm::vec3(scale));
		spr->Transform()->Translate(x,y,z);
		spr->InitializeBuffers();
		this->addComponent(entityID,spr);
		return spr;
	}

	IComponent* OpenGLSystem::createGLIcoSphere(const unsigned int entityID, const std::vector<Property> &properties) {
		Sigma::GLIcoSphere* sphere = new Sigma::GLIcoSphere(entityID);
		float scale = 1.0f;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		int componentID = 0;
		std::string shader_name = "shaders/icosphere";

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
				sphere->SetLightingEnabled(p->Get<bool>());
			}
		}
		sphere->Transform()->Scale(scale,scale,scale);
		sphere->Transform()->Translate(x,y,z);
		sphere->LoadShader(shader_name);
		sphere->InitializeBuffers();
		sphere->SetCullFace("back");
		this->addComponent(entityID,sphere);
		return sphere;
	}

	IComponent* OpenGLSystem::createGLCubeSphere(const unsigned int entityID, const std::vector<Property> &properties) {
		Sigma::GLCubeSphere* sphere = new Sigma::GLCubeSphere(entityID);

		std::string texture_name = "";
		std::string shader_name = "shaders/cubesphere";
		std::string cull_face = "back";
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
				sphere->SetLightingEnabled(p->Get<bool>());
			}
		}

		sphere->SetSubdivisions(subdivision_levels);
		sphere->SetFixToCamera(fix_to_camera);
		sphere->SetCullFace(cull_face);
		sphere->Transform()->Scale(scale,scale,scale);
		sphere->Transform()->Rotate(rx,ry,rz);
		sphere->Transform()->Translate(x,y,z);
		sphere->LoadShader(shader_name);
		sphere->LoadTexture(texture_name);
		sphere->InitializeBuffers();

		this->addComponent(entityID,sphere);
		return sphere;
	}

	IComponent* OpenGLSystem::createGLMesh(const unsigned int entityID, const std::vector<Property> &properties) {
		Sigma::GLMesh* mesh = new Sigma::GLMesh(entityID);

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
		std::string meshName = "";

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
				meshName = p->Get<std::string>();
				std::cerr << "Loading mesh: " << meshName << std::endl;
				mesh->LoadMesh(meshName);
			}
			else if (p->GetName() == "shader") {
				shaderfile = p->Get<std::string>();
			}
			else if (p->GetName() == "id") {
				componentID = p->Get<int>();
			}
			else if (p->GetName() == "cullface") {
				cull_face = p->Get<std::string>();
			}
			else if (p->GetName() == "lightEnabled") {
				mesh->SetLightingEnabled(p->Get<bool>());
			}
			else if (p->GetName() == "parent") {
				/* Right now hacky, only GLMesh and FPSCamera are supported as parents */

				int parentID = p->Get<int>();
				SpatialComponent *comp = dynamic_cast<SpatialComponent *>(this->getComponent(parentID, Sigma::GLMesh::getStaticComponentTypeName()));

				if(comp) {
					mesh->Transform()->SetParentTransform(comp->Transform());
				} else {
					comp = dynamic_cast<SpatialComponent *>(this->getComponent(parentID, Sigma::event::handler::FPSCamera::getStaticComponentTypeName()));

					if(comp) {
						mesh->Transform()->SetParentTransform(comp->Transform());
					}
				}
			}
		}

		mesh->SetCullFace(cull_face);
		mesh->Transform()->Scale(scale,scale,scale);
		mesh->Transform()->Translate(x,y,z);
		mesh->Transform()->Rotate(rx,ry,rz);
		if(shaderfile != "") {
			mesh->LoadShader(shaderfile);
			if(mesh->IsLightingEnabled() && shaderfile != GLMesh::DEFAULT_SHADER){
				std::cerr << "WARNING (" << meshName
					<< "): mesh lighting will only work well using the shader '"
					<< GLMesh::DEFAULT_SHADER << "', but '" << shaderfile
					<< "' was specified." << std::endl;
			}
		}
		else {
			mesh->LoadShader(); // load default
		}
		mesh->InitializeBuffers();
		this->addComponent(entityID,mesh);
		return mesh;
	}

	IComponent* OpenGLSystem::createScreenQuad(const unsigned int entityID, const std::vector<Property> &properties) {
		Sigma::GLScreenQuad* quad = new Sigma::GLScreenQuad(entityID);

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

		// Check if the texture is loaded and load it if not.
		if (textures.find(textureName) == textures.end()) {
			Sigma::resource::GLTexture texture;
			if (textureInMemory) { // We are using an in memory texture. It will be populated somewhere else
				Sigma::OpenGLSystem::textures[textureName] = texture;
			}
			else { // The texture in on disk so load it.
				texture.LoadDataFromFile(textureName);
				if (texture.GetID() != 0) {
					Sigma::OpenGLSystem::textures[textureName] = texture;
				}
			}
		}

		// It should be loaded, but in case an error occurred double check for it.
		if (textures.find(textureName) != textures.end()) {
			quad->SetTexture(&Sigma::OpenGLSystem::textures[textureName]);
		}

		quad->SetPosition(x, y);
		quad->SetSize(w, h);
		quad->LoadShader("shaders/quad");
		quad->InitializeBuffers();
		this->screensSpaceComp.push_back(std::unique_ptr<IGLComponent>(quad));

		return quad;
	}

	IComponent* OpenGLSystem::createPointLight(const unsigned int entityID, const std::vector<Property> &properties) {
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

	IComponent* OpenGLSystem::createSpotLight(const unsigned int entityID, const std::vector<Property> &properties) {
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
			else if (p->GetName() == "angle") {
				light->angle = p->Get<float>();
				light->cosCutoff = glm::cos(light->angle);
			}
			else if (p->GetName() == "exponent") {
				light->exponent = p->Get<float>();
			}
			else if (p->GetName() == "parent") {
				/* Right now hacky, only GLMesh and FPSCamera are supported as parents */
				int parentID = p->Get<int>();
				SpatialComponent *comp = dynamic_cast<SpatialComponent *>(this->getComponent(parentID, Sigma::GLMesh::getStaticComponentTypeName()));

				if(comp) {
					light->transform.SetParentTransform(comp->Transform());
				} else {
					comp = dynamic_cast<SpatialComponent *>(this->getComponent(parentID, Sigma::event::handler::FPSCamera::getStaticComponentTypeName()));

					if(comp) {
						light->transform.SetParentTransform(comp->Transform());
					}
				}
			}
		}

		light->transform.TranslateTo(x, y, z);
		light->transform.Rotate(rx, ry, rz);

		this->addComponent(entityID, light);

		return light;
	}

	int OpenGLSystem::createRenderTarget(const unsigned int w, const unsigned int h, const bool hasDepth) {
		std::unique_ptr<RenderTarget> newRT(new RenderTarget(w, h, hasDepth));
		this->renderTargets.push_back(std::move(newRT));
		return (this->renderTargets.size() - 1);
	}

	void OpenGLSystem::createRTBuffer(unsigned int rtID, GLint format, GLenum internalFormat, GLenum type) {
		RenderTarget *rt = this->renderTargets[rtID].get();
		rt->CreateTexture(format, internalFormat, type);
	}

	void OpenGLSystem::_RenderClearAll(){
		// Clear the backbuffer and primary depth/stencil buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f,0.0f,0.0f,1.0f);
		glViewport(0, 0, this->windowWidth, this->windowHeight); // Set the viewport size to fill the window
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear required buffers

		// Clear the GBuffer
		glBindFramebuffer(GL_FRAMEBUFFER, this->renderTargets[0]->fbo_id);
		glClearColor(0.0f,0.0f,0.0f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear required buffers
	}

	void OpenGLSystem::_RenderGBuffer(glm::mat4 &viewMatrix){
		// Bind the first buffer, which is the Geometry Buffer
		if(this->renderTargets.size() > 0) {
			this->renderTargets[0]->BindWrite();
		}

		// Loop through and draw each GL Component component.
		for (auto eitr = this->_Components.begin(); eitr != this->_Components.end(); ++eitr) {
			for (auto citr = eitr->second.begin(); citr != eitr->second.end(); ++citr) {
				IGLComponent *glComp = dynamic_cast<IGLComponent *>(citr->second.get());
				// only output geometry to g-buffer if lighting enabled
				// (this means that other objects are effectively transparent)
				if(glComp && glComp->IsLightingEnabled()) {
					GLSLShader &shader = *glComp->GetShader();

					// TODO
					// As this is written, uniforms are set once per object per frame.
					// They *can* be set only once, ever, since once a uniform is set
					// it persists until the program is relinked, and we require all
					// lit objects to use the same shader program at the moment. So
					// they could all just share the same uniform values (set once)
					shader.Use();
					{
						// For now, turn on ambient intensity and turn off lighting
						glUniform1f(shader("ambLightIntensity"), 0.05f);
						glUniform1f(shader("diffuseLightIntensity"), 0.0f);
						glUniform1f(shader("specularLightIntensity"), 0.0f);

						glComp->Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);
					}
					shader.UnUse();
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
		glBlitFramebuffer(0, 0, this->windowWidth, this->windowHeight, 0, 0, this->windowWidth, this->windowHeight,
			  GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		if(this->renderTargets.size() > 0) {
			this->renderTargets[0]->UnbindRead();
		}
	}

	void OpenGLSystem::_RenderAmbient(){
		// Currently simple constant ambient light, could use SSAO here
		glm::vec4 ambientLight(0.1f, 0.1f, 0.1f, 1.0f);

		GLSLShader &shader = (*this->ambientQuad.GetShader().get());
		shader.Use();
		{
			// Load variables
			glUniform4f(shader("ambientColor"), ambientLight.r, ambientLight.g, ambientLight.b, ambientLight.a);

			glUniform1i(shader("colorBuffer"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[0]);

			this->ambientQuad.Render();
		}
		shader.UnUse();
	}

	void OpenGLSystem::_RenderPointLight(PointLight *light, glm::mat4 &viewMatrix, glm::mat4 &viewProjInv, glm::vec3 &viewPosition){
		GLSLShader &shader = (*this->pointQuad.GetShader().get());
		shader.Use();
		{
			// Load variables
			glUniform3fv(shader("viewPosW"), 3, &viewPosition[0]);
			glUniformMatrix4fv(shader("viewProjInverse"), 1, false, &viewProjInv[0][0]);
			glUniform3fv(shader("lightPosW"), 1, &light->position[0]);
			glUniform1f(shader("lightRadius"), light->radius);
			glUniform4fv(shader("lightColor"), 1, &light->color[0]);

			// TODO set these once when the shader is created, since they are constant
			glUniform1i(shader("diffuseBuffer"), 0);
			glUniform1i(shader("normalBuffer"), 1);
			glUniform1i(shader("depthBuffer"), 2);

			// Bind GBuffer textures
			// texture 0 is ambient color
			// texture 1 is normals
			// texture 2 is z-depth
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[1]);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[2]);

			this->pointQuad.Render();
		}
		shader.UnUse();
	}

	void OpenGLSystem::_RenderSpotLight(SpotLight *light, glm::mat4 &viewMatrix, glm::mat4 &viewProjInv){
		GLSLShader &shader = (*this->spotQuad.GetShader().get());
		shader.Use();
		{
			glm::vec3 position = light->transform.ExtractPosition();
			glm::vec3 direction = light->transform.GetForward();

			// Load variables
			glUniformMatrix4fv(shader("viewProjInverse"), 1, false, &viewProjInv[0][0]);
			glUniform3fv(shader("lightPosW"), 1, &position[0]);
			glUniform3fv(shader("lightDirW"), 1, &direction[0]);
			glUniform4fv(shader("lightColor"), 1, &light->color[0]);
			glUniform1f(shader("lightAngle"), light->angle);
			glUniform1f(shader("lightCosCutoff"), light->cosCutoff);
			glUniform1f(shader("lightExponent"), light->exponent);

			glUniform1i(shader("diffuseBuffer"), 0);
			glUniform1i(shader("normalBuffer"), 1);
			glUniform1i(shader("depthBuffer"), 2);

			// Bind GBuffer textures
			// texture 0 is ambient color
			// texture 1 is normals
			// texture 2 is z-depth
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[1]);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, this->renderTargets[0]->texture_ids[2]);

			this->spotQuad.Render();
		}
		shader.UnUse();
	}

	void OpenGLSystem::_RenderUnlit(glm::mat4 &viewMatrix, glm::vec3 &viewPosition){
		// Loop through and draw each GL Component.
		for (auto eitr = this->_Components.begin(); eitr != this->_Components.end(); ++eitr) {
			for (auto citr = eitr->second.begin(); citr != eitr->second.end(); ++citr) {
				IGLComponent *glComp = dynamic_cast<IGLComponent *>(citr->second.get());
				if(glComp && !glComp->IsLightingEnabled()) {
					GLSLShader &shader = *glComp->GetShader();
					shader.Use();
					// Set view position
					glUniform3f(glGetUniformBlockIndex(glComp->GetShader()->GetProgram(), "viewPosW"), viewPosition.x, viewPosition.y, viewPosition.z);
					glComp->Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);
				}
			}
		}
	}

	void OpenGLSystem::_RenderOverlays(glm::mat4 &viewMatrix){
		// Enable transparent rendering
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (auto citr = this->screensSpaceComp.begin(); citr != this->screensSpaceComp.end(); ++citr) {
				citr->get()->GetShader()->Use();

				// For now, turn on ambient intensity and turn off lighting
				glUniform1f(glGetUniformLocation(citr->get()->GetShader()->GetProgram(), "ambLightIntensity"), 0.05f);
				glUniform1f(glGetUniformLocation(citr->get()->GetShader()->GetProgram(), "diffuseLightIntensity"), 0.0f);
				glUniform1f(glGetUniformLocation(citr->get()->GetShader()->GetProgram(), "specularLightIntensity"), 0.0f);
				citr->get()->Render(&viewMatrix[0][0], &this->ProjectionMatrix[0][0]);
		}

		// Remove blending
		glDisable(GL_BLEND);
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
			glm::mat4 viewMatrix, viewProj, viewProjInv;

			// Setup the view matrix and position variables
			if (this->views.size() > 0) {
				viewMatrix = this->views[this->views.size() - 1]->GetViewMatrix();
				viewPosition = this->views[this->views.size() - 1]->Transform()->GetPosition();
			}

			// Setup the projection matrix
			viewProj = glm::mul(this->ProjectionMatrix, viewMatrix);

			viewProjInv = glm::inverse(viewProj);

			// Calculate frustum for culling
			this->GetView(0)->CalculateFrustum(viewProj);

			/////////////////////
			// Render G-Buffer //
			/////////////////////

			this->_RenderClearAll();

			// Disable blending since the GBuffer is outputting geometry, not summing light
			glDisable(GL_BLEND);
			{
				this->_RenderGBuffer(viewMatrix);
			}
			// Turn on additive blending for the lighting passes
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			///////////////////
			// Lighting Pass //
			///////////////////

			// Bind the Geometry buffer for reading
			if(this->renderTargets.size() > 0) {
				this->renderTargets[0]->BindRead();
			}

			// Disable depth testing since all further operations are full-screen quads
			glDepthFunc(GL_NONE);
			glDepthMask(GL_FALSE);

			this->_RenderAmbient();

			// Dynamic light passes

			// Loop through each light, render a fullscreen quad if it is visible
			// TODO - define a spherical mesh. Use it to bound each light.
			//	Then, by rendering just this mesh, it only processes pixels affected by
			//	each light (plus some extra culling logic for this bounding mesh)
			for(auto eitr = this->_Components.begin(); eitr != this->_Components.end(); ++eitr) {
				for (auto citr = eitr->second.begin(); citr != eitr->second.end(); ++citr) {

					// Check if this component is a point light
					PointLight *light = dynamic_cast<PointLight*>(citr->second.get());
					// If it is a point light, and it intersects the frustum, then render
					if(light && this->GetView(0)->CameraFrustum.isectSphere(light->position, light->radius) ) {
						this->_RenderPointLight(light, viewMatrix, viewProjInv, viewPosition);
						continue;
					}

                    // Check if this component is a spot light
					SpotLight *spotLight = dynamic_cast<SpotLight *>(citr->second.get());
					if(spotLight && spotLight->IsEnabled()) {
						this->_RenderSpotLight(spotLight, viewMatrix, viewProjInv);
						continue;
					}
				}
			}

			// Deferred rendering is done.
			// Unbind the Geometry buffer
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

			this->_RenderUnlit(viewMatrix, viewPosition);

			//////////////////
			// Overlay Pass //
			//////////////////

			this->_RenderOverlays(viewMatrix);

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
			IGLComponent *glComp = dynamic_cast<IGLComponent *>((*compItr).second.get());
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
		this->pointQuad.SetSize(1.0f, 1.0f);
		this->pointQuad.SetPosition(0.0f, 0.0f);
		this->pointQuad.LoadShader("shaders/pointlight");
		this->pointQuad.Inverted(true);
		this->pointQuad.InitializeBuffers();
		this->pointQuad.SetCullFace("none");

		this->spotQuad.SetSize(1.0f, 1.0f);
		this->spotQuad.SetPosition(0.0f, 0.0f);
		this->spotQuad.LoadShader("shaders/spotlight");
		this->spotQuad.Inverted(true);
		this->spotQuad.InitializeBuffers();
		this->spotQuad.SetCullFace("none");

		this->ambientQuad.SetSize(1.0f, 1.0f);
		this->ambientQuad.SetPosition(0.0f, 0.0f);
		this->ambientQuad.LoadShader("shaders/ambient");
		this->ambientQuad.Inverted(true);
		this->ambientQuad.InitializeBuffers();
		this->ambientQuad.SetCullFace("none");

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

int printOglError(const std::string &file, int line)
{

	GLenum glErr;
	int    retCode = 0;

	glErr = glGetError();
	if (glErr != GL_NO_ERROR)
	{
		std::cerr << "glError in file " << file << " @ line " << line << ": " << gluErrorString(glErr) << std::endl;
		retCode = 1;
	}
	return retCode;
}
