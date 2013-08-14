/* #########################################################################
        simple_particle_swirl: Oculus Rift CUDA-powered demo 
   
   Interfaces with the RIFT SDK and CUDA to demo... well,
   both. Fancy particle graphics, what's not to like!

   Organized fairly haphazardly at the moment (class-ification to come
   in the future, probably), but it's based on glut's callback system.
   Function prototypes are listed below; main() does primarily initialization
   of the various helper classes and rift sdk things that we need to get
   up and running, sets up glut, and goes. Other functions do what they
   advertise, either drawing, passing idle ticks to our helper objects,
   passing input to helper objects, etc.

   Rev history:
     Gregory Izatt  20130717  Init revision
   ######################################################################### */    

#include "Eigen/Dense"
#include "Eigen/Geometry"

// Us!
#include "simple_particle_swirl.h"

// And a helper player class
#include "../common/player.h"
#include "../common/rift.h"
#include "../common/hydra.h"

// handy image loading
#include "../include/SOIL.h"

// menus!
#include "../common/textbox_3d.h"

// use protection guys
using namespace std;
using namespace OVR;
using namespace xen_rift;

//GLUT:
int screenX, screenY;
//Frame counters
int totalFrames = 0;
int frame = 0; //Start with frame 0
unsigned long lastTicks_framerate;
unsigned long lastTicks_elapsed;
double currFrameRate = 0;
unsigned long perfFreq;

// particle vbo
GLuint vbo[1];

// ground and sky tex
GLuint ground_tex;
GLuint sky_tex[6];

/*
Ptr<DeviceManager> pManager;
Ptr<HMDDevice> pHMD;
Ptr<SensorDevice> pSensor;
HMDInfo hmd;
SensorFusion SFusion;
*/

//Player manager
Player * player_manager;
//Rift
Rift * rift_manager;
//Hydra
Hydra * hydra_manager;

// debug
Textbox_3D test_textbox(std::string("Hello!"), Eigen::Vector3f(0.0f, 1.0f, 0.0f), Eigen::Vector3f(0.0f, 0.0f, 1.0f), 
                        1.0f, 0.8f, 0.2f);

/* #########################################################################
    
                            forward declarations
                            
   ######################################################################### */        
// opengl initialization
void initOpenGL(int w, int h, void*d);
//    GLUT display callback -- updates screen
void glut_display();
// Helper to set up lighting
void draw_setup_lighting();
// Helper to draw the skybox
void draw_demo_skybox();
// Helper to draw the demo room itself
void draw_demo_room();
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

// Set up CUDA; mainly makes and initializes shared buffers
extern  int initCuda(GLuint * vbo);
// Call kernel and advance particle swirl in time; pass in player eye pos
//  to do rough lighting (WIP)
extern void advance_particle_swirl(GLuint * vbo, float px, float py, float pz);


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
        if (strcmp(argv[i],"-nohydra") == 0) {
            use_hydra = false;
            printf("No Hydra.\n"); } 
        else if (strcmp(argv[i],"-verbose") == 0) {
            verbose = false;
            printf("Verbose printouts.\n"); } 
        else {
            printf("Usage:\n");
            printf("    * -nohydra | Don't wait for a Razer Hydra to show up.\n");
            printf("    * -verbose | Verbose printouts system-wide.\n");
            return 0;
        }
    }
    
    printf("Initializing... ");
    srand(time(0));
    //Set up timer
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
        printf("QueryPerformanceFrequency failed!\n");
    perfFreq = (unsigned long)(li.QuadPart);

    /*
    pManager = *DeviceManager::Create();
    printf("pManager: %p\n", pManager);
    if (pManager){
        pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
        printf("2/5\n");
        if (pHMD->GetDeviceInfo(&hmd))
        {
            char * MonitorName = hmd.DisplayDeviceName;
            printf("%s\n", MonitorName);
        }
        printf("3\n");
        if (pSensor)
            SFusion.AttachToSensor(pSensor);
        printf("4\n");
    }
    */

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

    // get helpers set up now that opengl is up
    Eigen::Vector3f zeropos = Eigen::Vector3f();
    Eigen::Vector2f zerorot = Eigen::Vector2f();
    player_manager = new Player(zeropos, zerorot, 1.6);
    //Rift
    rift_manager = new Rift(1280, 720, true);
    hydra_manager = new Hydra(use_hydra, verbose);

    //Main loop!
    printf("done!\n");
    glutMainLoop();

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

    // Load in ground texture
    glEnable (GL_TEXTURE_2D);
    ground_tex = SOIL_load_OGL_texture
    (
        "../resources/groundgrid.bmp",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );
    if( 0 == ground_tex )
    {
        printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
    }
    glBindTexture(GL_TEXTURE_2D, ground_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Load skybox
    if (loadSkyBox("starsky", sky_tex))
        exit(1);

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
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glEnable (GL_BLEND);
    //wglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Clear viewport
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    
    // Set up CUDA
    initCuda(vbo);

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

    // bring in vbo
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    // and get player location
    Eigen::Vector3f curr_translation = player_manager->get_position();
    Eigen::Vector2f curr_rotation = player_manager->get_rotation();

    Eigen::Vector3f curr_offset = hydra_manager->getCurrentPos('r')*1.0/1000.0;
    Eigen::Vector3f curr_offset_rpy = hydra_manager->getCurrentRPY('r');
    Vector3f curr_t_vec(curr_translation.x(), curr_translation.y(), curr_translation.z());
    Vector3f curr_r_vec(0.0f, curr_rotation.y()*M_PI/180.0, 0.0f);
    Vector3f curr_o_vec(curr_offset.x(), curr_offset.y(), curr_offset.z());
    Vector3f curr_ro_vec(curr_offset_rpy.y(), curr_offset_rpy.x(), curr_offset_rpy.z());

    // Go do Rift rendering!
    rift_manager->render(curr_t_vec, curr_r_vec+curr_ro_vec, curr_o_vec, true, render_core);

    // free vbo for CUDA
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    double curr = get_framerate();
    if (currFrameRate != 0.0f)
        currFrameRate = (10.0f*currFrameRate + curr)/11.0f;
    else
        currFrameRate = curr;

    //output useful framerate and status info:
    //printf ("framerate: %3.1f / %4.1f\n", curr, currFrameRate);
    // frame was rendered, give the player handler a tick
    player_manager->on_frame_render();

    totalFrames++;
}

/* #########################################################################
    
                                draw_setup_lighting
                                            
        -Helper to setup demo lighting each frame.
   ######################################################################### */
void draw_setup_lighting(){
    //Define lighting
    GLfloat amb[]= { 0.7f, 0.7f, 0.8f, 1.0f };
    GLfloat diff[]= { 0.8f, 0.8f, 1.0f, 1.0f };
    GLfloat spec[]= { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat lightpos[]= { 10.0f, 5.0f, 10.0f, 1.0f };
    GLfloat linearatten[] = {0.001f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, linearatten);
    glEnable(GL_LIGHT0);
}

/* #########################################################################
    
                                draw_demo_skybox
                                            
        -Helper to draw the demo's skybox.
        Reference to
            http://stackoverflow.com/questions/2859722/
            opengl-how-can-i-put-the-skybox-in-the-infinity
   ######################################################################### */
void draw_demo_skybox(){
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    float cxl = -500.0;
    float cxu = 500.0;
    float cyl = -500.0;
    float cyu = 500.0;
    float czl = -500.0;
    float czu = 500.0;

    // ceiling (-y)
    glBindTexture(GL_TEXTURE_2D, sky_tex[3]);
    glBegin(GL_QUADS);
    glTexCoord2f (0., 0.);
    glVertex3f(cxl,cyl,czl);
    glTexCoord2f (1., 0.);
    glVertex3f(cxu,cyl,czl);
    glTexCoord2f (1., 1.);
    glVertex3f(cxu,cyl,czu);
    glTexCoord2f (0., 1.);
    glVertex3f(cxl,cyl,czu);
    glEnd();

    // ceiling (+y)
    glBindTexture(GL_TEXTURE_2D, sky_tex[2]);
    glBegin(GL_QUADS);
    glTexCoord2f (0., 1.);
    glVertex3f(cxl,cyu,czl);
    glTexCoord2f (1., 1.);
    glVertex3f(cxu,cyu,czl);
    glTexCoord2f (1., 0.);
    glVertex3f(cxu,cyu,czu);
    glTexCoord2f (0., 0.);
    glVertex3f(cxl,cyu,czu);
    glEnd();

    // -x wall
    glBindTexture(GL_TEXTURE_2D, sky_tex[1]);
    glBegin(GL_QUADS);
    glTexCoord2f (0., 0.);
    glVertex3f(cxl,cyu,czu);
    glTexCoord2f (1., 0.);
    glVertex3f(cxl,cyu,czl);
    glTexCoord2f (1., 1.);
    glVertex3f(cxl,cyl,czl);
    glTexCoord2f (0., 1.);
    glVertex3f(cxl,cyl,czu);
    glEnd();
    
    // +x wall
    glBindTexture(GL_TEXTURE_2D, sky_tex[0]);
    glBegin(GL_QUADS);
    glTexCoord2f (0., 1.);
    glVertex3f(cxu,cyl,czl);
    glTexCoord2f (1., 1.);
    glVertex3f(cxu,cyl,czu);
    glTexCoord2f (1., 0.);
    glVertex3f(cxu,cyu,czu);
    glTexCoord2f (0., 0.);
    glVertex3f(cxu,cyu,czl);
    glEnd();

    // -z wall
    glBindTexture(GL_TEXTURE_2D, sky_tex[4]);
    glBegin(GL_QUADS);
    glTexCoord2f (0., 0.);
    glVertex3f(cxl,cyu,czl);
    glTexCoord2f (1., 0.);
    glVertex3f(cxu,cyu,czl);
    glTexCoord2f (1., 1.);
    glVertex3f(cxu,cyl,czl);
    glTexCoord2f (0., 1.);
    glVertex3f(cxl,cyl,czl);
    glEnd();
    // +z wall
    glBindTexture(GL_TEXTURE_2D, sky_tex[5]);
    glBegin(GL_QUADS);
    glTexCoord2f (0., 0.);
    glVertex3f(cxu,cyu,czu);
    glTexCoord2f (0., 1.);
    glVertex3f(cxu,cyl,czu);
    glTexCoord2f (1., 1.);
    glVertex3f(cxl,cyl,czu);
    glTexCoord2f (1., 0.);
    glVertex3f(cxl,cyu,czu);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

/* #########################################################################
    
                                draw_demo_room
                                            
        -Helper to draw demo room: basic walls, floor, environ, etc.
            If this becomes fancy enough / dynamic I may go throw it in
            its own file.

   ######################################################################### */
void draw_demo_room(){
    const float groundColor[]     = {0.7f, 0.7f, 0.7f, 1.0f};
    const float groundSpecular[]  = {0.1f, 0.1f, 0.1f, 1.0f};
    const float groundShininess[] = {0.2f};
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ground_tex);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, groundColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, groundSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, groundShininess);

    glEnable(GL_LIGHTING);
    // Floor; tesselate this nicely so lighting affects it
    for (float i=-10.; i<10.; i+=1.){
        for (float j=-10.; j<10.; j+=1.){
            glBegin(GL_QUADS);
            glNormal3f(0., 1.0, 0.);
            glTexCoord2f (-1., -1.);
            glVertex3f(3.*i,-0.1,3.*j);
            glTexCoord2f (1., -1.);
            glVertex3f(3.*i+3.,-0.1,3.*j);
            glTexCoord2f (1., 1.);
            glVertex3f(3.*i+3.,-0.1,3.*j+3.);
            glTexCoord2f (-1., 1.);
            glVertex3f(3.*i,-0.1,3.*j+3.);
            glEnd();
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

/* #########################################################################
    
                                render_core
        Render functionality shared between eyes.

   ######################################################################### */
void render_core(){

    // first draw skybox
    draw_demo_skybox();

    // then rest
    draw_setup_lighting();
    glEnable(GL_LIGHTING);

    const float partColor[]     = {0.4f, 0.1f, 0.3f, 1.0f};
    const float partSpecular[]  = {0.1f, 0.1f, 0.1f, 1.0f};
    const float partShininess[] = {0.1f};

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, partColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, partSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, partShininess);
    // render from the vbo
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    // Each 8-byte vertex in that buffer includes coodinate information
    //  and color information:
    //  byte index [ 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  ]
    //  info       [ <x, float>  <y, float>  <z, float >   <r   g   b   a> ]
    // These cmds instruct opengl to expect that:
    glColorPointer(4,GL_UNSIGNED_BYTE,16,(void*)12);
    glVertexPointer(3,GL_FLOAT,16,(void*)0);
    glDrawArrays(GL_POINTS,0, NUM_PARTICLES);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glPushMatrix();
    draw_demo_room();
    glPopMatrix();

    // draw in front-guide
    glPushMatrix();
    player_manager->draw_HUD();
    glPopMatrix();

    // and menu
    test_textbox.draw();

    glDisable(GL_LIGHTING);
}

/* #########################################################################
    
                                glut_idle
                                            
        -Callback from GLUT: called as the idle function, as rapidly
            as possible.
        - Advances particle swarm.

   ######################################################################### */    
void glut_idle(){

    Eigen::Vector3f tpos = player_manager->get_position();

    advance_particle_swirl(vbo, tpos.x(), tpos.y(), tpos.z());

    // and let rift handler update
    rift_manager->onIdle();
    hydra_manager->onIdle();

    glutPostRedisplay();
}


/* #########################################################################
    
                              normal_key_handler
                              
        -GLUT callback for non-special (generic letters and such)
          keypresses and releases.
        
   ######################################################################### */    
void normal_key_handler(unsigned char key, int x, int y) {
    player_manager->normal_key_handler(key, x, y);
    rift_manager->normal_key_handler(key, x, y);
    hydra_manager->normal_key_handler(key, x, y);
    switch (key) {
        default:
            break;
    }
}
void normal_key_up_handler(unsigned char key, int x, int y) {
    player_manager->normal_key_up_handler(key, x, y);
    rift_manager->normal_key_up_handler(key, x, y);
    hydra_manager->normal_key_up_handler(key, x, y);
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
    player_manager->special_key_handler(key, x, y);
    rift_manager->special_key_handler(key, x, y);
    hydra_manager->special_key_handler(key, x, y);
    switch (key) {
        default:
            break;
    }
}
void special_key_up_handler(int key, int x, int y){
    player_manager->special_key_up_handler(key, x, y);
    rift_manager->special_key_up_handler(key, x, y);
    hydra_manager->special_key_up_handler(key, x, y);
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
    player_manager->mouse(button, state, x, y);
    rift_manager->mouse(button, state, x, y);
    hydra_manager->mouse(button, state, x, y);
}


/* #########################################################################
    
                                    motion
                              
        -GLUT callback for mouse motion
        -When the mouse moves and various buttons are down, adjusts camera.
        
   ######################################################################### */    
void motion(int x, int y){
    player_manager->motion(x, y);
    rift_manager->motion(x, y);
    hydra_manager->motion(x, y);
}


/* #########################################################################
    
                                get_framerate
                                            
        -Takes totalFrames / ((curr time - start time)/CLOCKS_PER_SEC)
            INDEPENDENT OF GET_ELAPSED; USING THESE FUNCS FOR DIFFERENT
                TIMING PURPOSES
   ######################################################################### */     
double get_framerate ( ) {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    unsigned long currTicks = (unsigned long)(li.QuadPart);
    unsigned long elapsed = currTicks - lastTicks_framerate;
    double ret;
    if (elapsed != 0){
        ret = (((double)totalFrames) / (((double)elapsed)/((double)perfFreq)));
    }else{
        ret =  -1;
    }
    totalFrames = 0;
    lastTicks_framerate = currTicks;
    return ret;
}


/* #########################################################################
    
                                get_elapsed
                                            
        -Returns number of milliseconds since last call to this func
            as a double
        
   ######################################################################### */ 
double get_elapsed(){
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    unsigned long currTicks = (unsigned long)(li.QuadPart);
    double elapsed = (((double)(currTicks - lastTicks_elapsed))/((double)perfFreq))*1000;
    lastTicks_elapsed = currTicks;
    return elapsed;
}