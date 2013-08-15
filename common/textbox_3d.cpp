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

void Textbox_3D::set_pos( Vector3f& newpos ){
    _pos = newpos;
}

void Textbox_3D::draw( void ){

    glEnable(GL_LIGHTING);
    const float partColor[]     = {0.7f, 0.7f, 0.7f, 1.0f};
    const float partSpecular[]  = {0.1f, 0.1f, 0.1f, 1.0f};
    const float partShininess[] = {0.2f};
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, partColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, partSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, partShininess);

    glPushMatrix();

    glTranslatef(_pos.x(), _pos.y(), _pos.z());
    Quaternionf roti = _rot.inverse();
    glRotatef(roti.w(), roti.x(), roti.y(), roti.z() );

    // draw the six boundary quads
    // bottom
    glColor3f(0.7, 0.7, 0.7);
    glBegin(GL_QUADS);
    glNormal3f(0., -1.0, 0.);
    glVertex3f(-_width/2.0, -_height/2.0, -_depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, -_depth/2.0);
    glEnd();
    // top
    glBegin(GL_QUADS);
    glNormal3f(0., 1.0, 0.);
    glVertex3f(-_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(-_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, -_depth/2.0);
    glEnd();
    // left
    glBegin(GL_QUADS);
    glNormal3f(-1.0, 0., 0.);
    glVertex3f(-_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(-_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, -_depth/2.0);
    glEnd();
    // right
    glBegin(GL_QUADS);
    glNormal3f(1.0, 0., 0.);
    glVertex3f(_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, -_depth/2.0);
    glEnd();
    // forward
    glBegin(GL_QUADS);
    glNormal3f(0., 0., -1.0);
    glVertex3f(-_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, -_depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, -_depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, -_depth/2.0);
    glEnd();
    // back
    glBegin(GL_QUADS);
    glNormal3f(0., 0., 1.0);
    glVertex3f(-_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, _height/2.0, _depth/2.0);
    glVertex3f(_width/2.0, -_height/2.0, _depth/2.0);
    glVertex3f(-_width/2.0, -_height/2.0, _depth/2.0);
    glEnd();
    // and the text
    glDisable(GL_LIGHTING);
    glTranslatef(-_width/3.0, 0.0, _depth/1.99);
    glScalef(0.001,0.001,1);
    glEnable(GL_COLOR_MATERIAL);
    glColor3f(0.8, 0.2, 0.2);
    for (int i=0; i<_text.size(); i++){
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, _text[i]);
    }
    glColor3f(1.0, 1.0, 1.0);
    glDisable(GL_COLOR_MATERIAL);

    glPopMatrix();
}