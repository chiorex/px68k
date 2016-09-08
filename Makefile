include version.txt

AS   = as
CC	 = gcc
CXX	 = c++
CXXLINK	 = $(CXX)
RM	 = rm -f
TAGS	 = etags

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell mkdir -p $(DEPDIR)/x68k >/dev/null)
$(shell mkdir -p $(DEPDIR)/fmgen >/dev/null)
$(shell mkdir -p $(DEPDIR)/win32api >/dev/null)
$(shell mkdir -p $(DEPDIR)/x11 >/dev/null)
$(shell mkdir -p $(DEPDIR)/m68000 >/dev/null)

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

# for debug
debug: CDEBUGFLAGS = -g -O0 -fno-strict-aliasing -W 
debug: CFLAGS += -DWIN68DEBUG
debug: ASFLAGS += -g 

ifdef CYCLONE
all:: CDEBUGFLAGS = -Ofast -g 
else
all:: CDEBUGFLAGS = -O2 -g 
endif

ifdef PROFILE
CFLAGS += -g
CXXLDOPTIONS += -lprofiler -g
endif

ifdef MULTI_THREAD
CFLAGS += -DMULTI_THREAD
endif

ifdef VSYNC
CFLAGS += -DVSYNC
LDLIBS += -lbcm_host -L/opt/vc/lib
EXTRA_INCLUDES += -I/opt/vc/include/
endif

ifdef SDL2
CFLAGS += -DUSE_OGLES11
endif

CFLAGS+=-DPX68K_VERSION=$(PX68K_VERSION)

ifdef SDL2
# SDL 2.0
SDL_CONFIG?= sdl2-config
else
# SDL 1.2
SDL_CONFIG?= sdl-config
endif

SDL_INCLUDE=	`$(SDL_CONFIG) --cflags`
ifdef SDL2
SDL_LIB=	`$(SDL_CONFIG) --libs` -lSDL2_gfx -L/opt/vc/lib -lGLESv2
else
SDL_LIB=	`$(SDL_CONFIG) --libs` -lSDL_gfx
endif

ifeq ($(shell uname -m),armv7l)
	MOPT= -mcpu=cortex-a53  -mfpu=neon-fp-armv8 -funsafe-math-optimizations
else
	MOPT= -m32
endif

ifdef CYCLONE
CFLAGS += -DCYCLONE
endif

LDLIBS += -lm -lpthread 

EXTRA_INCLUDES += -I./x11 -I./x68k -I./fmgen -I./win32api $(SDL_INCLUDE) 

CXXDEBUGFLAGS= $(CDEBUGFLAGS)

CFLAGS+= $(MOPT) $(CDEBUGFLAGS) $(EXTRA_INCLUDES)
CXXFLAGS= $(CFLAGS)
CXXLDOPTIONS+= $(CXXDEBUGFLAGS)

CYCLONEOBJ = m68000/cyclone.o
C68KOBJ =  m68000/c68k.o

ifdef CYCLONE
CPUOBJS= x68k/d68k.o m68000/m68000.o $(CYCLONEOBJ)
else
CPUOBJS= x68k/d68k.o m68000/m68000.o $(C68KOBJ)
endif

X68KOBJS= $(addprefix x68k/,adpcm.o bg.o crtc.o dmac.o fdc.o fdd.o disk_d88.o disk_dim.o disk_xdf.o gvram.o ioc.o irqh.o mem_wrap.o mercury.o mfp.o palette.o midi.o pia.o rtc.o sasi.o scc.o scsi.o sram.o sysport.o tvram.o)

FMGENOBJS= $(addprefix fmgen/, fmgen.o fmg_wrap.o file.o fmtimer.o opm.o opna.o psg.o)

X11OBJS= $(addprefix x11/,joystick.o juliet.o keyboard.o mouse.o prop.o status.o timer.o dswin.o windraw.o winui.o about.o common.o)

X11CXXOBJS= x11/winx68k.o

WIN32APIOBJS= $(addprefix win32api/, dosio.o fake.o peace.o)

COBJS=		$(X68KOBJS) $(X11OBJS) $(WIN32APIOBJS) $(CPUOBJS)
CXXOBJS=	$(FMGENOBJS) $(X11CXXOBJS)
OBJS=		$(COBJS) $(CXXOBJS)

CSRCS=		$(COBJS:.o=.c)
CXXSRCS=	$(CXXOBJS:.o=.cpp)
SRCS=		$(CSRCS) $(CXXSRCS)

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) -c
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

%/%.o : %/%.c
%/%.o : %/%.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.o : %.cc
%.o : %.cc $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.o : %.cxx
%.o : %.cxx $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

all:: px68k
debug:: px68k

px68k: $(OBJS)
	$(RM) $@
	$(CXXLINK) $(MOPT) -o $@ $(CXXLDOPTIONS) $(OBJS) $(SDL_LIB) $(LDLIBS)

clean::
	$(RM) px68k
	$(RM) $(OBJS) $(C68KOBJ) $(CYCLONEOBJ)
	$(RM) *.CKP *.ln *.BAK *.bak *.o core errs ,* *~ *.a .emacs_* tags TAGS make.log MakeOut   "#"*

$(DEPDIR)/%.d : ;

.PRECIOUS: $(DEPDIR)/%.d

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))
