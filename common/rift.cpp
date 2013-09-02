/* #########################################################################
        Rift class -- rift initialization and such convenience functions
            (provides convenient wrappers to setup Rift in such a way
            that's reasonably robust to lack of device, etc)
        
        Much reference to Rift SDK samples for ways of robustly handling
            failure cases.

        Wraps the Rift SDK to manage its initialization (robust to lack
        of device) and support reasonably flexible stereo rendering.
        Keeping this updated with screen dimensions (mutators for
        those are a %TODO) will allow this to be called as a core render
        helper, passing its render() method a pointer to a render() function
        that does the actual drawing (after view, perspective, modelview
        are set).

   Rev history:
     Gregory Izatt  20130721  Init revision
     Gregory Izatt  201308**  Various revisions, fleshing this out to base
                                functionality and squashing bugs.
   ######################################################################### */    

#include "rift.h"
using namespace std;
using namespace xen_rift;
using namespace OVR;
using namespace OVR::Util::Render;

Rift::Rift(int inputWidth, int inputHeight, bool verbose) :
            // Stereo config helper
            _SConfig(),
            _PostProcess(PostProcess_Distortion),
            // timekeeping
            _mouselook(0),
            _mouseButtons(0),
            _c_down(false),
            _have_rift(false)
            {
    _verbose = verbose;

     // Position and look. The following apply:
     _EyePos = Vector3f();
    // Rotation around Y, CCW positive when looking at RHS (X,Z) plane.
    _EyeYaw = 0.0;      
    // Pitch. If sensor is plugged in, only read from sensor.
    _EyePitch = 0.0;       
    // Roll, only accessible from Sensor.
    _EyeRoll = 0.0;        
    // Stores previous Yaw value from to support computing delta.
    _LastSensorYaw = 0.0;  

    System::Init(Log::ConfigureDefaultLog(LogMask_All));
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
            cout << detectionMessage << endl;
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
        } else {
            detectResult = PR_CONTINUE;
        }

    } while (detectResult != PR_CONTINUE);

    /* did we get a rift? */
    if (_pHMD){
        _have_rift = true;
    }

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

    // Set up shaders (these aren't presently in use)
    _program_num = glCreateProgram();
    load_shaders("../shaders/rift_vert_shader.vert", &_vshader_num, 
                "../shaders/rift_frag_shader.frag", &_fshader_num);
    glAttachShader(_program_num, _fshader_num);
    glAttachShader(_program_num, _vshader_num);
    glLinkProgram(_program_num);

    // set up warp shaders (these are def. in use!)
    _warpShaderID = glCreateProgram();
    load_shaders("../shaders/empty.shdr", &_barrel_vert,
                "../shaders/barrel.frag", &_barrel_frag, 
                "../shaders/barrel.geom", &_barrel_geom);
    glAttachShader( _warpShaderID, _barrel_frag);
    glAttachShader( _warpShaderID, _barrel_geom);
    glAttachShader( _warpShaderID, _barrel_vert);
    glLinkProgram( _warpShaderID);

    // And set up fbos and textures for render-to-texture steps in pipeline
    // fbo that the regular scene render goes to:
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    // its texture:
    glGenTextures(1, &_render_texture);
    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D,  _render_texture);
    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
    // texture properties
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                    GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    // Set "_render_texture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _render_texture, 0);
    // And a depth buffer
    glGenRenderbuffers(1, &_render_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, _render_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _render_depth);

    // fbo that the warp render goes to:
    glGenFramebuffers(1, &_fbo_spare);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo_spare);
    glGenTextures(1, &_render_texture_spare);
    glBindTexture(GL_TEXTURE_2D,  _render_texture_spare);
    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    // texture properties
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                    GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    // Set "_render_texture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _render_texture_spare, 0);

    // Set the list of draw buffers.
    //GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0};
    //glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    // framebuffer is ok?
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        printf("Framebuffer problem.\n");
        exit(1);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glUseProgram(_program_num);

    QueryPerformanceCounter(&_lasttime);
}

int Rift::set_resolution(int width, int height)
{
    if (!_have_rift){
        _height = height;
        _width = width;
        return 0;
    } else {
        return 1;
    }
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

void Rift::normal_key_handler(unsigned char key, int x, int y){
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
        case 'c':
            _c_down = true;
            break;
    }
}
void Rift::normal_key_up_handler(unsigned char key, int x, int y){
    switch (key){
        case 'c':
            _c_down = false;
        default:
            break;
    }
}

void Rift::special_key_handler(int key, int x, int y){
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
void Rift::special_key_up_handler(int key, int x, int y){
    return;
}

void Rift::mouse(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        _mouseButtons |= 1<<button;
    } else if (state == GLUT_UP) {
        _mouseButtons = 0;
    }
    _mouseOldX = x;
    _mouseOldY = y;
    return;
}

void Rift::motion(int x, int y) {
    // mouse emulates head orientation
    float dx, dy;
    dx = (float)(x - _mouseOldX);
    dy = (float)(y - _mouseOldY);

    if (_c_down){

        if (_mouseButtons == 1) {
            // left
            _EyeYaw -= dx * 0.001f;
            _EyePitch -= dy * 0.001f;
        } else if (_mouseButtons == 2) {   
        } else if (_mouseButtons == 4) {
            // right
            _EyeRoll += dx * 0.001f;
        }

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

void Rift::stereoWarp(GLuint outFBO, GLuint inTexture)
{
    int tLoc;
    // Draw final fbo to screen
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    // Don't use a program.  That is, use the fixed funtion pipeline.
    glUseProgram(_warpShaderID);
    glActiveTexture(0);
    glBindFramebuffer( GL_FRAMEBUFFER, outFBO );
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inTexture);
    glClear(GL_COLOR_BUFFER_BIT);

    tLoc =  glGetUniformLocation(_warpShaderID,"Texture");
    glUniform1i(tLoc,0);

    glViewport(0,0,_width,_height);
    // render a single triangle, coords don't matter
    glBegin(GL_TRIANGLES);
        glVertex3f(0, 0, 0);
        glVertex3f(1, 0, 0);
        glVertex3f(0, 1, 0);
    glEnd(); 

    // But make sure we get back to normal afterwards.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glActiveTexture(0);
    return;
    
    /* set uniforms here */
    // and sets the input vars so the shader knows how much to scale
    //GLuint LensLCenterLoc = glGetUniformLocation(_program_num, "LensLeftCenter");
    //glUniform2f(LensLCenterLoc, 0.25, 0.5);    
    //GLuint LensRCenterLoc = glGetUniformLocation(_program_num, "LensRightCenter");
    //glUniform2f(LensRCenterLoc, 0.75, 0.5);
    //GLuint ScreenCenterLoc = glGetUniformLocation(_program_num, "ScreenCenter");
    //glUniform2f(ScreenCenterLoc, 0.5, 0.5);
    //GLuint ScaleLoc = glGetUniformLocation(_program_num, "Scale");
    //glUniform2f(ScaleLoc, 1.0, 1.0);    
    //GLuint ScaleInLoc = glGetUniformLocation(_program_num, "ScaleIn");
    //glUniform2f(ScaleInLoc, 1.0/1280.0, 1.0/800.0);
    //GLuint HmdWarpParamLoc = glGetUniformLocation(_warpShaderID,"DistortionOffset");
    //glUniform4f(HmdWarpParamLoc, stereo_left.pDistortion->K[0], 
    //                             stereo_left.pDistortion->K[1],
    //                             stereo_left.pDistortion->K[2],
    //                             stereo_left.pDistortion->K[3] );
    //                             stereo_left.pDistortion->K[3]);
//  tLoc =  glGetUniformLocation(_warpShaderID,"DistortionOffset");
//  glUniform1f(tLoc,0.1453f);
}

void Rift::render(Vector3f EyePos, Vector3f EyeRot, OVR::Vector3f EyeOffset, 
                  bool use_EyeOffset, void (*draw_scene)(void)){
    
    // Rotate and position View Camera, using YawPitchRoll in BodyFrame coordinates.
    Matrix4f rollPitchYaw = Matrix4f::RotationY(_EyeYaw+EyeRot.y) * 
                            Matrix4f::RotationX(_EyePitch+EyeRot.x) *
                            Matrix4f::RotationZ(_EyeRoll+EyeRot.z);
    Vector3f up      = rollPitchYaw.Transform(UpVector);
    Vector3f forward = rollPitchYaw.Transform(ForwardVector);
    // Minimal head modelling.
    float headBaseToEyeHeight     = 0.15f;  // Vertical height of eye from base of head
    float headBaseToEyeProtrusion = 0.09f;  // Distance forward of eye from base of head

    Vector3f eyeCenterInHeadFrame(0.0f, headBaseToEyeHeight, -headBaseToEyeProtrusion);
    Vector3f shiftedEyePos = EyePos + rollPitchYaw.Transform(eyeCenterInHeadFrame);
    //printf("Shifted Eye Pos: %f, %f, %f\n", shiftedEyePos.x, shiftedEyePos.y, shiftedEyePos.z);
    shiftedEyePos.y -= eyeCenterInHeadFrame.y; // Bring the head back down to original height

    Matrix4f View;

    if (use_EyeOffset){
        EyePos += EyeOffset;
        View = Matrix4f::LookAtRH(EyePos, EyePos + forward, up); 
    } else {
        View = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + forward, up); 
    }

    const StereoEyeParams& stereo_left = _SConfig.GetEyeRenderParams(StereoEye_Left);
    const StereoEyeParams& stereo_right = _SConfig.GetEyeRenderParams(StereoEye_Right);

    // distortion shaders, if active
    // Render to first framebuffer
    glActiveTexture(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (_PostProcess == PostProcess_Distortion)
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(0);

    switch(_SConfig.GetStereoMode())
    {
    case Stereo_None:
        render_one_eye(_SConfig.GetEyeRenderParams(StereoEye_Center), View, EyePos, draw_scene);
        break;

    case Stereo_LeftRight_Multipass:
        render_one_eye(stereo_left, View, EyePos, draw_scene);
        render_one_eye(stereo_right, View, EyePos, draw_scene);
        break;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Apply stereowarp, mapping it out to second framebuffer
    if (_PostProcess == PostProcess_Distortion){
        stereoWarp(_fbo_spare, _render_texture);
        // Draw final fbo to screen
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        // Don't use a program.  That is, use the fixed funtion pipeline.
        glUseProgram(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _render_texture_spare);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0,0,_width,_height);
        renderFullscreenQuad();
        // But make sure we get back to normal afterwards.
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
    }

    glutSwapBuffers();  

}

// Render the scene for one eye.
void Rift::render_one_eye(const StereoEyeParams& stereo, 
                            Matrix4f view_mat, Vector3f EyePos, void (*draw_scene)(void))
{
    float renderScale = _SConfig.GetDistortionScale();
    Viewport VP = stereo.VP;
    Matrix4f proj = stereo.Projection;
    Matrix4f viewadjust = stereo.ViewAdjust; // do I need this?

    Matrix4f real_mv = viewadjust * view_mat;

    // apply postprocessing stuff
    //pRender->BeginScene(PostProcess);

    // Apply Viewport/Projection for the eye.
    //pRender->ApplyStereoParams(stereo);    
    //pRender->Clear();
    //pRender->SetDepthMode(true, true);
    glViewport(VP.x,VP.y,VP.w,VP.h);
    //printf("Viewport: x:%d, y:%d, w:%d, h:%d\n", VP.x, VP.y, VP.w, VP.h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat tmp[16];
    //printf("Projection:\n");
    for (int i=0; i<4; i++){
        //printf("    ");
        for (int j=0; j<4; j++){
            // make sure to transpose projection matrix... opengl is silly
            tmp[j*4+i] = proj.M[i][j]; 
            //printf("| %5f |", tmp[i*4+j]);
        }
        //printf("\n");
    }
    //printf("\n");
    glLoadMatrixf(tmp);

    // set view matrix
    glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    //glTranslatef( EyePos.x, EyePos.y, EyePos.z );
    //printf("View:\n");
    for (int i=0; i<4; i++){
        //printf("    ");
        for (int j=0; j<4; j++){
            // tranpose this too...
            tmp[j*4+i] = real_mv.M[i][j];
            //printf("| %5f |", tmp[j*4+i]);
        }
        //printf("\n");
    }
    //printf("\n");
    glLoadMatrixf(tmp);

    // Call main renderer
    glPushMatrix();
    draw_scene();
    glPopMatrix();
}

