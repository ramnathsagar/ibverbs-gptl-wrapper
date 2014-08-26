LIBNAME=ibverbs-wrapper
LDFLAGS= -fpic -shared
LIBS=
MAKE=gcc

ifeq ($(GPTL_SRC_PATH), )
     $(warning GPTL_SRC_PATH is not set.. Set GPTL_SRC_PATH to the source dir of GPTL.)
     $(error For example: export GPTL_SRC_PATH=$(HOME)/gptl)
endif

include $(GPTL_SRC_PATH)/macros.make

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
	$(MAKE) $(LDFLAGS) $(INCL) $(LIBS) -o lib$(LIBNAME).so ibverbs_wrapper.c
install:
	install -d $(INSTALLDIR)/lib
	install -m 0644 lib$(LIBNAME).so $(INSTALLDIR)/lib
uninstall:
	rm -rf $(INSTALLDIR)/lib/lib$(LIBNAME).so
clean:
	rm -rf *.0
distclean:
	rm -rf *.0 *.so
