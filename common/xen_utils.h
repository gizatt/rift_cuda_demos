/* #########################################################################
        General utility functions used by common classes in here.
            They're all conveniences...

        Header.

   Rev history:
     Gregory Izatt  20130805    Init revision
   ######################################################################### */ 

#ifndef __XEN_UTILS_H
#define __XEN_UTILS_H

// Base system stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>
 #include "../include/SOIL.h"

namespace xen_rift {
    // print log wrt a shader
    void printShaderInfoLog(GLuint obj);

    // loads shader from file
    void load_shaders(char * vertexFileName, GLuint * vshadernum,
                       char * fragmentFileName, GLuint * fshadernum,
                       char * geomFileName = NULL, GLuint * gshadernum = NULL);

    // reads file as one large string
    char *textFileRead(char *fn);
    
    //render fullscreen quad
    void renderFullscreenQuad();

    //load a skybox cubemap from a base string
    int loadSkyBox(char * base_str, GLuint * out);
}

#endif //__XEN_UTILS_H