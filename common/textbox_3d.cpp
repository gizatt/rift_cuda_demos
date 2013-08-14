/* #########################################################################
        Textbox 3D class -- UI atom.

        Manages rendering of a rectangular prism with text printed on 
    one side, with some intersection methods for detecting intersections
    with the box.
    
   Rev history:
     Gregory Izatt  20130813  Init revision
   ######################################################################### */    

#include "textbox_3d.h"
using namespace std;
using namespace xen_rift;
using namespace Eigen;

Textbox_3D::Textbox_3D(string& text, Vector3f& initpos, Vector3f& initfacedir, 
                float width , float height, float depth) :
    _text(text),
    _width(width),
    _height(height),
    _depth(depth),
    _pos(initpos) {
    // init
    _rot = Quaternionf::FromTwoVectors(Vector3f(), initfacedir);
}
Textbox_3D::Textbox_3D(string& text, Vector3f& initpos, Quaternionf& initquat, 
                float width , float height, float depth) :
    _text(text),
    _width(width),
    _height(height),
    _depth(depth),
    _pos(initpos){
    _rot = initquat;
}

void Textbox_3D::draw( void ){
    glPushMatrix();
    glTranslatef(-_pos.x(), -_pos.y(), -_pos.z());
    Quaternionf roti = _rot.inverse();
    glRotatef(roti.w(), roti.x(), roti.y(), roti.z() );

    // draw the six boundary quads
    // bottom
    glBegin(GL_QUADS);
    glVertex3f(-_width/2.0, -_height/2.0, -_depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, -_depth/2.0);
    glEnd();
    // top
    glBegin(GL_QUADS);
    glVertex3f(-_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(-_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, -_depth/2.0);
    glEnd();
    // left
    glBegin(GL_QUADS);
    glVertex3f(-_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(-_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, -_depth/2.0);
    glEnd();
    // right
    glBegin(GL_QUADS);
    glVertex3f(_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, -_depth/2.0);
    glEnd();
    // forward
    glBegin(GL_QUADS);
    glVertex3f(-_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, -_depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, -_depth/2.0);
    glEnd();
    // back
    glBegin(GL_QUADS);
    glVertex3f(_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(-_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, _depth/2.0);
    glEnd();
    // and the text
    glScalef(0.003,0.003,1);
    glTranslatef(-_width/3.0, 0.0, -_depth/1.99);
    for (int i=0; i<_text.size(); i++){
        glutStrokeCharacter(GLUT_STROKE_ROMAN, _text[i]);
        glTranslatef(_width/3.0/(float)_text.size(),0.0, 0.0);
    }
    
    glPopMatrix();
}