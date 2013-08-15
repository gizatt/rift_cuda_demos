/* #########################################################################
        3D Textbox Class -- UI atom

	Manages records of the player's current position in space, and provides
	handler for movement key presses.

   Rev history:
     Gregory Izatt  20130718  Init revision
   ######################################################################### */    

#ifndef __XEN_TEXTBOX_3D_H
#define __XEN_TEXTBOX_3D_H

// Base system stuff
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>

#include "Eigen/Dense"
#include "Eigen/Geometry"

namespace xen_rift {
	class Textbox_3D {
		public:
			Textbox_3D(std::string& text, Eigen::Vector3f& initpos, Eigen::Vector3f& initfacedir, 
				float width = 0.3, float height=0.2, float depth=0.05);
			Textbox_3D(std::string& text, Eigen::Vector3f& initpos, Eigen::Quaternionf& initquat, 
				float width = 0.3, float height=0.2, float depth=0.05);
			void set_pos( Eigen::Vector3f& newpos );
			void draw( void );
		
		protected:
			// This is the dead center of the box
			Eigen::Vector3f _pos;
			Eigen::Quaternionf _rot;
			float _depth;
			float _width;
			float _height;
			std::string _text;
		private:
	};
};

#endif //__XEN_TEXTBOX_3D_H