#pragma once
#ifndef IGLSL_SHADER_H
#define IGLSL_SHADER_H

#include <GL/glew.h>
#include "systems/opengl/IGLView.h"
#include "IGLComponent.h"

#define REGISTER_SHADER(classname, type, source, name) \
public:\
virtual void CreateAndRegister() { \
    if(IGLSLShader::registry.find(name) = IGLSLShader::registry.end()) \
        IGLSLShader::registry[name] = std::shared_ptr<IGLSLShader>(new classname(type, source, name)); \
} \
private:\
IGLSLShader() : _name(name), _source_path(source), _type(type) { \
	IGLSLShader::register_shader(name, this); \
} \
static StaticRegistrar registrar;

#define STATIC_INSTANCE(classname) Sigma::StaticRegistrar classname::registrar;

namespace Sigma{

    struct StaticRegistrar{
        StaticRegistrar(void (*factory)()){
            factory();
        }
    };

	class IGLSLShader
	{
		public:
			// TODO add geometry shaders
			enum ShaderType {VERTEX_SHADER, FRAGMENT_SHADER};

			// constructor is private, and is defined in REGISTER_SHADER
			// destructor is default

			GLuint LoadAndCompile();
			static std::shared_ptr<IGLSLShader> Get(const std::string &name);
			std::string GetPath() const { return _source_path; }
			std::string GetName() const { return _name; }
			ShaderType GetType() const  { return _type; }

			virtual void InitUniforms() {}
			virtual void FrameUpdateUniforms(IGLView *view) {}
			virtual void ComponentUpdateUniforms(IGLComponent *component) {}
		protected:
            // subclasses need only use the macro REGISTER_SHADER to define CreateAndRegister
            // and STATIC_INSTANCE to create the instance of this shader
			virtual void CreateAndRegister() = 0;
			std::string _source_path; // relative path, like 'shaders/simple.vert'
			std::string _name; // a unique name for this shader
			ShaderType _type;  // whether it's vertex or fragment

		private:
			static std::map<std::string, std::shared_ptr<IGLSLShader>> registry;
			static void register_shader(const std::string &name, IGLSLShader *newshader);
	}; // class IGLSLShader
} // namespace Sigma

#endif // IGLSL_SHADER_H
