/* #########################################################################
        Player class -- camera viewport and movement management.

   Rev history:
     Gregory Izatt  20130718  Init revision
   ######################################################################### */    

#include "player.h"
using namespace std;
using namespace xen_rift;

Player::Player(float3 init_position, float2 init_rotation, float init_eye_height) {
  _position = init_position;
  _rotation = init_rotation;
  _eye_height = init_eye_height;
  _position.y = -init_eye_height;
  _mouseButtons = 0;
}
 
void Player::on_frame_render(){
	float3 forward_dir = 0.1*make_float3(-sinf(_rotation.y*M_PI/180.0), 0, cosf(_rotation.y*M_PI/180.0));
	float3 side_dir = 0.1*make_float3(cosf(_rotation.y*M_PI/180.0), 0, sinf(_rotation.y*M_PI/180.0));
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

    if (_mouseButtons == 1) {
        _rotation.x += dy * 0.2f;
        _rotation.y += dx * 0.2f;
    } else if (_mouseButtons == 2) {
        _position.x += dx * 0.01f;
        _position.y -= dy * 0.01f;        
    } else if (_mouseButtons == 4) {
        _position.z += dy * 0.01f;
    }

    _mouseOldX = x;
    _mouseOldY = y;
}
