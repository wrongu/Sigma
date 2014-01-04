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
            // Store names
            if(this->unique_name == ""){
                // create a name that is a composition of the shaders' names
                this->unique_name = vShader->GetName() + ":" + fShader->GetName();
            }
			this->composite_path = vShader->GetPath() + ":" + fShader->GetPath();
			// if a program with the same source has already been created, use that
			if(GLSLProgram::loadedPrograms.find(this->composite_path) != GLSLProgram::loadedPrograms.end()){
				this->program = GLSLProgram::loadedPrograms[this->composite_path];
			}
			// otherwise we need to create and link the first instance of this program
			else{
                // This is basically like program=std::make_shared<GLuint>(glCreateProgram());,
                // except this allows setting a custom deleter
			    GLuint *prog = new GLuint;
                *prog = glCreateProgram();
                this->program = std::shared_ptr<GLuint>(prog, ProgramDeleter());
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
				// if successful, save it
				else{
                    GLSLProgram::loadedPrograms[this->composite_path] = this->program;
				}
			}

            // Some uniforms are set once at initialization. request initialization here
			this->Use();
			this->InitUniforms();
			this->UnUse();
		}
	}

}
