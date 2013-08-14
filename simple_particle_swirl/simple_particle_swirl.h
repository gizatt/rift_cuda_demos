/* simple_particle_swirl: Oculus Rift CUDA-powered demo 
   Header!
*/

#ifndef __SIMPLE_PARTICLE_SWIRL_H
#define __SIMPLE_PARTICLE_SWIRL_H

// Base system stuff
#include <stdio.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

// OpenGL and friends
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>

//Windows
#include <windows.h>

//Rift
#include "OVR.h"

namespace xen_rift {
	// Data dimensionality
	#define NUM_PARTICLES (1024*250)

};

#endif //__SIMPLE_PARTICLE_SWIRL_H