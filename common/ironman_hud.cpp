/* #########################################################################
        Ironman HUD Manager

	Provides basis for a helmet HUD around player head position.

   Rev history:
     Gregory Izatt  20130818  Init revision
   ######################################################################### */    

#include "ironman_hud.h"

using namespace std;
using namespace xen_rift;
using namespace Eigen;

Ironman_HUD::Ironman_HUD( float k, float dampening, float tether_dist, float k_o, float dampening_o ) : 
		_k(k), _dampening(dampening), _k_o(k_o), _dampening_o(dampening_o),
		_last_pos( Vector3f( 0.0, 0.0, 0.0) ),
		_last_orientation( Quaternionf(Eigen::AngleAxisf(0.0, Eigen::Vector3f::UnitX())) ),
		 _velocity( Vector3f( 0.0, 0.0, 0.0) ),
		 _velocity_o( Quaternionf(Eigen::AngleAxisf(0.0, Eigen::Vector3f::UnitX())) ),
		 _velocity_rps(0.0f),
		 _tether_dist(tether_dist)
{
	return;
}

void Ironman_HUD::add_textbox( std::string& init_text, Eigen::Vector3f& init_offset_xyz,
								Eigen::Quaternionf& init_offset_quat, float width, 
								float height, float depth, float line_width) {
	_textboxes.push_back(new Textbox_3D(init_text, Vector3f(), Vector3f(), 
						 width, height, depth, line_width));
	_offsets_xyz.push_back(new Vector3f(init_offset_xyz));
	_offsets_quats.push_back(new Quaternionf(init_offset_quat));
}

void Ironman_HUD::onIdle( Eigen::Vector3f& player_origin, Eigen::Quaternionf& player_orientation, float dt ){
	// update pos and orientation based on spring model, founded on dt

	// dt is now how much of a frame we managed to cover, i.e. fraction of 1/60th of a second
	dt *= 60.;

	// not quite hooke's law, provides tighter response at high distances
	if (_k <= 0.0) _last_pos = player_origin;
	else {
		Vector3f diff_vec = player_origin - _last_pos;
		_velocity += _k*diff_vec;
		_velocity -= _dampening*_velocity;
		if (diff_vec.norm() > _tether_dist){
			_last_pos = player_origin - (_tether_dist*(diff_vec));
		} else {
			_last_pos += _velocity*dt;
		}
	}
	// and orientation... not quite hooke's law because of difficulties
	// with the math but should be a reasonable springy approx
	if (_k_o <= 0.0) _last_orientation = player_orientation;
	else {
		AngleAxisf diff(_last_orientation.inverse()*player_orientation);
		_velocity_rps = (1.-_dampening_o)*_k_o*diff.angle() + _dampening_o*_velocity_rps;
		if (diff.angle() == 0.0) _last_orientation = player_orientation;
		else {
			float how_far_along = dt*_velocity_rps / diff.angle();
			Quaternionf delta_orientation = _last_orientation.slerp(_k_o, player_orientation);
			delta_orientation = _last_orientation.inverse()*delta_orientation;
			_velocity_o = delta_orientation.slerp(_dampening_o, _velocity_o);
			Quaternionf reduced_velo = Quaternionf(Eigen::AngleAxisf(0.0, Eigen::Vector3f::UnitX())).slerp(dt, _velocity_o);
			_last_orientation = _last_orientation * reduced_velo;
		}
	} 
	for (int i=0; i<_textboxes.size(); i++){
		// compiler didn't like std::vector<object> for whatever reason, so I'm left
		// with a bunch of dereferences here.... yuck :P
		Vector3f pos = Vector3f(_last_pos + _last_orientation*(*_offsets_xyz[i]));
		Vector3f facedir = Vector3f(_last_orientation*(*_offsets_quats[i])*((-1.)*(*_offsets_xyz[i])));
		_textboxes[i]->set_pos(pos);
		_textboxes[i]->set_facedir(facedir);
	}	
}
void Ironman_HUD::draw(  ){
	for (int i=0; i<_textboxes.size(); i++){
		Vector3f updir = Vector3f(_last_orientation*Quaternionf::FromTwoVectors(
                Vector3f(0.0, 0.0, -1.0), *_offsets_xyz[i])*Vector3f(0.0, 1.0, 0.0));
		_textboxes[i]->draw(updir);
	}	
}