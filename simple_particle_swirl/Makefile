NVCC = nvcc
CFLAGS = /MTd
LFLAGS = -L../lib -L$(RIFTLDIR) -m32 -llibovr -lopengl32 -lUser32 -lGdi32 \
	-lglew32d -lcutil32d --optimize 9001 \
	-use_fast_math 
RIFTIDIR=C:\Users\gizatt\Dropbox\Programming\xen_rift\OculusSDK\LibOVR\Include
RIFTLDIR=C:\Users\gizatt\Dropbox\Programming\xen_rift\OculusSDK\LibOVR\Lib\Win32
simple_particle_swirl.exe: simple_particle_swirl.cu simple_particle_swirl.h
	vcvars32
	$(NVCC) -I$(RIFTIDIR) $(CLAFGS) $(LFLAGS) simple_particle_swirl.cu -o ../bin/$@