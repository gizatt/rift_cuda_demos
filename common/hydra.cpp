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
                               _using_hydra( using_hydra ) {
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

        _acd0 = _acd;
    } else {
        printf("Not using sixense library / hydra.\n");
    }
    return;
}

void Hydra::normal_key_handler(unsigned char key, int x, int y){
    if (_using_hydra){
        switch (key){
            case 'l':
                // store calibration
                _acd0 = _acd;
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

Vector3f Hydra::getCurrentPos(unsigned char which_hand) {
    int i;
    if (_using_hydra){
        if (which_hand == 'l'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1L);
        } else if (which_hand == 'r'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1R);
        } else {
            printf("Hydra::getCurrentPos called with unknown which_hand arg.\n");
            return Vector3f(0.0, 0.0, 0.0);
        }
        sixenseMath::Vector3 currpos = sixenseMath::Vector3(_acd.controllers[i].pos);
        sixenseMath::Vector3 origin = sixenseMath::Vector3(_acd0.controllers[i].pos);

        sixenseMath::Quat currorr = sixenseMath::Quat(_acd.controllers[i].rot_quat);
        sixenseMath::Quat orrorr = sixenseMath::Quat(_acd0.controllers[i].rot_quat);

        // We want offset in frame of our new origin. So take difference between origins...
        currpos -= origin;
        // And rotate by origin rotation
        currpos = orrorr.inverse()*currpos;

        return Vector3f(currpos[0], currpos[1], currpos[2]);
    } else {
        return Vector3f(0.0, 0.0, 0.0);
    }
}

Vector3f Hydra::getCurrentRPY(unsigned char which_hand) {
    int i;
    if (_using_hydra){
        if (which_hand == 'l'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1L);
        } else if (which_hand == 'r'){
            i = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::IControllerManager::P1R);
        } else {
            printf("Hydra::getCurrentPos called with unknown which_hand arg.\n");
            return Vector3f(0.0, 0.0, 0.0);
        }
        //sixenseMath::Vector3 currpos = sixenseMath::Vector3(_acd.controllers[i].pos);
        //sixenseMath::Vector3 origin = sixenseMath::Vector3(_acd0.controllers[i].pos);

        sixenseMath::Quat currorr = sixenseMath::Quat(_acd.controllers[i].rot_quat);
        sixenseMath::Quat orrorr = sixenseMath::Quat(_acd0.controllers[i].rot_quat);

        // We want rotation offset in our frame, which is just rotation of one quat to the other...
        currorr = currorr * orrorr.inverse();

        sixenseMath::Vector3 rpy = currorr.getEulerAngles();
        return Vector3f(rpy[0], rpy[1], rpy[2]);
    } else {
        return Vector3f(0.0, 0.0, 0.0);
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