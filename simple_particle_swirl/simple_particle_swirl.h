/* simple_particle_swirl: Oculus Rift CUDA-powered demo 
   Header!
*/

#ifndef __SIMPLE_PARTICLE_SWIRL_H
#define __SIMPLE_PARTICLE_SWIRL_H

// Base system stuff
#include <stdio.h>
#include <time.h>

// OpenGL and friends
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>

// CUDA and related
#include "../include/GL/cutil.h"
#include "../include/book.h"
#include <cuda.h>
#include <cuda_gl_interop.h>

//Windows
#include <windows.h>

//Rift
#include "OVR.h"

namespace xen_rift {
	// Data dimensionality
	#define NUM_PARTICLES (1024*2000)
	// buffer size for particle positions or velocities: 
	#define BUFFER_SIZE (NUM_PARTICLES*sizeof(float4))

	// CUDA block size
	#define BLOCK_SIZE (1024)
	#define GRID_SIZE (NUM_PARTICLES/BLOCK_SIZE)
};

#endif //__SIMPLE_PARTICLE_SWIRL_H