/* #########################################################################
        Player class -- camera viewport and movement management.

	Manages records of the player's current position in space, and provides
	handler for movement key presses.

   Rev history:
     Gregory Izatt  20130718  Init revision
   ######################################################################### */    

#ifndef __XEN_PLAYER_H
#define __XEN_PLAYER_H

// Base system stuff
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector_types.h>
#include "../include/GL/cutil_math.h"
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"

#include <gl/gl.h>

namespace xen_rift {
	class Player {
		public:
			Player(float3, float2, float);
			void on_frame_render();
			void normal_key_handler(unsigned char key, int x, int y);
			void normal_key_up_handler(unsigned char key, int x, int y);
			void special_key_handler(int key, int x, int y);
			void special_key_up_handler(int key, int x, int y);
			void mouse(int button, int state, int x, int y);
			void motion(int x, int y);
			float3 get_position() { return _position; }
			float2 get_rotation() { return _rotation; }
		protected:
			float3 _position;
			float2 _rotation;
			float _eye_height;
			int _mouseOldX;
			int _mouseOldY;
			int _mouseButtons;
			bool _w_down;
			bool _a_down;
			bool _s_down;
			bool _d_down;
			bool _c_down;
		private:
	};
};

#endif //__XEN_PLAYER_H