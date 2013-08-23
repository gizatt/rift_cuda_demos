/* #########################################################################
        General utility functions used by common classes in here.
            They're all conveniences...

        Much reference to:
            http://www.lighthouse3d.com/tutorials/glsl-core-tutorial/
                creating-a-shader/
            http://www.cs.unm.edu/~angel/BOOK/INTERACTIVE_COMPUTER_GRAPHICS/
                FOURTH_EDITION/PROGRAMS/GLSL_example
            And the source code from Caltech's 2013 CS179 class.

   Rev history:
     Gregory Izatt  20130805    Init revision
   ######################################################################### */ 

#include "xen_utils.h"
using namespace std;
using namespace xen_rift;
using namespace Eigen;

// Converts Eigen Quaternion to a Vector3f of Euler Angles
// adapted from sixense_math file of the sixense SDK
Vector3f xen_rift::getEulerAnglesFromQuat(Quaternionf& input){
    //printf("Input: %f %f %f %f\n", input.w(), input.x(), input.y(), input.z());
    Vector3f retval;
    Matrix3f cols=input.toRotationMatrix();
    float h, p, r;
    float A, B;

    B = cols(1, 2);
    p = asinf( B );
    A = cosf( p );


    if( fabs( A ) > 0.005f ) {
        h = atan2f( -cols(0, 2)/A, cols(2, 2)/A ); // atan2( D, C )
        r = atan2f( -cols(1, 0)/A, cols(1, 1)/A ); // atan2( F, E )
    } else {
        h = 0;
        r = atan2f( cols(2, 1), cols(2, 0) ); // atan2( F, E ) when B=0, D=1
    }

    retval[1] = -h;
    retval[0] = -p;
    retval[2] = -r;

    //printf("output: %f %f %f\n", retval[0], retval[1], retval[2]);
    return retval;
}
// Returns elapsed time in ms since last call to this function with the
//  given index, or 0 if this is the first time that index has been used
//  or if the index is out of range.
unsigned long lastTicks_elapsed[NUM_GET_ELAPSED_INDICES] = {};
unsigned long perfFreq;
int xen_rift::init_get_elapsed(){
    //Set up timer
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li)){
        printf("QueryPerformanceFrequency failed!\n");
        return -1;
    }
    perfFreq = (unsigned long)(li.QuadPart);
    return 0;
}
unsigned long xen_rift::get_elapsed(int index){
    if (index < 0 || index >= NUM_GET_ELAPSED_INDICES){
        return 0;
    }
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    unsigned long currTicks = (unsigned long)(li.QuadPart);
    if (lastTicks_elapsed[index] == 0){
        lastTicks_elapsed[index] = currTicks;
        return 0;
    }
    double elapsed = (((double)(currTicks - lastTicks_elapsed[index]))/((double)perfFreq))*1000;
    lastTicks_elapsed[index] = currTicks;
    return elapsed;
}

//--------------------------------------------------------------------------
// Prints an info log regarding the creation of a vertex or fragment shader
//  CS179 2013 Caltech
//--------------------------------------------------------------------------
void xen_rift::printShaderInfoLog(GLuint obj)
{
    GLint infologLength = 0;
    GLint charsWritten  = 0;
    char *infoLog;
    
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
    
    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n",infoLog);
        free(infoLog);
    }
}

// loads shader from file
void xen_rift::load_shaders(char * vertexFileName, GLuint * vshadernum,
                              char * fragmentFileName, GLuint * fshadernum,
                              char * geomFileName, GLuint * gshadernum){
    GLuint status = 0;

    *vshadernum = glCreateShader(GL_VERTEX_SHADER);
    *fshadernum = glCreateShader(GL_FRAGMENT_SHADER);

    // read the shader sources from their files
    char * vs = textFileRead(vertexFileName);
    if (vs == NULL){
        printf("Invalid vert shader filename: %s\n", vertexFileName);
        exit(1);
    }
    char * fs = textFileRead(fragmentFileName);
    if (vs == NULL){
        printf("Invalid frag shader filename: %s\n", fragmentFileName);
        exit(1);
    }
    // conversions to fit the next function
    const char *vv = vs;
    const char *fv = fs;

    // pass the source text to GL
    glShaderSource(*vshadernum, 1, &vv,NULL);
    glShaderSource(*fshadernum, 1, &fv,NULL);

    // finally compile the shader
    glCompileShader(*vshadernum);
    printShaderInfoLog(*vshadernum);
    glCompileShader(*fshadernum);
    printShaderInfoLog(*fshadernum);

    // free the memory from the source text
    free(vs);
    free(fs);

    // geometry if passed
    if (geomFileName) {
        *gshadernum = glCreateShader(GL_GEOMETRY_SHADER);
        char * gs = textFileRead(geomFileName);
        if (gs == NULL){
            printf("Invalid geom shader filename: %s\n", geomFileName);
            exit(1);
        }
        const char *gv = gs;
        glShaderSource(*gshadernum, 1, &gv, NULL);
        glCompileShader(*gshadernum);
        printShaderInfoLog(*gshadernum);
        free(gs);
    }
}

// reads file as one large string
char * xen_rift::textFileRead(char *fn) {
 
    FILE *fp;
    char *content = NULL;
 
    int count=0;
 
    if (fn != NULL) {
        fp = fopen(fn,"rt");
 
        if (fp != NULL) {
 
            fseek(fp, 0, SEEK_END);
            count = ftell(fp);
            rewind(fp);
 
            if (count > 0) {
                content = (char *)malloc(sizeof(char) * (count+1));
                count = fread(content,sizeof(char),count,fp);
                content[count] = '\0';
            }
            fclose(fp);
        }
    }
    return content;
}

//--------------------------------------------------------------------------
// Runs the current shader program/texture state/render target across
// a fullscreen quad.  Primarily useful for post-processing or GPGPU.
//  CS179 2013 Caltech
//--------------------------------------------------------------------------
void xen_rift::renderFullscreenQuad()
{
    // Make the screen go from 0,1 in the x and y direction, with no
    // frustum effect (that is, increasing z doesn't shrink x and y).
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    
    // Don't do any model transformation.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Draw a fullscreen quad with appropriate tex coords.
    glBegin(GL_POLYGON);
    glTexCoord2f(0, 0);
    glVertex3f(0, 0, 0);
    
    glTexCoord2f(0, 1);
    glVertex3f(0, 1, 0);
    
    glTexCoord2f(1, 1);
    glVertex3f(1, 1, 0);
    
    glTexCoord2f(1, 0);
    glVertex3f(1, 0, 0);
    glEnd();
    
    // Restore the modelview matrix.
    glPopMatrix();
    
    // Restore the projection matrix.
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    // Always good practice to get back to modelview mode at all times.
    glMatrixMode(GL_MODELVIEW);
}

// Loads textures for a skybox from a given base string. Returns 0 if 
// successful, 1 if not. Order that they come out in the array:
//  right left top bottom front back
static int loadSkyBoxHelper(char * tmpstring, GLuint * out){
    *out = SOIL_load_OGL_texture
    (
        tmpstring,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );
    if( 0 == *out )
    {
        printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, *out);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
}
int xen_rift::loadSkyBox(char * base_str, GLuint outs[6]){

    char tmpstring[100];
    glEnable (GL_TEXTURE_2D);

    // Right
    sprintf(tmpstring, "../resources/%s_right1.png", base_str);
    if (loadSkyBoxHelper(tmpstring, &outs[0]))
        return -1;
    // Left
    sprintf(tmpstring, "../resources/%s_left2.png", base_str);
    if (loadSkyBoxHelper(tmpstring, &outs[1]))
        return -1;
    // Top
    sprintf(tmpstring, "../resources/%s_top3.png", base_str);
    if (loadSkyBoxHelper(tmpstring, &outs[2]))
        return -1;
    // Bottom
    sprintf(tmpstring, "../resources/%s_bottom4.png", base_str);
    if (loadSkyBoxHelper(tmpstring, &outs[3]))
        return -1;
    // Front
    sprintf(tmpstring, "../resources/%s_front5.png", base_str);
    if (loadSkyBoxHelper(tmpstring, &outs[4]))
        return -1;
    // Back
    sprintf(tmpstring, "../resources/%s_back6.png", base_str);
    if (loadSkyBoxHelper(tmpstring, &outs[5]))
        return -1;

    return 0;
}