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

Ironman_HUD::Ironman_HUD()
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

void Ironman_HUD::draw( Eigen::Vector3f& player_origin, Eigen::Quaternionf& player_orientation ){
	for (int i=0; i<_textboxes.size(); i++){
		// compiler didn't like std::vector<object> for whatever reason, so I'm left
		// with a bunch of dereferences here.... yuck :P
		Vector3f pos = Vector3f(player_origin + player_orientation*(*_offsets_xyz[i]));
		Vector3f facedir = Vector3f(player_orientation*(*_offsets_quats[i])*((-1.)*(*_offsets_xyz[i])));
		_textboxes[i]->set_pos(pos);
		_textboxes[i]->set_facedir(facedir);
		Vector3f updir = Vector3f(player_orientation*Quaternionf::FromTwoVectors(
                Vector3f(0.0, 0.0, -1.0), *_offsets_xyz[i])*Vector3f(0.0, 1.0, 0.0));
		_textboxes[i]->draw(updir);
	}
}