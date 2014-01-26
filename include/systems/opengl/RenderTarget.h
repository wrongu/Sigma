#pragma once
#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include <GL/glew.h>
#include <vector>

namespace Sigma{
	/**
	 * \brief a RenderTarget is a frame-buffer (FBO) with convenience methods
	 *
	 * A Frame Buffer is like a hidden internal screen which you can render to
	 *  and save the result as a texture for further processing
	 */
	class RenderTarget {
	public:
		/** \brief create a RenderTarget; creates a new FBO with a depth buffer and no textures */
		RenderTarget(const int w, const int h, const bool hasDepth);

		/** \brief destroy the opengl objects referenced by this RenderTarget */
		virtual ~RenderTarget();

		/** \brief create and attach a new texture2D with the given parameters
		 *
		 * \return the id of the new texture
		 */
		GLuint CreateTexture(GLint format, GLenum internalFormat, GLenum type);

        GLuint GetId() const { return this->fbo_id; }
        GLuint GetTexture(const int target) const { return this->texture_ids[target]; }

		void BindWrite();
		void BindRead();
		void UnbindWrite();
		void UnbindRead();

	private:

		/** \brief helper method to create and attach a depth buffer for this RenderTarget */
		void CreateDepthBuffer();

		void InitBuffers();
		std::vector<GLuint> texture_ids;
		GLuint fbo_id;
		GLuint depth_id;
		unsigned int width;
		unsigned int height;
		bool hasDepth;
	};
} // namespace Sigma

#endif // RENDERTARGET_H
