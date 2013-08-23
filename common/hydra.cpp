/* #########################################################################
        Hydra class -- hydra initialization and such convenience functions
            (provides convenient wrappers to setup Hydra in such a way
            that's reasonably robust to lack of device, etc)
        

        Wraps the Hydra SDK to manage its initialization (robust to lack
        of device).

        Reference to the Hydra SDK examples.

        %TODO: controller manager setup callbacks are just functions,
            not members, due to some difficulty with function ptrs 
            to member funcs. This limits us to once instance of this
            class running at a time! This should be fixed...

   Rev history:
     Gregory Izatt  20130808  Init revision
     Gregory Izatt  20130808  Continuing revision now that I have a hydra

   ######################################################################### */    

#include "hydra.h"

using namespace std;
using namespace xen_rift;
using namespace Eigen;

Hydra::Hydra( bool using_hydra, bool verbose ) : _verbose(verbose),
                               _using_hydra( using_hydra ),
                               _calibration_state( NOT_CALIBRATING ),
                               _quatl0( Quaternionf() ),
                               _quatr0( Quaternionf() ),
                               _posl0( Vector3f() ),
                               _posr0( Vector3f() ),
                               _touch_point( Vector3f( 0.0, 0.3, -0.3) ) {
    if (_using_hydra){
        // sixsense init
        int retval = sixenseInit();
        if (retval != SIXENSE_SUCCESS){
            printf("Couldn't initialize sixense library.\n");
            _using_hydra = false;
            return;
        }

        if (verbose){
            printf("Initialializing controller manager for hydra...\n");
        }
        // Init the controller manager. This makes sure the controllers are present, assigned to left and right hands, and that
        // the hemisphere calibration is complete.
        sixenseUtils::getTheControllerManager()->setGameType( sixenseUtils::ControllerManager::ONE_PLAYER_TWO_CONTROLLER );
        sixenseUtils::getTheControllerManager()->registerSetupCallback( controller_manager_setup_callback );
        printf("Waiting for device setup...\n");
        // Wait for the menu calibrations to go away... lazy man's menu, this
        //  should be made graphical eventually
        while(!sixenseUtils::getTheControllerManager()->isMenuVisible()){
            // update the controller manager with the latest controller data here
            sixenseSetActiveBase(0);
            sixenseGetAllNewestData( &_acd );
            sixenseUtils::getTheControllerManager()->update( &_acd );
        }
        while(sixenseUtils::getTheControllerManager()->isMenuVisible()){
            // update the controller manager with the latest controller data here
            sixenseSetActiveBase(0);
            sixenseGetAllNewestData( &_acd );
            sixenseUtils::getTheControllerManager()->update( &_acd );
        }

        // spawn textbox we'll use to present calibration instructions
        _instruction_textbox = new Textbox_3D(string(""), Vector3f(), Vector3f(), 
                1.5 , 0.5, 0.05, 5);

    } else {
        printf("Not using sixense library / hydra.\n");
    }
    return;
}

void Hydra::normal_key_handler(unsigned char key, int x, int y){
    int i;
    if (_using_hydra){
        switch (key){
            case 'k':
                // store calibation zero point
                i = sixenseUtils::getTheControllerManager()->getIndex(
                                sixenseUtils::IControllerManager::P1L);
                _posl0 = Vector3f(_acd.controllers[i].pos);
                _quatl0 = Quaternionf(_acd.controllers[i].rot_quat);
                i = sixenseUtils::getTheControllerManager()->getIndex(
                                sixenseUtils::IControllerManager::P1R);
                _posr0 = Vector3f(_acd.controllers[i].pos);
                _quatr0 = Quaternionf(_acd.controllers[i].rot_quat);
                break;
            case 'l':
                // advance through calibration procedure
                switch (_calibration_state){
                    case NOT_CALIBRATING:
                        _calibration_state = STARTING_CALIBRATING;
                        _instruction_textbox->set_text(string("Touch the square and hit l."));
                        break;
                    case STARTING_CALIBRATING:
                        i = sixenseUtils::getTheControllerManager()->getIndex(
                                        sixenseUtils::IControllerManager::P1L);
                        _posl0 = Vector3f(_acd.controllers[i].pos);
                        i = sixenseUtils::getTheControllerManager()->getIndex(
                                        sixenseUtils::IControllerManager::P1R);
                        _posr0 = Vector3f(_acd.controllers[i].pos);
                        _instruction_textbox->set_text(string("Hold straight out and hit l."));
                        _calibration_state = MIDDLE_CALIBRATING;
                        break;
                    case MIDDLE_CALIBRATING:
                        i = sixenseUtils::getTheControllerManager()->getIndex(
                                        sixenseUtils::IControllerManager::P1L);
                        _quatl0 = Quaternionf(_acd.controllers[i].rot_quat);
                        i = sixenseUtils::getTheControllerManager()->getIndex(
                                        sixenseUtils::IControllerManager::P1R);
                        _quatr0 = Quaternionf(_acd.controllers[i].rot_quat);
                        _calibration_state = NOT_CALIBRATING;
                        break;
                }
                break;
            default:
                break;
        }
    }
}
void Hydra::normal_key_up_handler(unsigned char key, int x, int y){
    if (_using_hydra){
        switch (key){
            default:
                break;
            }
    }
}

void Hydra::special_key_handler(int key, int x, int y){
    if (_using_hydra){
        switch (key) {
            default:
                break;
            }
    }
}
void Hydra::special_key_up_handler(int key, int x, int y){
    return;
}

void Hydra::mouse(int button, int state, int x, int y) {
    return;
}

void Hydra::motion(int x, int y) {
    return;
}

void Hydra::onIdle() {
    if (_using_hydra){
        sixenseSetActiveBase(0);
        sixenseGetAllNewestData( &_acd );
        sixenseUtils::getTheControllerManager()->update( &_acd );

        Vector3f retl = getCurrentPos('l');
        Vector3f retr = getCurrentPos('r');
    }
}

void Hydra::draw( Vector3f& player_origin, Quaternionf& player_orientation ){
    Vector3f tmp_vec;
    // draw calibration stuff if active
    switch(_calibration_state){
        case STARTING_CALIBRATING:
            // Touch point to get offset
            // draw textbox with such instructions
            tmp_vec = player_origin + player_orientation * Vector3f(0.0, -0.3, -1.0);
            _instruction_textbox->set_pos(tmp_vec);
            tmp_vec = player_origin - tmp_vec;
            tmp_vec[1] = 0.0;
            _instruction_textbox->set_facedir(tmp_vec);
            tmp_vec = player_orientation * Vector3f(0.0, 1.0, 0.0);
            _instruction_textbox->draw(tmp_vec);
            // and draw that little box
            tmp_vec = player_origin + player_orientation * _touch_point;;
            glBegin(GL_QUADS);
            glVertex3f(tmp_vec.x()-0.01, tmp_vec.y()-0.01, tmp_vec.z()-0.01);
            glVertex3f(tmp_vec.x()-0.01, tmp_vec.y()+0.01, tmp_vec.z()-0.01);
            glVertex3f(tmp_vec.x()+0.01, tmp_vec.y()+0.01, tmp_vec.z()+0.01);
            glVertex3f(tmp_vec.x()+0.01, tmp_vec.y()-0.01, tmp_vec.z()+0.01);
            glEnd();
            glBegin(GL_QUADS);
            glVertex3f(tmp_vec.x()+0.01, tmp_vec.y()-0.01, tmp_vec.z()-0.01);
            glVertex3f(tmp_vec.x()+0.01, tmp_vec.y()+0.01, tmp_vec.z()-0.01);
            glVertex3f(tmp_vec.x()-0.01, tmp_vec.y()+0.01, tmp_vec.z()+0.01);
            glVertex3f(tmp_vec.x()-0.01, tmp_vec.y()-0.01, tmp_vec.z()+0.01);
            glEnd();
            break;
        case MIDDLE_CALIBRATING:
            // hold out straight to get angle
            // Touch point to get offset
            // draw textbox with such instructions
            tmp_vec = player_origin + player_orientation * Vector3f(0.0, -0.3, -1.0);
            _instruction_textbox->set_pos(tmp_vec);
            tmp_vec = player_origin - tmp_vec;
            tmp_vec[1] = 0.0;
            _instruction_textbox->set_facedir(tmp_vec);
            tmp_vec = player_orientation * Vector3f(0.0, 1.0, 0.0);
            _instruction_textbox->draw(tmp_vec);
            break;
            break;
        default:
            break;
    }
}

void Hydra::draw_cursor( unsigned char which_hand, 
    Vector3f& player_origin, Quaternionf& player_orientation ) {
    if (_using_hydra){
        if (which_hand == 'l' || which_hand == 'r'){
            Vector3f tmp = getCurrentPos(which_hand)/1000.0;
            Vector3f pos = player_orientation*(getCurrentPos(which_hand)/1000.0) + player_origin;
            Quaternionf rot = player_orientation*getCurrentQuat(which_hand);

            //printf("Pos: %f, %f, %f, rot: %f, %f, %f, %f\n", 
            //    pos.x(), pos.y(), pos.z(), rot.w(), rot.x(), rot.y(), rot.z());

            glPushMatrix();
            //glTranslatef(pos.x(), pos.y(), pos.z());
            //AngleAxisf roti = AngleAxisf(rot);
            //glRotatef(roti.angle()*180./M_PI, roti.axis().x(), roti.axis().y(), roti.axis().z() );

            float rot_mat[4][4];
            Matrix3f roti = rot.normalized().toRotationMatrix();
            for( int i=0; i<3; i++ ) 
                for( int j=0; j<3; j++ ) 
                    rot_mat[i][j] = roti(j, i);

            rot_mat[0][3] = 0.0f;
            rot_mat[1][3] = 0.0f;
            rot_mat[2][3] = 0.0f;
            rot_mat[3][0] = pos[0];
            rot_mat[3][1] = pos[1];
            rot_mat[3][2] = pos[2];
            rot_mat[3][3] = 1.0f;
            glMultMatrixf( (GLfloat*)rot_mat );
            // rotate to orient "bottom" in -y instead of +z
            glRotatef(-90.0, 1.0, 0.0, 0.0);

            // bottom surface at
            glBegin(GL_QUADS);
            glVertex3f(-0.02, -0.08, -0.02);
            glVertex3f(-0.02, -0.08, 0.02);
            glVertex3f(0.02, -0.08, 0.02);
            glVertex3f(0.02, -0.08, -0.02);
            glEnd();

            // -x surface
            glBegin(GL_TRIANGLES);
            glVertex3f(0.0, 0.05, 0.0);
            glVertex3f(-0.02, -0.08, -0.02);
            glVertex3f(-0.02, -0.08, 0.02);
            glEnd();
            // +x surface
            glBegin(GL_TRIANGLES);
            glVertex3f(0.0, 0.05, 0.0);
            glVertex3f(0.02, -0.08, -0.02);
            glVertex3f(0.02, -0.08, 0.02);
            glEnd();

            // -z surface
            glBegin(GL_TRIANGLES);
            glVertex3f(0.0, 0.05, 0.0);
            glVertex3f(-0.02, -0.08, -0.02);
            glVertex3f(0.02, -0.08, -0.02);
            glEnd();
            // +z surface
            glBegin(GL_TRIANGLES);
            glVertex3f(0.0, 0.05, 0.0);
            glVertex3f(0.02, -0.08, 0.02);
            glVertex3f(-0.02, -0.08, 0.02);
            glEnd();

            glPopMatrix();

        } else {
            printf("Invalid hand argument.\n");
        }
    }
}

Vector3f Hydra::getCurrentPos(unsigned char which_hand) {
    int i;
    if (_using_hydra){
        Vector3f origin;
        Quaternionf orrorr;
        if (which_hand == 'l'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1L);
            origin = _posl0;
            orrorr = _quatl0;
        } else if (which_hand == 'r'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1R);
            origin = _posr0;
            orrorr = _quatr0;
        } else {
            printf("Hydra::getCurrentPos called with unknown which_hand arg.\n");
            return Vector3f(0.0, 0.0, 0.0);
        }
        Vector3f currpos = Vector3f(_acd.controllers[i].pos);

        Quaternionf currorr = Quaternionf(_acd.controllers[i].rot_quat);

        // We want offset in frame of our new origin. So take difference between origins...
        currpos -= origin;
        // And rotate by origin rotation
        currpos = orrorr.inverse()*currpos;

        // add in the touch point
        currpos += _touch_point*1000.0;

        return currpos;
    } else {
        return Vector3f(0.0, 0.0, 0.0);
    }
}

// I REALLY REALLY ADVISE AGAINST USING THIS... RPY LOSES SOME ORIENTATION INFO
// THAT QUATS MAINTAIN.
Vector3f Hydra::getCurrentRPY(unsigned char which_hand) {
    int i;
    if (_using_hydra){
        Quaternionf orrorr;
        if (which_hand == 'l'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1L);
            orrorr = _quatl0;
        } else if (which_hand == 'r'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1R);
            orrorr = _quatr0;
        } else {
            printf("Hydra::getCurrentPos called with unknown which_hand arg.\n");
            return Vector3f(0.0, 0.0, 0.0);
        }

        Quaternionf currorr = Quaternionf(_acd.controllers[i].rot_quat);

        // We want rotation offset in our frame, which is just rotation of one quat to the other...
        currorr = currorr * orrorr.inverse();

        Vector3f rpy = getEulerAnglesFromQuat(currorr);
        // make agree with opengl
        return Vector3f(rpy[2], rpy[1], -rpy[0]);
    } else {
        return Vector3f(0.0, 0.0, 0.0);
    }
}

Quaternionf Hydra::getCurrentQuat(unsigned char which_hand) {
    int i;
    if (_using_hydra){
        Quaternionf orrorr;
        if (which_hand == 'l'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1L);
            orrorr = _quatl0;
        } else if (which_hand == 'r'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1R);
            orrorr = _quatr0;
        } else {
            printf("Hydra::getCurrentPos called with unknown which_hand arg.\n");
            return Quaternionf(Eigen::AngleAxisf(0.0, Eigen::Vector3f::UnitX()));
        }

        Quaternionf currorr = Quaternionf(_acd.controllers[i].rot_quat);

        // We want rotation offset in our frame, which is just rotation of one quat to the other...
        currorr = currorr * orrorr.inverse();

        return Quaternionf(currorr.w(), currorr.z(), currorr.y(), -currorr.x());
    } else {
        return Quaternionf(Eigen::AngleAxisf(0.0, Eigen::Vector3f::UnitX()));
    }
}

// This is the callback that gets registered with the sixenseUtils::controller_manager. 
// It will get called each time the user completes one of the setup steps so that the game 
// can update the instructions to the user. If the engine supports texture mapping, the 
// controller_manager can prove a pathname to a image file that contains the instructions 
// in graphic form.
// The controller_manager serves the following functions:
//  1) Makes sure the appropriate number of controllers are connected to the system. 
//     The number of required controllers is designaged by the
//     game type (ie two player two controller game requires 4 controllers, 
//     one player one controller game requires one)
//  2) Makes the player designate which controllers are held in which hand.
//  3) Enables hemisphere tracking by calling the Sixense API call 
//     sixenseAutoEnableHemisphereTracking. After this is completed full 360 degree
//     tracking is possible.
void xen_rift::controller_manager_setup_callback( sixenseUtils::ControllerManager::setup_step step ) {

    if( sixenseUtils::getTheControllerManager()->isMenuVisible()) {

        // Draw the instruction.
        const char * instr = sixenseUtils::getTheControllerManager()->getStepString();
        printf("Hydra controller manager: %s\n", instr);

    }
}