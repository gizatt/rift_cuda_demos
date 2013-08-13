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

// Us!
#include "simple_particle_swirl.h"
// And a helper player class
#include "../common/player.h"
#include "../common/rift.h"
#include "../common/hydra.h"
// handy image loading
#include "../include/SOIL.h"

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

//VBO for particles
// vbo variables
static GLuint vbo[1];
cudaGraphicsResource *resources[1];
// Device buffer variables
float4* d_velocities;

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

// And the magnificent kernel!
__global__ void d_simple_particle_swirl( float4* pos, float4* vels, unsigned int N, float dt,
        float3 player_pos); 

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
    float3 zeropos = float3();
    float2 zerorot = float2();
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

    //Start off by resetting cudaDevice
    cudaDeviceReset();
        
    // first, find a CUDA device and set it to graphic interop
    cudaDeviceProp  prop;
    int dev;
    memset( &prop, 0, sizeof( cudaDeviceProp ) );
    prop.major = 1;
    prop.minor = 0;
    HANDLE_ERROR( cudaChooseDevice( &dev, &prop ) );
    cudaGLSetGLDevice( dev );
        
    
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
    
    //And set up shared vertex buffer
    glGenBuffers( 1, vbo );
    glBindBuffer( GL_ARRAY_BUFFER, *vbo );
    float4* temppos = (float4*)malloc(BUFFER_SIZE);
    if (!temppos){ printf("Memory alloc error.\n"); exit(1);}
    for(int i = 0; i < NUM_PARTICLES; i++)
    {
        /* Initial position in 30-radius ring at y=30 */
        float radius = ((float)rand())/RAND_MAX*27.0+3.0;
        float theta = ((float)rand())/RAND_MAX*2.0*M_PI/8.0;
        temppos[i].x = radius*cosf(theta);
        temppos[i].y = ((float)rand())/RAND_MAX * 1.0 + 29.5;
        temppos[i].z = radius*sinf(theta);
        unsigned char * tmp = (unsigned char *)&(temppos[i].w);
        tmp[0] = (unsigned char) 200;
        tmp[1] = (unsigned char) 200;
        tmp[2] = (unsigned char) 200;
        tmp[3] = 255;
    }
    glBufferData( GL_ARRAY_BUFFER, BUFFER_SIZE, temppos, GL_DYNAMIC_DRAW );
    // (whenever I bind buffer index 0, that's just the way of unbinding
    //     openGL from any buffer...)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (glGetError()){
        unsigned char * glErrorBuffer = (unsigned char *) gluErrorString(glGetError());
        printf("Opengl error: %s\n", glErrorBuffer);
    }
    // register, map and unmap to try to cycle it in...
    CUDA_SAFE_CALL( cudaGraphicsGLRegisterBuffer(resources, *vbo, cudaGraphicsMapFlagsWriteDiscard) );

    // allocate velocity buffer on device side
    CUDA_SAFE_CALL( cudaMalloc( (void**)&d_velocities, BUFFER_SIZE ) );
    float4* tempvel = (float4*)malloc(BUFFER_SIZE);
    if (!tempvel){ printf("Memory alloc error.\n"); exit(1);}
    for(int i = 0; i < NUM_PARTICLES; i++)
    {
        /* Initial velocity around origin at 0, 30, 0 */
        tempvel[i].x = ((float)rand())/RAND_MAX * 2.0 - 1.0 + temppos[i].z;
        tempvel[i].y = ((float)rand())/RAND_MAX * 1.0 - 0.5;
        tempvel[i].z = ((float)rand())/RAND_MAX * 2.0 - 1.0 - temppos[i].x;
        tempvel[i].w = 1.0;
    }
    CUDA_SAFE_CALL( cudaMemcpy( d_velocities, tempvel, BUFFER_SIZE, cudaMemcpyHostToDevice ) );
    free(tempvel);
    free(temppos);
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
    float3 curr_translation = player_manager->get_position();
    float2 curr_rotation = player_manager->get_rotation();

    float3 curr_offset = hydra_manager->getCurrentPos('r')*1.0/1000.0;
    float3 curr_offset_rpy = hydra_manager->getCurrentRPY('r');
    Vector3f curr_t_vec(curr_translation.x, curr_translation.y, curr_translation.z);
    Vector3f curr_r_vec(0.0f, curr_rotation.y*M_PI/180.0, 0.0f);
    Vector3f curr_o_vec(curr_offset.x, curr_offset.y, curr_offset.z);
    Vector3f curr_ro_vec(curr_offset_rpy.y, curr_offset_rpy.x, curr_offset_rpy.z);

    // Go do Rift rendering!
    rift_manager->render(curr_t_vec, curr_r_vec+curr_ro_vec, curr_o_vec, true, render_core);

    /*
    //Left viewport:
    glViewport(0,0,screenX/2,screenY);
    // set view matrix
    glMatrixMode(GL_MODELVIEW);
    // reset it to default
    glLoadIdentity();
    // And transform in camera position
    glRotatef(curr_rotation.x, 1.0, 0.0, 0.0);
    glRotatef(curr_rotation.y, 0.0, 1.0, 0.0);   
    glTranslatef(curr_translation.x-0.1, curr_translation.y, curr_translation.z);
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
    draw_demo_room();
    
    //Right viewport:
    glViewport(screenX/2,0,screenX/2,screenY);
    // set view matrix
    glMatrixMode(GL_MODELVIEW);
    // reset it to default
    glLoadIdentity();
    // And transform in camera position
    glRotatef(curr_rotation.x, 1.0, 0.0, 0.0);
    glRotatef(curr_rotation.y, 0.0, 1.0, 0.0);
    glTranslatef(curr_translation.x+0.1, curr_translation.y, curr_translation.z);
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
    draw_demo_room();


    // swap whole screen
    glutSwapBuffers();   
    */
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

    glDisable(GL_LIGHTING);
}

/* #########################################################################
    
                                glut_idle
                                            
        -Callback from GLUT: called as the idle function, as rapidly
            as possible.
        - Grab VBO control for CUDA, run kernel to advance particles,
            and pass VBO control back when done.

   ######################################################################### */    
int framesRendered = 0;
void glut_idle(){
    // map OpenGL buffer object for writing from CUDA
    float4 *dptr;
    CUDA_SAFE_CALL( cudaGraphicsMapResources(1, resources) );
    size_t size;
    CUDA_SAFE_CALL( cudaGraphicsResourceGetMappedPointer((void **)(&dptr), &size, resources[0]) );
    float dt = (float) get_elapsed();

    // execute the kernel
    if (framesRendered > 0) 
        d_simple_particle_swirl<<< GRID_SIZE, BLOCK_SIZE >>>(dptr, d_velocities, NUM_PARTICLES, dt,
            player_manager->get_position());

    // unmap buffer object
    CUDA_SAFE_CALL(cudaGraphicsUnmapResources(1, resources, 0));

    // and let rift handler update
    rift_manager->onIdle();
    hydra_manager->onIdle();

    framesRendered++;
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




/* #########################################################################
    
                           d_simple_particle_swirl
                                    KERNEL!
        
   ######################################################################### */ 
__global__ void d_simple_particle_swirl(float4* pos, float4* vels, unsigned int N, float dt,
        float3 player_pos)
{
    // Indices into the VBO data.
    unsigned long int i = blockIdx.x*blockDim.x + threadIdx.x;
    if (0 <= i && i < N) {
        /* Update vels to orbit <0, 3, 0> */
        float dist2 = pos[i].x*pos[i].x + pos[i].y*pos[i].y + pos[i].z*pos[i].z;
        if (dist2 != 0){
            vels[i].x -= 10.0 * pos[i].x / dist2;
            vels[i].y -= 10.0 * (pos[i].y - 30.0) / dist2;
            vels[i].z -= 10.0 * pos[i].z / dist2;
        }
        /* And update position based on velocity */
        pos[i].x += vels[i].x*dt/1000.0;
        pos[i].y += vels[i].y*dt/1000.0;
        pos[i].z += vels[i].z*dt/1000.0;
        /* Assign color as a rough diffuse lighting model */
        float3 to_our_pos = normalize(make_float3(pos[i].x - player_pos.x,
                                                  pos[i].y - player_pos.y,
                                                  pos[i].z - player_pos.z));
        float value = 150;
        unsigned char * tmp = (unsigned char *)&(pos[i]);
        tmp[12] = (unsigned char)(value);
        tmp[13] = (unsigned char)(50); 
        tmp[14] = (unsigned char)(10); 
        tmp[15] = (unsigned char) 150;
    }
}
