/* #########################################################################
        Player class -- camera viewport and movement management.

        Contains methods that map one-to-one with standard glut callbacks;
    can be called on those callbacks (just feed through the arguments) to manage
    a player position using rules built into this class.
        (A good thing to do would be to make some virtual functions that can
    be rewritten in children of this class to do specific rule implementations
    for different motion support. For now, this is a true class, not just a 
    base...)
    
   Rev history:
     Gregory Izatt  20130718  Init revision
   ######################################################################### */    

#include "player.h"
using namespace std;
using namespace xen_rift;
using namespace Eigen;

Player::Player(Vector3f init_position, Vector2f init_rotation, float init_eye_height) {
  _position = init_position;
  _rotation = init_rotation;
  _eye_height = init_eye_height;
  _position.y() = init_eye_height;
  _mouseButtons = 0;
  _w_down = false;
  _s_down = false;
  _a_down = false;
  _d_down = false;
  _c_down = false;
}
 
void Player::on_frame_render(){
    
	Vector3f forward_dir = 0.1*get_forward_dir();
	Vector3f side_dir = 0.1*get_side_dir();

    if (_w_down){
    	_position += forward_dir;
    }
    if (_s_down){
		_position -= forward_dir;
	}
	if (_a_down){
    	_position += side_dir;
    }
    if (_d_down){
    	_position -= side_dir;
    }

}

void Player::normal_key_handler(unsigned char key, int x, int y) {
    switch (key) {
    	case 'w':
    		_w_down = true;
    		break;
    	case 's':
    		_s_down = true;
    		break;
    	case 'a':
    		_a_down = true;
    		break;
    	case 'd':
    		_d_down = true;
    		break;
        case 'c':
            _c_down = true;
            break;
        default:
            break;
    }
}

void Player::normal_key_up_handler(unsigned char key, int x, int y) {
    switch (key) {
    	case 'w':
    		_w_down = false;
    		break;
    	case 's':
    		_s_down = false;
    		break;
    	case 'a':
    		_a_down = false;
    		break;
        case 'd':
            _d_down = false;
            break;
    	case 'c':
    		_c_down = false;
    		break;
        default:
            break;
    }
}

void Player::special_key_handler(int key, int x, int y){
    switch (key) {
        default:
            break;
    }
}

void Player::special_key_up_handler(int key, int x, int y){
    switch (key) {
        default:
            break;
    }
}

void Player::mouse(int button, int state, int x, int y){
    if (state == GLUT_DOWN) {
        _mouseButtons |= 1<<button;
    } else if (state == GLUT_UP) {
        _mouseButtons = 0;
    }

    _mouseOldX = x;
    _mouseOldY = y;
}

void Player::motion(int x, int y){
    float dx, dy;
    dx = (float)(x - _mouseOldX);
    dy = (float)(y - _mouseOldY);

    if (_c_down == false){
        if (_mouseButtons == 1) {
            //_rotation.x += dy * 0.2f;
            _rotation.y() -= dx * 0.2f;
        } else if (_mouseButtons == 2) {
            _position.x() += dx * 0.01f;
            _position.y() -= dy * 0.01f;        
        } else if (_mouseButtons == 4) {
            _position.z() += dy * 0.01f;
        }
    }
    _mouseOldX = x;
    _mouseOldY = y;
}

// empirically derived negatives...
Vector3f Player::get_forward_dir(){
    return -Vector3f(-sinf(-_rotation.y()*M_PI/180.0), 0, cosf(-_rotation.y()*M_PI/180.0));
}
Vector3f Player::get_side_dir(){
    return -Vector3f(cosf(-_rotation.y()*M_PI/180.0), 0, sinf(-_rotation.y()*M_PI/180.0));
}
Vector3f Player::get_up_dir(){
    // always oriented flat, so this is trivial
    return Vector3f(0, 1.0, 0.0);
}

Quaternionf Player::get_quaternion(){
    return Quaternionf(AngleAxisf(_rotation.y()*M_PI/180.0f, Vector3f::UnitY()));
}

void Player::draw_HUD(){
    // draw in a guide pointing the view dir of the player
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    Vector3f fdir = get_forward_dir();
    Vector3f sdir = 0.05*get_side_dir();
    Vector3f udir = 0.05*get_up_dir();
    Vector3f ppos = get_position();
    glColor3f(0.0, 0.3, 0.0);
    glNormal3f(-fdir.x(), -fdir.y(), -fdir.z());
    glVertex3f(ppos.x()+fdir.x()-0.5*sdir.x(), ppos.y()+fdir.y()-0.5*udir.y(), ppos.z()+fdir.z()-0.5*sdir.z());
    glVertex3f(ppos.x()+fdir.x()-0.5*sdir.x(), ppos.y()+fdir.y()+0.5*udir.y(), ppos.z()+fdir.z()-0.5*sdir.z());
    glVertex3f(ppos.x()+fdir.x()+0.5*sdir.x(), ppos.y()+fdir.y()+0.5*udir.y(), ppos.z()+fdir.z()+0.5*sdir.z());
    glVertex3f(ppos.x()+fdir.x()+0.5*sdir.x(), ppos.y()+fdir.y()-0.5*udir.y(), ppos.z()+fdir.z()+0.5*sdir.z());
    glColor3f(1.0f, 1.0f, 1.0f);
    glEnd();
}