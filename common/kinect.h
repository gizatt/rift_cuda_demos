/* #########################################################################
        Kinect Access Wrapper

        Much reference to:
             http://openkinect.org/wiki/C%2B%2BOpenCvExample

   Rev history:
     Gregory Izatt  20130903    Init revision
   ######################################################################### */ 

#ifndef __XEN_KINECT_H
#define __XEN_KINECT_H

// Base system stuff
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <vector>
#include <time.h>
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>
 #include "../include/SOIL.h"
//Windows
#include <windows.h>

//pthread for mutex
#include <pthread.h>

// opencv needed for mat
#include "libfreenect.hpp"

#include "opencv/cv.h"

#include "Eigen/Dense"
#include "Eigen/Geometry"

#include "xen_utils.h"

namespace xen_rift {


    class XenFreenectDevice : public Freenect::FreenectDevice {
        public:
            XenFreenectDevice(freenect_context *_ctx, int _index);
            void VideoCallback(void* _rgb, uint32_t timestamp);
            // Do not call directly even in child
            void DepthCallback(void* _depth, uint32_t timestamp);
            bool getVideo(cv::Mat& output);
            bool getDepth(cv::Mat& output);

        private:
            std::vector<uint8_t> m_buffer_depth;
            std::vector<uint8_t> m_buffer_rgb;
            std::vector<uint16_t> m_gamma;
            cv::Mat depthMat;
            cv::Mat rgbMat;
            cv::Mat ownMat;
            Mutex m_rgb_mutex;
            Mutex m_depth_mutex;
            bool m_new_rgb_frame;
            bool m_new_depth_frame;
    };

}

#endif //__XEN_KINECT_H