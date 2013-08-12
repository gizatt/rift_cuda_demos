NVCC = nvcc
CL = cl

CFLAGS=  
LFLAGS= -L./lib -L$(RIFTLDIR) -Xlinker=/NODEFAULTLIB:MSVCRT -Xlinker=/NODEFAULTLIB:LIBCMT \
	-Xlinker=/NODEFAULTLIB:MSVCRTD -Xlinker=/NODEFAULTLIB:LIBCMTD \
	-m32 -lSOIL -lsixensed -lsixense_utilsd -lmsvcrt -llibovr -lopengl32 -lUser32 -lGdi32 \
    -lglew32d -lcutil32d --optimize 9001 \
    -use_fast_math
ODIR=./obj

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

simple_particle_swirl.exe: player.obj rift.obj hydra.obj xen_utils.obj \
    simple_particle_swirl/simple_particle_swirl.cu \
    simple_particle_swirl/simple_particle_swirl.h
	vcvars32
	$(NVCC) $(CFLAGS) $(LFLAGS) -I$(RIFTIDIR),$(HYDRAIDIR) \
        $(ODIR)/player.obj $(ODIR)/rift.obj $(ODIR)/hydra.obj $(ODIR)/xen_utils.obj \
        simple_particle_swirl/simple_particle_swirl.cu -o ./bin/$@

player.obj: common/player.cpp common/player.h
	vcvars32
	$(NVCC) $(CFLAGS) $(LFLAGS) common/player.cpp -odir $(ODIR) -c $@

rift.obj: xen_utils.obj common/rift.cpp common/rift.h
	vcvars32
	$(NVCC) $(CFLAGS) -I$(RIFTIDIR) $(LFLAGS) $(ODIR)/xen_utils.obj \
		common/rift.cpp -odir $(ODIR) -c $@

hydra.obj: xen_utils.obj common/hydra.cpp common/hydra.h
	vcvars32
	$(NVCC) $(CFLAGS) -I$(HYDRAIDIR) $(LFLAGS) $(ODIR)/xen_utils.obj \
		common/hydra.cpp -odir $(ODIR) -c $@

xen_utils.obj: common/xen_utils.cpp common/xen_utils.h
	vcvars32
	$(NVCC) $(CFLAGS) $(LFLAGS) common/xen_utils.cpp -odir $(ODIR) -c $@