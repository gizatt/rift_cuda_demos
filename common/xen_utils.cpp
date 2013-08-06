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

//--------------------------------------------------------------------------
// Prints an info log regarding the creation of a vertex or fragment shader
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