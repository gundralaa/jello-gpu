#pragma once

#include "includes.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <streambuf>
#include <vector>


const char *GLErrorStr(GLenum err);
void getErrors(const char *place);
void getInfoLog(GLuint program);
void validateProgram(GLuint program);
void loadShader(const char *path, GLuint type, GLuint program, char *prelude);