include ../macros.make
LDFLAGS= -fpic -shared 
LIBS=
ifeq ($(HAVE_PAPI),yes)
  LIBS += $(PAPI_LIBFLAGS)
  INCL += $(PAPI_INCFLAGS)
endif

ifeq ($(HAVE_LIBXML2),yes)
  LIBS += $(LIBXML2_LIBFLAGS)
  INCL += $(LIBXML2_INCFLAGS)
endif

ifeq ($(HAVE_LIBRT),yes)
 LIBS += -lrt
endif

LIBS += -ldl -L/usr/local/lib -lgptl -L/usr/lib64 -libverbs

all:
	gcc $(LDFLAGS) $(INCL) $(LIBS) -o libibverbs-wrapper.so ibverbs_wrapper.c
