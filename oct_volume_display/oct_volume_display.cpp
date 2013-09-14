/* #########################################################################
       oct volume display, not very fancy

   Rev history:
     Gregory Izatt  20130913 Init revision
   ######################################################################### */    

#include "Eigen/Dense"
#include "Eigen/Geometry"

// Us!
#include "oct_volume_display.h"

#include "../common/rift.h"
#include "../common/textbox_3d.h"
#include "../common/xen_utils.h"

// use protection guys
using namespace std;
using namespace OVR;
using namespace xen_rift;

typedef enum _get_elapsed_indices{
    GET_ELAPSED_IDLE=0,
    GET_ELAPSED_FRAMERATE=1
} get_elapsed_indices;

//GLUT:
int screenX, screenY;
//Frame counters
int totalFrames = 1;
int frame = 0; //Start with frame 0
double currFrameRate = 0;

//Rift
Rift * rift_manager;

// FPS textbox
Textbox_3D * textbox_fps;
Eigen::Vector3f textbox_fps_pos(-2.0, -2.0, -2.0);

bool show_textbox_hud = true;

char data_filename[] = "out_greg_500x_512y_50f.raw";
int data_x = 500;
int data_y = 512;
int data_z = 50;
FILE *data_file;
short int * data;
float * vertexBuffer;
static GLuint VBO[1];

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
void load_cornea_data_to_mat();

/* #########################################################################
    
                                    MAIN
                                    
        -Parses cmdline args (none right now)
        -Initializes OpenGL and log file
        -%TODO some setup stuff!
        -Registers callback funcs with GLUT
        -Gives control to GLUT's main loop
        
   ######################################################################### */    
int main(int argc, char* argv[]) {    

/*
    printf("\n\n\n");
    printf("Hello world\n");

    return 0;
*/

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
    get_elapsed(GET_ELAPSED_FRAMERATE);

    //Go get openGL set up / get the critical glob. variables set up
    initOpenGL(1280, 720, NULL);

    load_cornea_data_to_mat();

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

    //fps textbox
    Eigen::Vector3f tmpdir = -1.0*textbox_fps_pos;
    textbox_fps = new Textbox_3D(string("FPS: NNN"), textbox_fps_pos, 
           tmpdir, 1.5, 0.8, 0.05, 5);

    printf("... done!\n");

    glutMainLoop();

    return 0;
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

    char tmp[100];
    sprintf(tmp, "FPS: %0.3f", currFrameRate);
    textbox_fps->set_text(string(tmp));

    //output useful framerate and status info:
    // printf ("framerate: %3.1f / %4.1f\n", curr, currFrameRate);
    // frame was rendered, give the player handler a tick

    totalFrames++;
}

/* #########################################################################
    
                                render_core
        Render functionality shared between eyes.

   ######################################################################### */
void render_core(){

    glPushMatrix();
    glLoadIdentity();
    // Bind in the shared vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, *VBO );
    glEnableClientState( GL_COLOR_ARRAY );
    glEnableClientState( GL_VERTEX_ARRAY );
    // Each 16-byte vertex in that buffer includes coodinate information
    //  and color information:
    //  byte index [ 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  ]
    //  info       [ <x, float>  <y, float>  <z, float >  <r   g   b    a, unsigned bytes>]
    // These cmds instruct opengl to expect that:
    glColorPointer(4,GL_UNSIGNED_BYTE,16,(void*)12);
    glVertexPointer(3,GL_FLOAT,16,(void*)0);

    glDrawArrays(GL_POINTS,0,data_x*data_y*data_z);

    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPopMatrix();

    // and textboxs
    if (show_textbox_hud){
        glPushMatrix();
        glLoadIdentity();
        Eigen::Vector3f updog = Eigen::Vector3f(Eigen::Quaternionf::FromTwoVectors(
            Eigen::Vector3f(0.0, 0.0, -1.0), textbox_fps_pos)*Eigen::Vector3f(0.0, 1.0, 0.0));
        textbox_fps->draw( updog );
        glPopMatrix();
    }

}

/* #########################################################################
    
                                glut_idle
                                            
        -Callback from GLUT: called as the idle function, as rapidly
            as possible.
        - Advances particle swarm.

   ######################################################################### */    
float total_wait = 0;
void glut_idle(){
    float dt = ((float)get_elapsed( GET_ELAPSED_IDLE )) / 1000.0;
    total_wait += dt;

    // and let rift handler update
    rift_manager->onIdle();

    if (total_wait > 30){
        total_wait = 0;
        glutPostRedisplay();
    }
}


/* #########################################################################
    
                              normal_key_handler
                              
        -GLUT callback for non-special (generic letters and such)
          keypresses and releases.
        
   ######################################################################### */    
void normal_key_handler(unsigned char key, int x, int y) {
    rift_manager->normal_key_handler(key, x, y);
    switch (key) {
        case 'h':
            show_textbox_hud = !show_textbox_hud;
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
    double elapsed = (1./1000.)*(double) get_elapsed(GET_ELAPSED_FRAMERATE);
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
                        helper
   ######################################################################### */   
void load_cornea_data_to_mat()
{
    data = (short int *) malloc(data_x*data_y*data_z*sizeof(short int));
    if (!data){
        printf("Couldn't alloc buffer!\n");
        exit(1);
    }

    printf("Allocated buffer, reading in data of size %d bytes\n", sizeof(short int));

    data_file = fopen(data_filename, "rb");
    if (data_file == NULL){
        printf("Couldn't open file %s\n", data_filename);
        exit(1);
    }
    int n = fread((char*)data, sizeof(short int), data_x*data_y*data_z, data_file);
    if (n < data_x*data_y*data_z){
        printf("Error reading data file %s, read %d / %d\n", data_filename,
                n, data_x*data_y*data_z*sizeof(short int));
        exit(1);
    }
    fclose(data_file);

    // set up vertex buffer
    vertexBuffer = (float *) malloc(4*data_x*data_y*data_z*sizeof(float));
    if (!vertexBuffer){
        printf("Couldn't alloc vertex buffer!\n");
        exit(1);
    }

    int index = 0;
    for (int f=0; f<data_z; f++){ 
        for (int y=0; y<data_y; y++){ 
            for (int x=0; x<data_x; x++){   
                vertexBuffer[index*4] = ((float)x)/((float)data_x);
                vertexBuffer[index*4+1] = ((float)y)/((float)data_y);
                vertexBuffer[index*4+2] = -2. + ((float)f)/((float)data_z);
                char * tmp = (char *)(&vertexBuffer[index*4+3]);
                tmp[0] = 200;
                tmp[1] = 150;
                tmp[2] = 150;
                tmp[3] = data[index];
                index++;
            }
        }
    }
    glGenBuffers( 1, VBO );
    glBindBuffer( GL_ARRAY_BUFFER, *VBO );
    glBufferData( GL_ARRAY_BUFFER, data_x*data_y*data_z*16, vertexBuffer, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    printf("Read in.\n");
}