#include "systems/opengl/GLSLProgram.h"
#include <iostream>

namespace Sigma{

    void GLSLProgram::AddShader(const std::string &name){
        std::shared_ptr<IGLSLShader> lookup = IGLSLShader::Get(name);
        this->AddShader(lookup);
    }

    void GLSLProgram::AddShader(std::shared_ptr<IGLSLShader> shader){
        if(shader.get() != nullptr){
            switch(shader->GetType()){
            case IGLSLShader::ShaderType::VERTEX_SHADER:
                this->vShader = shader;
                break;
            case IGLSLShader::ShaderType::FRAGMENT_SHADER:
                this->fShader = shader;
                break;
            }
        }
    }

	void GLSLProgram::Link(){
		if(vShader.get() == nullptr){
			std::cerr << "Cannot link program without a vertex shader" << std::endl;
		}
		else if(fShader.get() == nullptr){
			std::cerr << "Cannot link program without a fragment shader" << std::endl;
		}
		else{
			std::string composite_name = vShader->GetPath() + ":" + fShader->GetPath();
			// if an identical program already exists, use that
			if(GLSLProgram::loadedPrograms.find(composite_name) != GLSLProgram::loadedPrograms.end()){
				this->program = GLSLProgram::loadedPrograms[composite_name];
			}
			// otherwise we need to create and link the first instance of this program
			else{
				GLuint* pid = new GLuint;
				*pid = glCreateProgram();
				this->program = std::shared_ptr<GLuint>(pid, ProgramDeleter());
				// compile each shader
				GLuint vs_id = vShader->LoadAndCompile();
				GLuint fs_id = fShader->LoadAndCompile();
				// attach each shader
				glAttachShader(*this->program, vs_id);
				glAttachShader(*this->program, fs_id);
				// link the program
				glLinkProgram(*this->program);
				// check for errors
				GLint status;
				glGetProgramiv(*this->program, GL_LINK_STATUS, &status);
				if(status == GL_FALSE){
					std::cerr << "Could not link program for " << (vShader->GetName()) << " and " << (fShader->GetName()) << std::endl;
					glDeleteProgram(*this->program);
				}
			}

			this->Use();
			{
                vShader->InitUniforms();
                fShader->InitUniforms();
			}
			this->UnUse();
		}
	}

}
