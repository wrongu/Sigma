#pragma once
#ifndef OPENGLSYSTEM_H
#define OPENGLSYSTEM_H

#include "Property.h"

#ifndef __APPLE__
#include "GL/glew.h"
#endif
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include <memory>

#include "IFactory.h"
#include "ISystem.h"
#include "components/Renderable.h"
#include "systems/opengl/IGLView.h"
#include "systems/opengl/RenderTarget.h"
#include <vector>
#include "resources/Texture.h"
#include "resources/GLScreenQuad.h"
#include "Sigma.h"
#include "composites/PhysicalWorldLocation.h"
#include "ResourceSsytem.h"

struct IGLView;

int printOglError(const std::string &file, int line);
#define printOpenGLError() printOglError(__FILE__, __LINE__)

namespace Sigma{

	class OpenGLSystem : public IFactory, public ISystem<IComponent> {
	public:

		DLL_EXPORT OpenGLSystem();

		/**
		 * \brief Starts the OpenGL rendering system.
		 *
		 * Starts the OpenGL rendering system and creates a rendering context. It will also attempt to create a newer rendering context (>3) if available.
		 * \return -1 in the 0 index on failure, else the major and minor version in index 0 and 1 respectively.
		 */
		DLL_EXPORT const int* Start();

		/**
		 * \brief Causes an update in the system based on the change in time.
		 *
		 * Updates the state of the system based off how much time has elapsed since the last update.
		 * \param delta the time (in seconds) since the last update
		 * \return true if rendering was performed
		 */
		DLL_EXPORT bool Update(const double delta);

		/**
		 * \brief Sets the window width and height for glViewport
		 *
		 * \param width new width for the window
		 * \param height new height for the window
		 * \return void
		 */
		DLL_EXPORT void SetWindowDim(int width, int height) { this->windowWidth = width; this->windowHeight = height; }

		/**
		 * \brief Sets the viewport width and height.
		 *
		 * \param width new viewport width
		 * \param height new viewport height
		 */
		DLL_EXPORT void SetViewportSize(const unsigned int width, const unsigned int height);

		/**
		 * \brief set the framerate at runtime
		 *
		 *  Note that the constructor sets a default of 60fps
		 */
		DLL_EXPORT void SetFrameRate(double fr) { this->framerate = fr; }

		std::map<std::string,FactoryFunction> getFactoryFunctions();

		DLL_EXPORT IComponent* createPointLight(const id_t entityID, const std::vector<Property> &properties);
		DLL_EXPORT IComponent* createSpotLight(const id_t entityID, const std::vector<Property> &properties);
		DLL_EXPORT IComponent* createScreenQuad(const id_t entityID, const std::vector<Property> &properties);

		// TODO: Move these methods to the components themselves.
		DLL_EXPORT IComponent* createGLSprite(const id_t entityID, const std::vector<Property> &properties) ;
		DLL_EXPORT IComponent* createGLIcoSphere(const id_t entityID, const std::vector<Property> &properties) ;
		DLL_EXPORT IComponent* createGLCubeSphere(const id_t entityID, const std::vector<Property> &properties) ;
		DLL_EXPORT IComponent* createGLMesh(const id_t entityID, const std::vector<Property> &properties) ;
		// Views are not technically components, but perhaps they should be
		DLL_EXPORT IComponent* createGLView(const id_t entityID, const std::vector<Property> &properties) ;

		// Managing rendering internals
		/*
		 * \brief creates a new render target of desired size
		 */
		DLL_EXPORT int createRenderTarget(const unsigned int w, const unsigned int h, bool hasDepth);

		/*
		 * \brief returns the fbo_id of primary render target (index 0)
		 */
		DLL_EXPORT int getRenderTarget(unsigned int rtID) { return (this->renderTargets.size() > rtID) ? this->renderTargets[rtID]->fbo_id : -1; }
		DLL_EXPORT int getRenderTexture(const unsigned int target=0) { return (this->renderTargets.size() > 0) ? this->renderTargets[0]->texture_ids[target] : -1; }
		DLL_EXPORT void createRTBuffer(unsigned int rtID, GLint format, GLenum internalFormat, GLenum type);
		DLL_EXPORT void initRenderTarget(unsigned int rtID);

		// Rendering methods
		DLL_EXPORT void RenderTexture(GLuint texture_id);

		/**
		 * \brief Gets the specified view.
		 *
		 * \param unsigned int index The index of the view to retrieve.
		 * \return IGLView* The specified view.
		 */
		DLL_EXPORT IGLView* GetView(unsigned int index = 0) const {
			if (index >= this->views.size()) {
				//return this->views[this->views.size() - 1];
				return 0;
			}
			return this->views[index];
		}

		/**
		 * \brief Adds a view to the stack.
		 *
		 * \param[in] IGLView * view The view to add to the stack.
		 * \return void
		 */
		DLL_EXPORT void PushView(IGLView* view) {
			this->views.push_back(view);
		}

		/**
		 * \brief Pops a view from the stack.
		 *
		 * Pops a view from the stack and deletes it.
		 * \return void
		 */
		DLL_EXPORT void PopView() {
			if (this->views.size() > 0) {
				delete this->views[this->views.size() - 1];
				this->views.pop_back();
			}
		}

		DLL_EXPORT GLTransform* GetTransformFor(const unsigned int entityID);
	private:
		unsigned int windowWidth; // Store the width of our window
		unsigned int windowHeight; // Store the height of our window

		int OpenGLVersion[2];

		// Scene matrices
		glm::mat4 ProjectionMatrix;
		std::vector<IGLView*> views; // A stack of the view. A vector is used to support random access.

		double deltaAccumulator; // milliseconds since last render
		double framerate; // default is 60fps

		// Utility quads for rendering
		// TODO make this smarter, allow multiple shaders/materials per glcomponent
		Renderable pointQuad, spotQuad, ambientQuad;

		// Render targets to draw to
		std::vector<std::unique_ptr<RenderTarget>> renderTargets;

		std::vector<std::unique_ptr<Renderable>> screensSpaceComp; // A vector that holds only screen space components. These are rendered separately.

		std::shared_ptr<resource::ResourceSystem> resSystem;
	}; // class OpenGLSystem
} // namespace Sigma
#endif // OPENGLSYSTEM_H
