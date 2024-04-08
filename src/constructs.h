#pragma once

#include "includes.h"

typedef struct {
    /*
    types: 
    0: null
    1: structural
    2: shearing
    3: bending
    */ 
    GLuint point1;
    GLuint point2;
    GLuint type;
    GLfloat len;
} Spring;

typedef struct  {
    GLuint index1;
    GLuint index2;
    GLuint index3;
} Face;
