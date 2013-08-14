NVCC = nvcc
CL = cl

## UPDATE TO YOUR RIFTDIR
## todo: pull riftdir from environmental variables, don't harcode here.
RIFTDIR=C:\Users\gizatt\Dropbox\Programming\xen_rift\OculusSDK\LibOVR
RIFTIDIR="$(RIFTDIR)\Include"
RIFTLDIR="$(RIFTDIR)\Lib\Win32"

## UPDATE TO YOUR SIXENSE SDK DIR
## todo: pull from env vars...
HYDRADIR=C:\Program Files (x86)\Steam\steamapps\common\Sixense SDK\SixenseSDK
HYDRAIDIR="$(HYDRADIR)\include"
HYDRALDIR="$(HYDRADIR)\lib\win32\debug_dll"

## UPDATE TO YOUR CUDA DIR
CUDADIR=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v5.0
CUDAIDIR="$(CUDADIR)\include"
CUDALDIR="$(CUDADIR)\lib\Win32"

ODIR=./obj
IDIR=./include
BDIR=./bin

CFLAGS=/I$(RIFTIDIR) /I$(HYDRAIDIR) /I$(IDIR)
LFLAGS= /MD /link /LIBPATH:./lib /LIBPATH:$(RIFTLDIR) /NODEFAULTLIB:LIBCMT\
	SOIL.lib sixensed.lib sixense_utilsd.lib libovr.lib libovrd.lib opengl32.lib User32.lib Gdi32.lib \
    glew32d.lib cutil32d.lib 

NVCC_CFLAGS=
NVCC_LFLAGS= -L./lib -Xlinker=/NODEFAULTLIB:MSVCRT -Xlinker=/NODEFAULTLIB:LIBCMT \
	-Xlinker=/NODEFAULTLIB:MSVCRTD -Xlinker=/NODEFAULTLIB:LIBCMTD \
	-m32 -lmsvcrt -lopengl32 -lUser32 -lGdi32 \
    -lglew32d -lcutil32d --optimize 9001 \
    -use_fast_math


simple_particle_swirl.exe: player.obj rift.obj hydra.obj xen_utils.obj \
	simple_particle_swirl_cu.obj \
    simple_particle_swirl/simple_particle_swirl.cpp \
    simple_particle_swirl/simple_particle_swirl.h
	vcvars32
	$(CL) simple_particle_swirl/simple_particle_swirl.cpp $(CFLAGS) /Fe$(BDIR)/$@  \
		$(LFLAGS) /LIBPATH:$(CUDALDIR) cudart.lib $(ODIR)/player.obj $(ODIR)/rift.obj \
		$(ODIR)/hydra.obj $(ODIR)/textbox_3d.obj \
		$(ODIR)/xen_utils.obj $(ODIR)/simple_particle_swirl_cu.obj 

simple_particle_swirl_cu.obj: simple_particle_swirl/simple_particle_swirl.cu \
		simple_particle_swirl/simple_particle_swirl_cu.h
	vcvars32
	$(NVCC) $(NVCC_CFLAGS) $(NVCC_LFLAGS) -I$(RIFTIDIR),$(HYDRAIDIR) \
        -c simple_particle_swirl/simple_particle_swirl.cu -o ./obj/$@

player.obj: textbox_3d.obj common/player.cpp common/player.h
	vcvars32
	$(CL) /c common/player.cpp $(CFLAGS) /Fo$(ODIR)/$@ $(LFLAGS) $(ODIR)/xen_utils.obj

rift.obj: xen_utils.obj common/rift.cpp common/rift.h
	vcvars32
	$(CL) /c common/rift.cpp $(CFLAGS) /Fo$(ODIR)/$@ $(LFLAGS) $(ODIR)/xen_utils.obj

hydra.obj: xen_utils.obj common/hydra.cpp common/hydra.h
	vcvars32
	$(CL) /c common/hydra.cpp $(CFLAGS) /Fo$(ODIR)/$@ $(LFLAGS) $(ODIR)/xen_utils.obj 

textbox_3d.obj: xen_utils.obj common/textbox_3d.cpp common/textbox_3d.h
	vcvars32
	$(CL) /c common/textbox_3d.cpp $(CFLAGS) /Fo$(ODIR)/$@ $(LFLAGS)

xen_utils.obj: common/xen_utils.cpp common/xen_utils.h
	vcvars32
	$(CL) /c common/xen_utils.cpp $(CFLAGS) /Fo$(ODIR)/$@ $(LFLAGS)