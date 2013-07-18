/* #########################################################################
        simple_particle_swirl: Oculus Rift CUDA-powered demo 
   
   Interfaces with the RIFT SDK and CUDA to demo... well,
   both. Fancy particle graphics, what's not to like!

   read_from_due mockup to test reading from the arduino. 

   Rev history:
     Gregory Izatt  20130717  Init revision
   ######################################################################### */    

// Us!
#include "simple_particle_swirl.h"

// use protection guys
using namespace std;
using namespace OVR;

//GLUT:
// mouse controls (copied from CUDA SDK ocean example)
int mouseOldX, mouseOldY;
int mouseButtons = 0;
float rotateX = 20.0f, rotateY = 0.0f;
float translateX = 0.0f, translateY = 0.0f, translateZ = -2.0f;
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

//Rift
Ptr<DeviceManager> pManager;
Ptr<HMDDevice> pHMD;
Ptr<SensorDevice> pSensor;
HMDInfo hmd;
SensorFusion SFusion;

/* #########################################################################
    
                            forward declarations
                            
   ######################################################################### */        
// opengl initialization
void initOpenGL(int w, int h, void*d);
//    GLUT display callback -- updates screen
void glut_display();
// GLUT idle callback -- launches a CUDA analysis cycle
void glut_idle();
//GLUT resize callback
void resize(int width, int height);
//Key handlers and mouse handlers; all callbacks for GLUT
void normal_key_handler(unsigned char key, int x, int y);
void special_key_handler(int key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);

// Get our framerate
double get_framerate();
// Return curr time in ms since last call to this func (high res)
double get_elapsed();

// And the magnificent kernel!
__global__ void d_simple_particle_swirl( float4* pos, float4* vels, unsigned int N, float dt); 

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
    if (argc!=1){
        printf("Usage: no arguments. That's it ¯\\_(ツ)_/¯\n");
        exit(1);
    }
    
    printf("Initializing... ");
    srand(time(0));
    //Set up timer
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
        printf("QueryPerformanceFrequency failed!\n");
    perfFreq = (unsigned long)(li.QuadPart);
    //Rift init
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

    //Go get openGL set up / get the critical glob. variables set up
    initOpenGL(1024, 768, NULL);


    //Gotta register our callbacks
    glutIdleFunc( glut_idle );
    glutDisplayFunc( glut_display );
    glutKeyboardFunc ( normal_key_handler );
    glutSpecialFunc ( special_key_handler );
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(resize);

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
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Clear viewport
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    
    //And set up shared vertex buffer
    glGenBuffers( 1, vbo );
    glBindBuffer( GL_ARRAY_BUFFER, *vbo );
    float4* temppos = (float4*)malloc(BUFFER_SIZE);
    if (!temppos){ printf("Memory alloc error.\n"); exit(1);}
    for(int i = 0; i < NUM_PARTICLES; i++)
    {
        /* Initial position within 0.05 of <0, 0.5, 0> */
        temppos[i].x = ((float)rand())/RAND_MAX * 0.1 - 0.05;
        temppos[i].y = ((float)rand())/RAND_MAX * 0.1 + 0.45;
        temppos[i].z = ((float)rand())/RAND_MAX * 0.1 - 0.05;
        temppos[i].w = 1.0;
    }
    glBufferData( GL_ARRAY_BUFFER, BUFFER_SIZE, temppos, GL_DYNAMIC_DRAW );
    // (whenever I bind buffer index 0, that's just the way of unbinding
    //     openGL from any buffer...)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    free(temppos);
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
        /* Initial velocity near <1, 0, 0> */
        tempvel[i].x = ((float)rand())/RAND_MAX * 0.15 + 0.950;
        tempvel[i].y = ((float)rand())/RAND_MAX * 0.05 - 0.025;
        tempvel[i].z = ((float)rand())/RAND_MAX * 0.05 - 0.025;
        tempvel[i].w = 1.0;
    }
    CUDA_SAFE_CALL( cudaMemcpy( d_velocities, tempvel, BUFFER_SIZE, cudaMemcpyHostToDevice ) );
    free(tempvel);
    //store our screen sizing information
    screenX = glutGet(GLUT_WINDOW_WIDTH);
    screenY = glutGet(GLUT_WINDOW_HEIGHT);



    glFinish();
}

void resize(int width, int height){
    glViewport(0,0,width,height);
    screenX = glutGet(GLUT_WINDOW_WIDTH);
    screenY = glutGet(GLUT_WINDOW_HEIGHT);
}


/* #########################################################################
    
                                glut_display
                                            
        -Callback from GLUT: called whenever screen needs to be
            re-rendered
        -Feeds vertex buffer, along with specifications of what
            is in it, to OpenGL to render.
        
   ######################################################################### */    
void render_core(){

}
void glut_display(){

    //Clear out buffers before rendering the new scene
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // bring in vbo
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);

    //Left viewport:
    glViewport(0,0,screenX/2,screenY);
    // set view matrix
    glMatrixMode(GL_MODELVIEW);
    // reset it to default
    glLoadIdentity();
    // And transform in camera position
    glTranslatef(translateX+0.1, translateY, translateZ);
    glRotatef(rotateX, 1.0, 0.0, 0.0);
    glRotatef(rotateY, 0.0, 1.0, 0.0);
    // render from the vbo
    glVertexPointer(4, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColor3f(1.0, 0.0, 0.0);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    //Right viewport:
    glViewport(screenX/2,0,screenX/2,screenY);
    // set view matrix
    glMatrixMode(GL_MODELVIEW);
    // reset it to default
    glLoadIdentity();
    // And transform in camera position
    glTranslatef(translateX-0.1, translateY, translateZ);
    glRotatef(rotateX, 1.0, 0.0, 0.0);
    glRotatef(rotateY, 0.0, 1.0, 0.0);
    // render from the vbo
    glVertexPointer(4, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColor3f(1.0, 0.0, 0.0);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
    glDisableClientState(GL_VERTEX_ARRAY);

    // swap whole screen
    glutSwapBuffers();   
    // free vbo for CUDA
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    double curr = get_framerate();
    if (currFrameRate != 0.0f)
        currFrameRate = (10.0f*currFrameRate + curr)/11.0f;
    else
        currFrameRate = curr;

    //output useful framerate and status info:
    printf ("framerate: %3.1f / %4.1f\n", curr, currFrameRate);
    
    //Unbind, restore cuda access to buffer, and continue:
    /*
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    HANDLE_ERROR( cudaGraphicsMapResources(1, resources, fftStream) );
    size_t size;
    HANDLE_ERROR( cudaGraphicsResourceGetMappedPointer((void **)(&vertexPointer), &size, resources[0]) );
    
    if (writelog) fprintf(log_file, "%05.5f, Exit refresh_screen\n", get_elapsed());
    if (writelog) fflush(log_file);
    */
    totalFrames++;
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
        d_simple_particle_swirl<<< GRID_SIZE, BLOCK_SIZE >>>(dptr, d_velocities, NUM_PARTICLES, dt);

    // unmap buffer object
    CUDA_SAFE_CALL(cudaGraphicsUnmapResources(1, resources, 0));
    framesRendered++;
    glutPostRedisplay();
}


/* #########################################################################
    
                              normal_key_handler
                              
        -GLUT callback for non-special (generic letters and such)
          keypresses.
        -If escape is pressed, cleans up and exits.
        
   ######################################################################### */    
void normal_key_handler(unsigned char key, int x, int y) {

    switch (key) {
        default:
            printf("Unhandled key %u\n", key);
            break;
    }
}


/* #########################################################################
    
                              special_key_handler
                              
        -GLUT callback for special (arrow keys, F keys, etc)
          keypresses.
        -Binds up/down to adjusting parameters
        
   ######################################################################### */    
void special_key_handler(int key, int x, int y){
    switch (key) {
        default:
            printf("Unhandled key %u\n", key);
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
    if (state == GLUT_DOWN) {
        mouseButtons |= 1<<button;
    } else if (state == GLUT_UP) {
        mouseButtons = 0;
    }

    mouseOldX = x;
    mouseOldY = y;
}


/* #########################################################################
    
                                    motion
                              
        -GLUT callback for mouse motion
        -When the mouse moves and various buttons are down, adjusts camera.
        
   ######################################################################### */    
void motion(int x, int y){
    float dx, dy;
    dx = (float)(x - mouseOldX);
    dy = (float)(y - mouseOldY);

    if (mouseButtons == 1) {
        rotateX += dy * 0.2f;
        rotateY += dx * 0.2f;
    } else if (mouseButtons == 2) {
        translateX += dx * 0.01f;
        translateY -= dy * 0.01f;        
    } else if (mouseButtons == 4) {
        translateZ += dy * 0.01f;
    }

    mouseOldX = x;
    mouseOldY = y;
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
__global__ void d_simple_particle_swirl(float4* pos, float4* vels, unsigned int N, float dt)
{
    // Indices into the VBO data.
    unsigned int i = blockIdx.x*blockDim.x + threadIdx.x;
    if (i < N) {
        /* Update vels to orbit origin */
        float dist2 = pos[i].x*pos[i].x + pos[i].y*pos[i].y + pos[i].z*pos[i].z;
        if (dist2 != 0){
            vels[i].x -= 0.01 * pos[i].x / dist2;
            vels[i].y -= 0.01 * pos[i].y / dist2;
            vels[i].z -= 0.01 * pos[i].z / dist2;
        }
        /* And update position based on velocity */
        pos[i].x += vels[i].x*dt/1000.0;
        pos[i].y += vels[i].y*dt/1000.0;
        pos[i].z += vels[i].z*dt/1000.0;
    }
}
