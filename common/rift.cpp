/* #########################################################################
        Rift class -- rift initialization and such convenience functions
            (provides convenient wrappers to setup Rift in such a way
            that's reasonably robust to lack of device, etc)
        
        Much reference to Rift SDK samples for ways of robustly handling
            failure cases.

   Rev history:
     Gregory Izatt  20130721  Init revision
   ######################################################################### */    

#include "rift.h"
using namespace std;
using namespace xen_rift;
using namespace OVR;
using namespace OVR::Util::Render;

Rift::Rift(int inputHeight, int inputWidth, bool verbose) :
            // Stereo config helper
            _SConfig(),
            _PostProcess(PostProcess_Distortion),
            // timekeeping
            _mouselook(0),
            _mouseButtons(0)
            {

    _verbose = verbose;

    _pManager = *DeviceManager::Create();
    // We'll handle its messages.
    _pManager->SetMessageHandler(this);  

    detect_prompt_t detectResult = PR_CONTINUE;
    const char* detectionMessage;

    do 
    {
        // Release Sensor/HMD in case this is a retry.
        _pSensor.Clear();
        _pHMD.Clear();

        _pHMD  = *_pManager->EnumerateDevices<HMDDevice>().CreateDevice();
        if (_pHMD)
        {
                _pSensor = *_pHMD->GetSensor();

            // This will initialize HMDInfo with information about configured IPD,
            // screen size and other variables needed for correct projection.
            // We pass HMD DisplayDeviceName into the renderer to select the
            // correct monitor in full-screen mode.
            if (_pHMD->GetDeviceInfo(&_HMDInfo))
            {            
                _SConfig.SetHMDInfo(_HMDInfo);
            }
        } else {            
            // If we didn't detect an HMD, try to create the sensor directly.
            // This is useful for debugging sensor interaction; it is not needed in
            // a shipping app.
            _pSensor = *_pManager->EnumerateDevices<SensorDevice>().CreateDevice();
        }

        // If there was a problem detecting the Rift, display appropriate message.
        detectResult  = PR_ABORT;        

        if (!_pHMD && !_pSensor)
            detectionMessage = "Oculus Rift not detected.";
        else if (!_pHMD)
            detectionMessage = "Oculus Sensor detected; HMD Display not detected.";
        else if (!_pSensor)
            detectionMessage = "Oculus HMD Display detected; Sensor not detected.";
        else if (_HMDInfo.DisplayDeviceName[0] == '\0')
            detectionMessage = "Oculus Sensor detected; HMD display EDID not detected.";
        else
            detectionMessage = NULL;

        if (detectionMessage)
        {
            cout << "detectionMessage" << endl;
            cout << "Enter 'r' to retry, 'c' to continue, and anything else to abort:" << endl;
            string input;
            getline(cin, input);
            if (input=="r"){
                detectResult = PR_RETRY;
            } else if (input=="c"){
                detectResult = PR_CONTINUE;
            }

            if (detectResult== PR_ABORT)
                exit(1);
        }

    } while (detectResult != PR_CONTINUE);

    // If we have info on the HMD resolution, grab that.
    if (_HMDInfo.HResolution > 0)
    {
        _width  = _HMDInfo.HResolution;
        _height = _HMDInfo.VResolution;
    } else {
        _width = inputWidth;
        _height = inputHeight;
    }

    if (_pSensor)
    {
        // We need to attach sensor to SensorFusion object for it to receive 
        // body frame messages and update orientation. SFusion.GetOrientation() 
        // is used in OnIdle() to orient the view.
        _SFusion.AttachToSensor(_pSensor);
        _SFusion.SetDelegateMessageHandler(this);
        _SFusion.SetPredictionEnabled(true);
    }

    // *** Configure Stereo settings.

    _SConfig.SetFullViewport(Viewport(0,0, _width, _height));
    _SConfig.SetStereoMode(Stereo_LeftRight_Multipass);

    // Configure proper Distortion Fit.
    // For 7" screen, fit to touch left side of the view, leaving a bit of invisible
    // screen on the top (saves on rendering cost).
    // For smaller screens (5.5"), fit to the top.
    if (_HMDInfo.HScreenSize > 0.0f)
    {
        if (_HMDInfo.HScreenSize > 0.140f) // 7"
            _SConfig.SetDistortionFitPointVP(-1.0f, 0.0f);
        else
            _SConfig.SetDistortionFitPointVP(0.0f, 1.0f);
    }
    _SConfig.Set2DAreaFov(DegreeToRad(85.0f));

    QueryPerformanceCounter(&_lasttime);
}


void Rift::OnMessage(const Message& msg)
{
    if (_verbose){
        if (msg.Type == Message_DeviceAdded && msg.pDevice == _pManager)
        {
            printf("DeviceManager reported device added.\n");
        }
        else if (msg.Type == Message_DeviceRemoved && msg.pDevice == _pManager)
        {
            printf("DeviceManager reported device removed.\n");
        }
        else if (msg.Type == Message_DeviceAdded && msg.pDevice == _pSensor)
        {
            printf("Sensor reported device added.\n");
        }
        else if (msg.Type == Message_DeviceRemoved && msg.pDevice == _pSensor)
        {
            printf("Sensor reported device removed.\n");
        }
    }
}

void normal_key_handler(unsigned char key, int x, int y){
    switch (key){
        case 'R':
            _SFusion.Reset();
            break;
        case '+':
        case '=':
            _SConfig.SetIPD(_SConfig.GetIPD() + 0.0005f);
            break;
        case '-':
        case '_':
            _SConfig.SetIPD(_SConfig.GetIPD() - 0.0005f);  
            break;     
    }
}
void normal_key_up_handler(unsigned char key, int x, int y){
    switch (key){
        default:
            break;
    }
}

void special_key_handler(int key, int x, int y){
    switch (key) {
        case GLUT_KEY_F1:
            _SConfig.SetStereoMode(Stereo_None);
            _PostProcess = PostProcess_None;
            break;
        case GLUT_KEY_F2:
            _SConfig.SetStereoMode(Stereo_LeftRight_Multipass);
            _PostProcess = PostProcess_None;
            break;
        case GLUT_KEY_F3:
            _SConfig.SetStereoMode(Stereo_LeftRight_Multipass);
            _PostProcess = PostProcess_Distortion;
            break;
    }
}
void special_key_up_handler(int key, int x, int y){
    return;
}

void mouse(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        _mouseButtons |= 1<<button;
    } else if (state == GLUT_UP) {
        _mouseButtons = 0;
    }
    _mouseOldX = x;
    _mouseOldY = y;
    return;
}

void motion(int x, int y) {
    // mouse emulates head orientation
    float dx, dy;
    dx = (float)(x - _mouseOldX);
    dy = (float)(y - _mouseOldY);

    if (_mouseButtons == 1) {
        // left
        _EyeYaw += dx * 0.001f;
        _EyePitch += dy * 0.001f;
    } else if (_mouseButtons == 2) {   
    } else if (_mouseButtons == 4) {
        // right
        _EyeRoll += dx * 0.001f;
    }

    _mouseOldX = x;
    _mouseOldY = y;  
}


void Rift::onIdle() {
    QueryPerformanceCounter(&_currtime);

    float dt = float((unsigned long)(_currtime.QuadPart) - (unsigned long)(_lasttime.QuadPart));
    _lasttime = _currtime;

    // Handle Sensor motion.
    // We extract Yaw, Pitch, Roll instead of directly using the orientation
    // to allow "additional" yaw manipulation with mouse/controller.
    if (_pSensor)
    {        
        Quatf    hmdOrient = _SFusion.GetOrientation();
        float    yaw = 0.0f;

        hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &_EyePitch, &_EyeRoll);

        _EyeYaw += (yaw - _LastSensorYaw);
        _LastSensorYaw = yaw;    
    }    


}

void Rift::render(Vector3f EyePos, Vector3f EyeRot, void (*draw_scene)(void)){
    // Rotate and position View Camera, using YawPitchRoll in BodyFrame coordinates.
    Matrix4f rollPitchYaw = Matrix4f::RotationY(_EyeYaw+EyeRot.x) * 
                            Matrix4f::RotationX(_EyePitch+EyeRot.y) *
                            Matrix4f::RotationZ(_EyeRoll+EyeRot.z);
    Vector3f up      = rollPitchYaw.Transform(UpVector);
    Vector3f forward = rollPitchYaw.Transform(ForwardVector);
    // Minimal head modelling.
    float headBaseToEyeHeight     = 0.15f;  // Vertical height of eye from base of head
    float headBaseToEyeProtrusion = 0.09f;  // Distance forward of eye from base of head

    Vector3f eyeCenterInHeadFrame(0.0f, headBaseToEyeHeight, -headBaseToEyeProtrusion);
    Vector3f shiftedEyePos = EyePos + rollPitchYaw.Transform(eyeCenterInHeadFrame);
    shiftedEyePos.y -= eyeCenterInHeadFrame.y; // Bring the head back down to original height

    Matrix4f View = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + forward, up); 

    // This is what transformation would be without head modeling.    
    // View = Matrix4f::LookAtRH(EyePos, EyePos + forward, up);    

    switch(SConfig.GetStereoMode())
    {
    case Stereo_None:
        render_one_eye(_SConfig.GetEyeRenderParams(StereoEye_Center), View, draw_scene);
        break;

    case Stereo_LeftRight_Multipass:
        render_one_eye(_SConfig.GetEyeRenderParams(StereoEye_Left), View, draw_scene);
        render_one_eye(_SConfig.GetEyeRenderParams(StereoEye_Right), View, draw_scene);
        break;
    }

}

// Render the scene for one eye.
void Rift::render_one_eye(const StereoEyeParams& stereo, 
                            Matrix4f view_mat, void (*draw_scene)(void))
{
    float renderScale = _SConfig.GetDistortionScale();
    Viewport VP = stereo.VP;
    Matrix4f proj = stereo.Projection;
    Matrix4f viewadjust = stereo.ViewAdjust; // do I need this?

    // apply postprocessing stuff
    //pRender->BeginScene(PostProcess);

    // Apply Viewport/Projection for the eye.
    //pRender->ApplyStereoParams(stereo);    
    //pRender->Clear();
    //pRender->SetDepthMode(true, true);
    glViewport(VP.x,VP.y,VP.w,VP.h);
    glMatrixMode(GL_PROJECTION);
    GLfloat tmp[16];
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            tmp[i*4+j] = proj.M[i][j]; 
    glLoadMatrixf(tmp);

    // set view matrix
    glMatrixMode(GL_MODELVIEW);
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            tmp[i*4+j] = view_mat.M[i][j]; 
    glLoadMatrixf(tmp);

    // Call main renderer
    draw_scene();
    //Scene.Render(pRender, stereo.ViewAdjust * View);

    // finish up
    //pRender->FinishScene();
    glutSwapBuffers();  
}

