/* #########################################################################
        Ironman HUD Manager

	Provides basis for a helmet HUD around player head position.

   Rev history:
     Gregory Izatt  20130818  Init revision
   ######################################################################### */    

#ifndef __XEN_IRONMAN_HUD_H
#define __XEN_IRONMAN_HUD_H

// Base system stuff
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>

#include "textbox_3d.h"
#include "Eigen/Dense"
#include "Eigen/Geometry"

namespace xen_rift {
	class Ironman_HUD {
		public:
			Ironman_HUD( float k = 0.0, float dampening = 0.1, float tether_dist=100.0,
						 float k_o = 0.5, float dampening_o = 0.8 );
			void add_textbox( std::string& init_text, Eigen::Vector3f& init_offset_xyz,
								Eigen::Quaternionf& init_offset_quat, float width = 0.3, 
								float height=0.2, float depth=0.05, float line_width = 1.0f);
			void onIdle( Eigen::Vector3f& player_origin, Eigen::Quaternionf& player_orientation, float dt );
			void draw( void );

			EIGEN_MAKE_ALIGNED_OPERATOR_NEW
		protected:
			std::vector<xen_rift::Textbox_3D *> _textboxes;
			std::vector<Eigen::Vector3f *> _offsets_xyz;
			std::vector<Eigen::Quaternionf *> _offsets_quats;

			float _k; float _k_o;
			float _dampening; float _dampening_o;
			float _tether_dist;
			Eigen::Vector3f _last_pos;
			Eigen::Quaternionf _last_orientation;
			Eigen::Vector3f _velocity;
			Eigen::Quaternionf _velocity_o;
			float _velocity_rps;

		private:
	};
};

#endif //__XEN_TEXTBOX_3D_H