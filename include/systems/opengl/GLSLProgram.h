#pragma once
#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#include <GL/glew.h>
#include <string>
#include <map>
#include <memory>
#include "systems/opengl/IGLSLShader.h"

namespace Sigma{

    /** \brief A Deleter functor object used to handle deleting the shared_ptr of the program object
     *
     * This allows multiple GLSLProgram objects to use the same compiled program, and the program
     * is only deleted once its reference count is 0
     */
	struct ProgramDeleter{
		void operator()(GLuint *program){
			glDeleteProgram(*program);
			delete program;
		}
	};

	class GLSLProgram
	{
		public:
			GLSLProgram() : program(new GLuint), vShader(nullptr), fShader(nullptr) {}

			void Link();
			void AddShader(const std::string &name);
			void AddShader(std::shared_ptr<IGLSLShader> shader);
			bool isLoaded() { return *program != 0; }

			void Use() const { glUseProgram(*program); }
			void UnUse() const { glUseProgram(0); }

		private:
			// the specific combination of vertex and fragment shader objects is unique to this program object
			// TODO add geometry shader option
			std::shared_ptr<IGLSLShader> vShader, fShader;
			// the program itself may be shared with other GLSLProgram objects
			// so a shared_ptr is used so that reference counting is done for us
			std::shared_ptr<GLuint> program;
			// map for shared programs (keys are unique for each unique pair of VS and FS source files)
			std::map<std::string, std::shared_ptr<GLuint>> loadedPrograms;
	};
} // namespace Sigma

#endif // GLSLPROGRAM_H
