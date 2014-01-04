#include "systems/opengl/IGLSLShader.h"

#include <fstream>
#include <iostream>

namespace Sigma{

	std::map<std::string, std::shared_ptr<IGLSLShader>> IGLSLShader::registry;

	void IGLSLShader::register_shader(const std::string &name, IGLSLShader *newshader){
		if(IGLSLShader::registry.find(name) == IGLSLShader::registry.end())
			IGLSLShader::registry[name] = std::shared_ptr<IGLSLShader>(newshader);
		else
			std::cerr << "Shader name conflict: " << name << std::endl;
	}

	GLuint IGLSLShader::LoadAndCompile(){
		// create the shader
		GLuint shader_id;
		switch(this->_type){
		case VERTEX_SHADER:
			shader_id = glCreateShader(GL_VERTEX_SHADER);
			break;
		case FRAGMENT_SHADER:
			shader_id = glCreateShader(GL_FRAGMENT_SHADER);
			break;
		}
		// load the shader source
        std::ifstream fp;
        std::string buffer;
        fp.open(this->_source_path.c_str(), std::ios_base::in);
        if(fp) {
                buffer = std::string(std::istreambuf_iterator<char>(fp), (std::istreambuf_iterator<char>()));
        } else {
                std::cerr << "Error loading shader: " << _source_path << std::endl;
        }
        const char * ptmp = buffer.c_str();
        glShaderSource (shader_id, 1, &ptmp, NULL);

        // compile and check status
        GLint status;
        glCompileShader (shader_id);
        glGetShaderiv (shader_id, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
                GLint infoLogLength;
                glGetShaderiv (shader_id, GL_INFO_LOG_LENGTH, &infoLogLength);
                GLchar *infoLog= new GLchar[infoLogLength];
                glGetShaderInfoLog (shader_id, infoLogLength, NULL, infoLog);
                std::cerr<<"Compile log: "<<infoLog<<std::endl;
                delete [] infoLog;
        }
        return shader_id;
	}

	std::shared_ptr<IGLSLShader> IGLSLShader::Get(const std::string &name){
        if(IGLSLShader::registry.find(name) != IGLSLShader::registry.end()){
            return IGLSLShader::registry[name];
        }
        else{
            std::cerr << "IGLSLShader::Get called for unregistered name: " << name << std::endl;
            return std::shared_ptr<IGLSLShader>(nullptr);
        }
    }
}
