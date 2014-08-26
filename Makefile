LDFLAGS= -fpic -shared
LIBS=

ifeq ($(GPTL_SRC_PATH), )
     $(warning GPTL_SRC_PATH is not set.. Set GPTL_SRC_PATH to the source dir of GPTL.)
     $(error For example: export GPTL_SRC_PATH=$(HOME)/gptl)
endif

include $(GPTL_SRC_PATH)/macros.make
$(warning $(HAVE_PAPI))

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

LIBS += /usr/local/lib/libgptl.a
LIBS += -ldl -L/usr/local/lib -lgptl -L/usr/lib64 -libverbs

all:
	gcc $(LDFLAGS) $(INCL) $(LIBS) -o libibverbs-wrapper.so ibverbs_wrapper.c
