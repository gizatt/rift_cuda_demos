/* #########################################################################
        simple_particle_swirl: Oculus Rift CUDA-powered demo kernels

   Rev history:
     Gregory Izatt  20130717  Init revision
     Gregory Izatt  20130814  Separating device / host code
   ######################################################################### */    

// Us!
#include "simple_particle_swirl_cu.h"

// use protection guys
using namespace std;
using namespace xen_rift;

//Frame counters
static int totalFrames = 0;
static int frame = 0; //Start with frame 0
static unsigned long lastTicks_framerate;
static unsigned long lastTicks_elapsed;
static double currFrameRate = 0;
static unsigned long perfFreq;

//VBO holder
cudaGraphicsResource *resources[1];
// Device buffer variables
float4* d_velocities;

/* #########################################################################
    
                            forward declarations
                            
   ######################################################################### */        

// The magnificent kernel!
__global__ void d_simple_particle_swirl( float4* pos, float4* vels, unsigned int N, float dt,
        float3 player_pos); 

// Get our framerate
static double get_framerate();
// Return curr time in ms since last call to this func (high res)
static double get_elapsed();


/* #########################################################################
    
                                initCuda
                                            
        -Sets up both a CUDA context
        -Initializes the shared vertex buffer that we'll use, and 
            gets it registered with CUDA
        
            Return -1 if fail, 0 if success.
   ######################################################################### */
int initCuda(GLuint * vbo) {

    srand(time(0));
    //Set up timer
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
        printf("QueryPerformanceFrequency failed!\n");
    perfFreq = (unsigned long)(li.QuadPart);
    
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
    
    //And set up shared vertex buffer
    glGenBuffers( 1, vbo );
    glBindBuffer( GL_ARRAY_BUFFER, *vbo );
    float4* temppos = (float4*)malloc(BUFFER_SIZE);
    if (!temppos){ printf("Memory alloc error.\n"); return -1;}
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
    if (!tempvel){ printf("Memory alloc error.\n"); return -1;}
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

    return 0;
}

/* #########################################################################
    
                             advance_particle_swirl
        - Grab VBO control for CUDA, run kernel to advance particles,
            and pass VBO control back when done.

   ######################################################################### */    
int framesRendered = 0;
void advance_particle_swirl(GLuint * vbo, float px, float py, float pz){
    // map OpenGL buffer object for writing from CUDA
    float4 *dptr;
    CUDA_SAFE_CALL( cudaGraphicsMapResources(1, resources) );
    size_t size;
    CUDA_SAFE_CALL( cudaGraphicsResourceGetMappedPointer((void **)(&dptr), &size, resources[0]) );
    float dt = (float) get_elapsed();

    // execute the kernel
    if ((framesRendered) > 0)
        d_simple_particle_swirl<<< GRID_SIZE, BLOCK_SIZE >>>(dptr, d_velocities, NUM_PARTICLES, dt,
            make_float3(px, py, pz));

    // unmap buffer object
    CUDA_SAFE_CALL(cudaGraphicsUnmapResources(1, resources, 0));
    framesRendered++;
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
    if (i < N) {
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
        float3 to_our_pos = make_float3(pos[i].x - player_pos.x,
                                                  pos[i].y - player_pos.y,
                                                  pos[i].z - player_pos.z);
        float flen = to_our_pos.x*to_our_pos.x + to_our_pos.y*to_our_pos.y + to_our_pos.z*to_our_pos.z;
        if (flen != 0.0){
            to_our_pos.x /= flen;
            to_our_pos.y /= flen;
            to_our_pos.z /= flen;
        }

        float value = 150;
        unsigned char * tmp = (unsigned char *)&(pos[i]);
        tmp[12] = (unsigned char)(value);
        tmp[13] = (unsigned char)(50); 
        tmp[14] = (unsigned char)(10); 
        tmp[15] = (unsigned char) 150;
    }
}

/* #########################################################################
    
                                get_framerate
                                            
        -Takes totalFrames / ((curr time - start time)/CLOCKS_PER_SEC)
            INDEPENDENT OF GET_ELAPSED; USING THESE FUNCS FOR DIFFERENT
                TIMING PURPOSES
   ######################################################################### */     
static double get_framerate ( ) {
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
static double get_elapsed(){
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    unsigned long currTicks = (unsigned long)(li.QuadPart);
    double elapsed = (((double)(currTicks - lastTicks_elapsed))/((double)perfFreq))*1000;
    lastTicks_elapsed = currTicks;
    return elapsed;
}
