#pragma once
#ifndef OPENGLSYSTEM_H
#define OPENGLSYSTEM_H

#include "Property.h"

#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include <memory>

#include "IFactory.h"
#include "ISystem.h"
#include "IGLComponent.h"
#include "systems/IGLView.h"
#include <vector>
#include "resources/GLTexture.h"

struct IGLView;

namespace Sigma{

	struct RenderTarget {
		GLuint texture_id;
		GLuint fbo_id;
		GLuint depth_id;

		RenderTarget() : texture_id(0), fbo_id(0), depth_id(0) {}
		virtual ~RenderTarget();

		void Use(int slot);
	};

    class OpenGLSystem
        : public Sigma::IFactory, public ISystem<IComponent> {
    public:

        OpenGLSystem();

        /**
         * \brief Starts the OpenGL rendering system.
         *
         * Starts the OpenGL rendering system and creates a rendering context. It will also attempt to create a newer rendering context (>3) if available.
         * \return -1 in the 0 index on failure, else the major and minor version in index 0 and 1 respectively.
         */
        const int* Start();

        /**
         * \brief Causes an update in the system based on the change in time.
         *
         * Updates the state of the system based off how much time has elapsed since the last update.
         * \param delta the time (in milliseconds) since the last update
         * \return true if rendering was performed
         */
        bool Update(const double delta);

        /**
         * \brief Sets the window width and height for glViewport
         *
         * \param width new width for the window
         * \param height new height for the window
         * \return void
         */
        void SetWindowDim(int width, int height) { this->windowWidth = width; this->windowHeight = height; }

        /**
         * \brief Sets the viewport width and height.
         *
         * \param width new viewport width
         * \param height new viewport height
         */
        void SetViewportSize(const unsigned int width, const unsigned int height);

        /**
         * \brief set the framerate at runtime
         *
         *  Note that the constructor sets a default of 60fps
         */
        void SetFrameRate(double fr) { this->framerate = fr; }

        std::map<std::string,FactoryFunction> getFactoryFunctions();

		IComponent* createPointLight(const unsigned int entityID, const std::vector<Property> &properties);
		IComponent* createScreenQuad(const unsigned int entityID, const std::vector<Property> &properties);

		// TODO: Move these methods to the components themselves.
        IComponent* createGLSprite(const unsigned int entityID, const std::vector<Property> &properties) ;
        IComponent* createGLIcoSphere(const unsigned int entityID, const std::vector<Property> &properties) ;
        IComponent* createGLCubeSphere(const unsigned int entityID, const std::vector<Property> &properties) ;
        IComponent* createGLMesh(const unsigned int entityID, const std::vector<Property> &properties) ;
		// Views are not technically components, but perhaps they should be
		IComponent* createGLView(const unsigned int entityID, const std::vector<Property> &properties, std::string mode) ;

		// Managing rendering internals
		/*
		 * \brief creates a new render target of desired size
		 */
		int createRenderTarget(const unsigned int w, const unsigned int h, const unsigned int format);
		
		/*
		 * \brief returns the fbo_id of primary render target (index 0)
		 */
		int getRender() { return (this->renderTargets.size() > 0) ? this->renderTargets[0]->fbo_id : -1; }
		int getRenderTexture() { return (this->renderTargets.size() > 0) ? this->renderTargets[0]->texture_id : -1; }

		// Rendering methods
		void RenderTexture(GLuint texture_id);

        /**
         * \brief Gets the specified view.
         *
		 * \param unsigned int index The index of the view to retrieve.
         * \return IGLView* The specified view.
         */
		IGLView* GetView(unsigned int index = 0) const {
			if (index >= this->views.size()) {
				//return this->views[this->views.size() - 1];
				return 0;
			}
			return this->views[index];
		}

		/**
		 * \brief Adds a view to the stack.
		 *
		 * \param[in/out] IGLView * view The view to add to the stack.
		 * \return void
		 */
		void PushView(IGLView* view) {
			this->views.push_back(view);
		}

		/**
		 * \brief Pops a view from the stack.
		 *
		 * Pops a view from the stack and deletes it.
		 * \return void
		 */
		void PopView() {
			if (this->views.size() > 0) {
				delete this->views[this->views.size() - 1];
				this->views.pop_back();
			}
		}

		GLTransform* GetTransformFor(const unsigned int entityID);

		/**
		 * \brief Gets the current view mode.
		 *
		 * This method needs to be reworked because views are now stack based and as such this is for the primary view.
		 * \return    const std::string& The current view mode.
		 */
		const std::string& GetViewMode() { return this->viewMode; }

		static std::map<std::string, Sigma::resource::GLTexture> textures;
    private:
        unsigned int windowWidth; // Store the width of our window
        unsigned int windowHeight; // Store the height of our window

        int OpenGLVersion[2];

		// Scene matrices
        glm::mat4 ProjectionMatrix;
        std::vector<IGLView*> views; // A stack of the view. A vector is used to support random access.

        double deltaAccumulator; // milliseconds since last render
        double framerate; // default is 60fps
		
		// Type of view to create
		std::string viewMode;

		// Render targets to draw to
		std::vector<std::unique_ptr<RenderTarget>> renderTargets;

		std::vector<std::unique_ptr<IGLComponent>> screensSpaceComp; // A vector that holds only screen space components. These are rendered separately.
    }; // class OpenGLSystem
} // namespace Sigma
#endif // OPENGLSYSTEM_H
