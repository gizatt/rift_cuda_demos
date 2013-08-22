/* #########################################################################
        Hydra class -- Hydra initialization and such convenience functions

   Rev history:
     Gregory Izatt  20130808  Init revision
   ######################################################################### */    

#ifndef __XEN_HYDRA_H
#define __XEN_HYDRA_H

// Base system stuff
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>

#include "xen_utils.h"

#include <windows.h>
#include "textbox_3d.h"

#define SIXENSE_STATIC_LIB
#include "sixense.h"
#include "sixense_utils\controller_manager\controller_manager.hpp"

#include "Eigen/Dense"
#include "Eigen/Geometry"
     
namespace xen_rift {

	typedef enum _calibration_state_t {
		NOT_CALIBRATING,
		STARTING_CALIBRATING,
		MIDDLE_CALIBRATING
	} calibration_state_t;

	class Hydra {
		public:
			Hydra( bool using_hydra = true, bool verbose = true );

			// glut passthroughs
			void normal_key_handler(unsigned char key, int x, int y);
			void normal_key_up_handler(unsigned char key, int x, int y);
			void special_key_handler(int key, int x, int y);
			void special_key_up_handler(int key, int x, int y);
			void mouse(int button, int state, int x, int y);
			void motion(int x, int y);
			void onIdle( void );
			void draw( Eigen::Vector3f& player_origin, Eigen::Quaternionf& player_orientation );
			void draw_cursor( unsigned char hand, Eigen::Vector3f& player_origin, 
					Eigen::Quaternionf& player_orientation );
			Eigen::Vector3f getCurrentPos(unsigned char hand);
			Eigen::Vector3f getCurrentRPY(unsigned char hand);
			Eigen::Quaternionf getCurrentQuat(unsigned char which_hand);

			EIGEN_MAKE_ALIGNED_OPERATOR_NEW
		protected:
			bool _verbose;
			bool _using_hydra;
        	sixenseAllControllerData _acd;
        	Eigen::Vector3f _posl0;
        	Eigen::Vector3f _posr0;
        	Eigen::Quaternionf _quatl0;
        	Eigen::Quaternionf _quatr0;
        	calibration_state_t _calibration_state;
        	Textbox_3D * _instruction_textbox;
        	Eigen::Vector3f _touch_point;

		private:
	};

	void controller_manager_setup_callback(  sixenseUtils::ControllerManager::setup_step step );

}

#endif //__XEN_HYDRA_H