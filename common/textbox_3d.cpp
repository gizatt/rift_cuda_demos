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
                float width , float height, float depth, float line_width) :
    _text(text),
    _width(width),
    _height(height),
    _depth(depth),
    _pos(initpos) {
    _rot = Quaternionf::FromTwoVectors(Vector3f(0.0, 0.0, 1.0), initfacedir);
    _line_width = (GLfloat) line_width;
}
Textbox_3D::Textbox_3D(string& text, Vector3f& initpos, Quaternionf& initquat, 
                float width , float height, float depth, float line_width) :
    _text(text),
    _width(width),
    _height(height),
    _depth(depth),
    _pos(initpos){
    _rot = initquat;
    _line_width = (GLfloat) line_width;
}

void Textbox_3D::set_pos( Vector3f& newpos ){
    _pos = newpos;
}

void Textbox_3D::set_facedir( Vector3f& newfacedir ){
    _rot = Quaternionf::FromTwoVectors(Vector3f(0.0, 0.0, 1.0), newfacedir);
}

void Textbox_3D::draw( Vector3f& up_dir ){

    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const float partColor[]     = {0.6f, 0.8f, 1.0f, 0.3f};
    const float partSpecular[]  = {0.6f, 0.8f, 1.0f, 0.3f};
    const float partShininess[] = {0.2f};
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, partColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, partSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, partShininess);

    glPushMatrix();

    glTranslatef(_pos.x(), _pos.y(), _pos.z());
    AngleAxisf roti = AngleAxisf(_rot);
    glRotatef(roti.angle()*180./M_PI, roti.axis().x(), roti.axis().y(), roti.axis().z() );
    // rotate it back upright
    Vector3f mod_up_dir = roti.inverse()*up_dir;
    float s = (mod_up_dir.cross(Vector3f(0.0, 1.0, 0.0))).norm();
    float c = mod_up_dir.dot(Vector3f(0.0, 1.0, 0.0));
    float angle = atan2(s, c);
    if (mod_up_dir.x() > 0)
        angle *= -1.;
    glRotatef(angle*180./M_PI, 0.0f, 0.0f, 1.0f );

    // draw the six boundary quads
    // bottom
    glColor4f(0.6f, 0.8f, 1.0f, 0.3f);
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
    glTranslatef(-_width/3.0, -_height/3.0, _depth/1.99);

    float vscale = _height * 0.005;
    float hscale = _width * 0.006 / ((float)_text.size());
    glScalef(hscale,vscale,1);
    glEnable(GL_COLOR_MATERIAL);
    glColor4f(0.95, 1.0, 1.0, 0.8);
    GLfloat old_line_width; glGetFloatv(GL_LINE_WIDTH, &old_line_width);
    glLineWidth(_line_width);
    for (int i=0; i<_text.size(); i++){
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, _text[i]);
    }
    glLineWidth(old_line_width);
    glColor3f(1.0, 1.0, 1.0);
    glDisable(GL_COLOR_MATERIAL);

    glPopMatrix();
    glEnable(GL_LIGHTING);
}