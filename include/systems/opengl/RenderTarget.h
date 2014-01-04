#pragma once
#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include <GL/glew.h>
#include <vector>

namespace Sigma{
	struct RenderTarget {
		std::vector<GLuint> texture_ids;
		GLuint fbo_id;
		GLuint depth_id;
		unsigned int width;
		unsigned int height;
		bool hasDepth;

		RenderTarget() : fbo_id(0), depth_id(0) {}
		virtual ~RenderTarget();

		void BindWrite();
		void BindRead();
		void UnbindWrite();
		void UnbindRead();
	};
} // namespace Sigma

#endif // RENDERTARGET_H
