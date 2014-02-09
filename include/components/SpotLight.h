#pragma once

#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include "glm/glm.hpp"
#include "IComponent.h"
#include "systems/opengl/GLSLShader.h"
#include "GLTransform.h"
#include "Sigma.h"

namespace Sigma {
	class SpotLight : public IComponent {
	public:
		SpotLight(const id_t entityID);
		virtual ~SpotLight() {}

		SET_COMPONENT_TYPENAME("SpotLight");

		GLTransform transform;
		glm::vec4 color;
		float intensity;

		float innerAngle;
		float outerAngle;
		float cosInnerAngle;
		float cosOuterAngle;

		bool enabled;

		bool IsEnabled() { return enabled; }
	};
}
#endif
