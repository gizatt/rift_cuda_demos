/* #########################################################################
        Rift class -- rift initialization and such convenience functions

   Rev history:
     Gregory Izatt  20130721  Init revision
   ######################################################################### */    

#ifndef __XEN_RIFT_H
#define __XEN_RIFT_H

// Base system stuff
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector_types.h>
#include "../include/GL/cutil_math.h"
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>

#include "OVR.h"

#include <windows.h>

namespace xen_rift {
	typedef enum _detect_prompt_t {
		PR_CONTINUE,
		PR_ABORT,
		PR_RETRY
	} detect_prompt_t;

	typedef enum _PostProcessType{
	    PostProcess_None,
	    PostProcess_Distortion
	} PostProcessType;

	const OVR::Vector3f UpVector(0.0f, 1.0f, 0.0f);
	const OVR::Vector3f ForwardVector(0.0f, 0.0f, -1.0f);
	const OVR::Vector3f RightVector(1.0f, 0.0f, 0.0f);

	class Rift : public OVR::MessageHandler {
		public:
			Rift(int inputHeight = 1280, int inputWidth = 720, bool verbose = true );
			void Rift::OnMessage(const OVR::Message& msg);
			// glut passthroughs
			void normal_key_handler(unsigned char key, int x, int y);
			void normal_key_up_handler(unsigned char key, int x, int y);
			void special_key_handler(int key, int x, int y);
			void special_key_up_handler(int key, int x, int y);
			void mouse(int button, int state, int x, int y);
			void motion(int x, int y);
			void onIdle( void );
			void render(OVR::Vector3f EyePos, OVR::Vector3f EyeRot, void (*draw_scene)(void));
			void render_one_eye(const OVR::Util::Render::StereoEyeParams& stereo, 
                            OVR::Matrix4f view_mat, void (*draw_scene)(void));
		protected:
		    // *** Rendering Variables
		    int                 _width;
		    int 				_height;
		    // Stereo view parameters.
		    OVR::Util::Render::StereoConfig        _SConfig;
		    PostProcessType     _PostProcess;
		    // *** Oculus HMD Variables
		    OVR::Ptr<OVR::DeviceManager>  _pManager;
		    OVR::Ptr<OVR::SensorDevice>   _pSensor;
		    OVR::Ptr<OVR::HMDDevice>      _pHMD;
		    OVR::SensorFusion        _SFusion;
		    OVR::HMDInfo        _HMDInfo;

		     // Position and look. The following apply:
		    OVR::Vector3f       _EyePos;
		    float               _EyeYaw;         // Rotation around Y, CCW positive when looking at RHS (X,Z) plane.
		    float               _EyePitch;       // Pitch. If sensor is plugged in, only read from sensor.
		    float               _EyeRoll;        // Roll, only accessible from Sensor.
		    float               _LastSensorYaw;  // Stores previous Yaw value from to support computing delta.

		    // timekeeping
		    LARGE_INTEGER _lasttime;
		    LARGE_INTEGER _currtime;

		    // mouselook enabled?
		    bool _mouselook;
			int _mouseOldX;
			int _mouseOldY;
			int _mouseButtons;

			// verbose?
			bool _verbose;

		private:
	};
};

#endif //__XEN_PLAYER_H