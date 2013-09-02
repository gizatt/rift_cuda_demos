/* #########################################################################
       webcam_feedthrough -- simple webcam-to-RIFT demo

    Many of the filtering options are direct rips from the opencv
    tutorials on their site, btw.

        Press:
            c to enable contour detection ({/} change threshold)
            t to enable thresholding ([/] change threshold)
            b to enable black/white 
            s to enable sobel
            and press +/- to draw image closer or farther to get
                the rough projection size correct.

   Rev history:
     Gregory Izatt  20130901 Init revision
   ######################################################################### */    

#include "Eigen/Dense"
#include "Eigen/Geometry"

// Us!
#include "webcam_feedthrough.h"

#include "../common/rift.h"

// handy image loading
#include "../include/SOIL.h"

// use protection guys
using namespace std;
using namespace OVR;
using namespace xen_rift;
using namespace cv;

typedef enum _get_elapsed_indices{
    GET_ELAPSED_IDLE=0,
    GET_ELAPSED_FRAMERATE=1
} get_elapsed_indices;

//GLUT:
int screenX, screenY;
//Frame counters
int totalFrames = 0;
int frame = 0; //Start with frame 0
double currFrameRate = 0;

//Rift
Rift * rift_manager;

//opencv image capture
CvCapture* capture;
GLuint ipl_convert_texture;
float render_dist = 1.5;
bool black_and_white = false;
bool apply_threshold = false;
bool apply_sobel = false;
bool apply_canny_contours = false;
int threshold_val = 100;
int canny_thresh = 50;
RNG rng(12345);

/* #########################################################################
    
                            forward declarations
                            
   ######################################################################### */        
// opengl initialization
void initOpenGL(int w, int h, void*d);
//    GLUT display callback -- updates screen
void glut_display();
// and shared between eyes rendering core
void render_core();
// GLUT idle callback -- launches a CUDA analysis cycle
void glut_idle();
//GLUT resize callback
void resize(int width, int height);
//Key handlers and mouse handlers; all callbacks for GLUT
void normal_key_handler(unsigned char key, int x, int y);
void normal_key_up_handler(unsigned char key, int x, int y);
void special_key_handler(int key, int x, int y);
void special_key_up_handler(int key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);

// Get our framerate
double get_framerate();
// Return curr time in ms since last call to this func (high res)
double get_elapsed();

//convenience conversion
void ConvertMatToTexture(Mat &image, GLuint texture);


/* #########################################################################
    
                                    MAIN
                                    
        -Parses cmdline args (none right now)
        -Initializes OpenGL and log file
        -%TODO some setup stuff!
        -Registers callback funcs with GLUT
        -Gives control to GLUT's main loop
        
   ######################################################################### */    
int main(int argc, char* argv[]) {    

    //Deal with cmd-line args
    //printf("argc = %d, argv[0] = %s, argv[1] = %s\n",argc, argv[0], argv[1]);
    bool use_hydra = true;
    bool verbose = false;
    for (int i = 1; i < argc; i++) { //Iterate over argv[] to get the parameters stored inside.
        printf("Usage: nothing\n");
        return 0;
    }
    
    printf("Initializing... ");
    srand(time(0));
    // set up timer
    if (init_get_elapsed())
        exit(1);

    //Go get openGL set up / get the critical glob. variables set up
    initOpenGL(1280, 720, NULL);

    //Gotta register our callbacks
    glutIdleFunc( glut_idle );
    glutDisplayFunc( glut_display );
    glutKeyboardFunc ( normal_key_handler );
    glutKeyboardUpFunc ( normal_key_up_handler );
    glutSpecialFunc ( special_key_handler );
    glutSpecialUpFunc ( special_key_up_handler );
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(resize);

    //Rift
    rift_manager = new Rift(1280, 720, true);

    //opencv capture
    capture = cvCaptureFromCAM(CV_CAP_ANY); 

    printf("done!\n");
    glutMainLoop();

    cvReleaseCapture( &capture );
    return(1);
}


/* #########################################################################
    
                                initOpenGL
                                            
        -Sets up both a CUDA and OpenGL context
        -Initializes OpenGL for 3D rendering
        -Initializes the shared vertex buffer that we'll use, and 
            gets it registered with CUDA
        
   ######################################################################### */
void initOpenGL(int w, int h, void*d = NULL) {
    // a bug in the Windows GLUT implementation prevents us from
    // passing zero arguments to glutInit()
    int c=1;
    char* dummy = "";
    glutInit( &c, &dummy );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
    glutInitWindowSize( w, h );
    glutCreateWindow( "display" );

    //Get glew set up, and make sure that worked
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    if (glewIsSupported("GL_VERSION_3_3"))
        ;
    else {
        printf("OpenGL 3.3 not supported\n");
        exit(1);
    }

    // %TODO: this should talk to rift manager
    //Viewpoint setup
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ratio =  w * 1.0 / h;
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);

    //And adjust point size
    glPointSize(2);
    //Enable depth-sorting of points during rendering
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable (GL_BLEND);
    //wglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Clear viewport
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    
    //store our screen sizing information
    screenX = glutGet(GLUT_WINDOW_WIDTH);
    screenY = glutGet(GLUT_WINDOW_HEIGHT);

    glEnable( GL_NORMALIZE );

    glGenTextures(1,&ipl_convert_texture);

    glFinish();
}

void resize(int width, int height){
    glViewport(0,0,width,height);
    screenX = glutGet(GLUT_WINDOW_WIDTH);
    screenY = glutGet(GLUT_WINDOW_HEIGHT);
    rift_manager->set_resolution(screenX, screenY);
}


/* #########################################################################
    
                                glut_display
                                            
        -Callback from GLUT: called whenever screen needs to be
            re-rendered
        -Feeds vertex buffer, along with specifications of what
            is in it, to OpenGL to render.
        
   ######################################################################### */    
void glut_display(){

    //Clear out buffers before rendering the new scene
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //capture webcam frame
    IplImage* frame_ipl = cvQueryFrame( capture );
    // I suspect that frame_ipl should be freed but 
    //  the leak is small enough not to matter if it exists at all.
    //  %TODO
    if ( !frame_ipl ) {
        printf( "ERROR: frame is null...\n" );
    } else {
        Mat frame(frame_ipl);
        Mat gray = Mat(frame.size(),IPL_DEPTH_8U,1);
        Mat gray2 = Mat(frame.size(),IPL_DEPTH_8U,1);

        if (black_and_white || apply_threshold || apply_sobel || apply_canny_contours){
            cvtColor(frame, gray, CV_BGR2GRAY);
        }
        if (apply_threshold){
            threshold( gray, gray2, threshold_val, 255, THRESH_BINARY );
        } else if (apply_sobel){
            Mat grad_x, grad_y;
            Mat abs_grad_x, abs_grad_y;
            // blur first
            GaussianBlur( gray, gray, Size(3,3), 0, 0, BORDER_DEFAULT );
            // Gradient X
            Sobel( gray, grad_x, CV_16S, 1, 0, 3, 1, 0, BORDER_DEFAULT );
            convertScaleAbs( grad_x, abs_grad_x );
            // Gradient Y
            Sobel( gray, grad_y, CV_16S, 0, 1, 3, 1, 0, BORDER_DEFAULT );
            convertScaleAbs( grad_y, abs_grad_y );
            addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, gray2 );
        } else if (apply_canny_contours) {
            Mat canny_output;
            vector<vector<Point> > contours;
            vector<Vec4i> hierarchy;

            /// Detect edges using canny
            Canny( gray, canny_output, canny_thresh, canny_thresh*2, 3 );
            /// Find contours
            findContours( canny_output, contours, hierarchy, 
                CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

            /// Draw contours
            Mat frame2 = Mat(frame.size(), frame.type());
            for( int i = 0; i< contours.size(); i++ ){
                Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
                drawContours( frame2, contours, i, color, 2, 8, hierarchy, 0, Point() );
            }
            frame = frame2;
        }

        if (apply_threshold || apply_sobel)
            cvtColor(gray2, frame, CV_GRAY2BGR);
        else if (black_and_white)
            cvtColor(gray, frame, CV_GRAY2BGR);



          /// Gradient X
          //Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );

          /// Gradient Y
          //Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );


          /// Total Gradient (approximate)
          

        ConvertMatToTexture(frame, ipl_convert_texture);
    }

    // and get player location
    Eigen::Vector3f curr_translation(0.0, 0.0, 0.0);
    Eigen::Vector2f curr_rotation(0.0, 0.0);

    Eigen::Vector3f curr_offset(0.0, 0.0, 0.0);
    Eigen::Vector3f curr_offset_rpy(0.0, 0.0, 0.0);

    Vector3f curr_t_vec(curr_translation.x(), curr_translation.y(), curr_translation.z());
    Vector3f curr_r_vec(0.0f, curr_rotation.y()*M_PI/180.0, 0.0f);
    Vector3f curr_o_vec(curr_offset.x(), curr_offset.y(), curr_offset.z());
    Vector3f curr_ro_vec(curr_offset_rpy.y(), curr_offset_rpy.x(), curr_offset_rpy.z());
    curr_ro_vec = Vector3f();
    // Go do Rift rendering! not using eye offset
    rift_manager->render(curr_t_vec, curr_r_vec+curr_ro_vec, curr_o_vec, false, render_core);

    double curr = get_framerate();
    if (currFrameRate != 0.0f)
        currFrameRate = (10.0f*currFrameRate + curr)/11.0f;
    else
        currFrameRate = curr;

    //output useful framerate and status info:
    //printf ("framerate: %3.1f / %4.1f\n", curr, currFrameRate);
    // frame was rendered, give the player handler a tick

    totalFrames++;
}

/* #########################################################################
    
                                render_core
        Render functionality shared between eyes.

   ######################################################################### */
void render_core(){
    // show ipl convert texture as quad in front of player
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, ipl_convert_texture);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -1.0*render_dist);
    glBegin(GL_POLYGON);
    glTexCoord2f(0, 0);
    glVertex3f(-1, 1, 0);
    
    glTexCoord2f(0, 1);
    glVertex3f(-1, -1, 0);
    
    glTexCoord2f(1, 1);
    glVertex3f(1, -1, 0);
    
    glTexCoord2f(1, 0);
    glVertex3f(1, 1, 0);
    glEnd();
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
}

/* #########################################################################
    
                                glut_idle
                                            
        -Callback from GLUT: called as the idle function, as rapidly
            as possible.
        - Advances particle swarm.

   ######################################################################### */    
void glut_idle(){
    float dt = ((float)get_elapsed( GET_ELAPSED_IDLE )) / 1000.0;

    // and let rift handler update
    rift_manager->onIdle();

    glutPostRedisplay();
}


/* #########################################################################
    
                              normal_key_handler
                              
        -GLUT callback for non-special (generic letters and such)
          keypresses and releases.
        
   ######################################################################### */    
void normal_key_handler(unsigned char key, int x, int y) {
    rift_manager->normal_key_handler(key, x, y);
    switch (key) {
        case '+':
        case '=':
            render_dist += 0.05;
            printf("Renderdist %f\n", render_dist);
            break;
        case '-':
        case '_':
            if (render_dist > 0.05)
                render_dist -= 0.05;
            printf("Renderdist %f\n", render_dist);
            break;
        case 'b':
            black_and_white = !black_and_white;
            break;
        case 't':
            apply_threshold = !apply_threshold;
            if (apply_threshold){
                apply_sobel = false;
                apply_canny_contours = false;
            }
            break;
        case 's':
            apply_sobel = !apply_sobel;
            if (apply_sobel){
                apply_threshold = false;
                apply_canny_contours = false;
            }
            break;
        case 'c':
            apply_canny_contours = !apply_canny_contours;
            if (apply_canny_contours){
                apply_threshold = false;
                apply_sobel = false;
            }
            break;
        case ']':
            if (threshold_val < 255)
                threshold_val+=5;
            printf("Threshold val %d\n", threshold_val);
            break;
        case '[':
            if (threshold_val > 0)
                threshold_val-=5;
            printf("Threshold val %d\n", threshold_val);
            break;
        case '}':
            if (canny_thresh < 120)
                canny_thresh+=5;
            printf("Canny threshold val %d\n", canny_thresh);
            break;
        case '{':
            if (canny_thresh > 0)
                canny_thresh-=5;
            printf("Canny threshold val %d\n", canny_thresh);
            break;
        default:
            break;
    }
}
void normal_key_up_handler(unsigned char key, int x, int y) {
    rift_manager->normal_key_up_handler(key, x, y);
    switch (key) {
        default:
            break;
    }
}

/* #########################################################################
    
                              special_key_handler
                              
        -GLUT callback for special (arrow keys, F keys, etc)
          keypresses and releases.
        -Binds up/down to adjusting parameters
        
   ######################################################################### */    
void special_key_handler(int key, int x, int y){
    rift_manager->special_key_handler(key, x, y);
    switch (key) {
        default:
            break;
    }
}
void special_key_up_handler(int key, int x, int y){
    rift_manager->special_key_up_handler(key, x, y);
    switch (key) {
        default:
            break;
    }
}

/* #########################################################################
    
                                    mouse
                              
        -GLUT callback for mouse button presses
        -Records mouse button presses when they happen, for use
            in the mouse movement callback, which does the bulk of
            the work in managing the camera
        
   ######################################################################### */    
void mouse(int button, int state, int x, int y){
    rift_manager->mouse(button, state, x, y);
}


/* #########################################################################
    
                                    motion
                              
        -GLUT callback for mouse motion
        -When the mouse moves and various buttons are down, adjusts camera.
        
   ######################################################################### */    
void motion(int x, int y){
    rift_manager->motion(x, y);
}


/* #########################################################################
    
                                get_framerate
                                            
        -Takes totalFrames / ((curr time - start time)/CLOCKS_PER_SEC)
            INDEPENDENT OF GET_ELAPSED; USING THESE FUNCS FOR DIFFERENT
                TIMING PURPOSES
   ######################################################################### */     
double get_framerate ( ) {
    double elapsed = (double) get_elapsed(GET_ELAPSED_FRAMERATE);
    double ret;
    if (elapsed != 0.){
        ret = ((double)totalFrames) / elapsed;
    }else{
        ret =  -1.0;
    }
    totalFrames = 0;
    return ret;
}

/* #########################################################################
    
                               ConvertMatToTexture
                                            
        -Handy IPL to OpenGL Texture conversion based on
    http://carldukeprogramming.wordpress.com/2012/08/28/converting-
        iplimage-to-opengl-texture/
   ######################################################################### */   
void ConvertMatToTexture(Mat &image, GLuint texture)
{

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  gluBuild2DMipmaps(GL_TEXTURE_2D,3,image.size().width,image.size().height,
                    GL_BGR,GL_UNSIGNED_BYTE,image.ptr());
}