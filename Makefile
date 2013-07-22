NVCC = nvcc
CL = cl

CFLAGS = /MTd
LFLAGS = -L./lib -L$(RIFTLDIR) -m32 -llibovr -lopengl32 -lUser32 -lGdi32 \
	-lglew32d -lcutil32d --optimize 9001 \
	-use_fast_math 
ODIR=./obj
RIFTIDIR=C:\Users\gizatt\Dropbox\Programming\xen_rift\OculusSDK\LibOVR\Include
RIFTLDIR=C:\Users\gizatt\Dropbox\Programming\xen_rift\OculusSDK\LibOVR\Lib\Win32

simple_particle_swirl.exe: player.obj rift.obj \
						simple_particle_swirl/simple_particle_swirl.cu \
						simple_particle_swirl/simple_particle_swirl.h
	vcvars32
	$(NVCC) -I$(RIFTIDIR) $(CLAFGS) $(LFLAGS) \
		$(ODIR)/player.obj $(ODIR/rift.obj \
		simple_particle_swirl/simple_particle_swirl.cu -o ./bin/$@

player.obj: common/player.cpp common/player.h
	vcvars32
	$(NVCC) -I$(RIFTIDIR) $(CLAFGS) $(LFLAGS) common/player.cpp -odir $(ODIR) -c $@

rift.obj: common/rift.cpp common/rift.h
	vcvars32
	$(NVCC) -I$(RIFTIDIR) $(CLAFGS) $(LFLAGS) common/rift.cpp -odir $(ODIR) -c $@