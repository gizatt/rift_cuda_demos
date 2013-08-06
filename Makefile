NVCC = nvcc
CL = cl

CFLAGS=  
LFLAGS= -L./lib -L$(RIFTLDIR) -m32 -llibovr -lopengl32 -lUser32 -lGdi32 \
    -lglew32d -lcutil32d --optimize 9001 \
    -use_fast_math
ODIR=./obj

## UPDATE TO YOUR RIFTDIR
## todo: pull riftdir from environmental variables, don't harcode here.
RIFTDIR=C:\Users\gizatt\Dropbox\Programming\xen_rift\OculusSDK\LibOVR
RIFTIDIR=$(RIFTDIR)\Include
RIFTLDIR=$(RIFTDIR)\Lib\Win32


simple_particle_swirl.exe: player.obj rift.obj xen_utils.obj \
    simple_particle_swirl/simple_particle_swirl.cu \
    simple_particle_swirl/simple_particle_swirl.h
	vcvars32
	$(NVCC) $(CFLAGS) -I$(RIFTIDIR) $(LFLAGS) \
        $(ODIR)/player.obj $(ODIR)/rift.obj $(ODIR)/xen_utils.obj \
        simple_particle_swirl/simple_particle_swirl.cu -o ./bin/$@

player.obj: common/player.cpp common/player.h
	vcvars32
	$(NVCC) $(CFLAGS) -I$(RIFTIDIR) $(LFLAGS) common/player.cpp -odir $(ODIR) -c $@

rift.obj: common/rift.cpp common/rift.h
	vcvars32
	$(NVCC) $(CFLAGS) -I$(RIFTIDIR) $(LFLAGS) $(ODIR)/xen_utils.obj \
		common/rift.cpp -odir $(ODIR) -c $@

xen_utils.obj: common/xen_utils.cpp common/xen_utils.h
	vcvars32
	$(NVCC) $(CFLAGS) -I$(RIFTIDIR) $(LFLAGS) common/xen_utils.cpp -odir $(ODIR) -c $@