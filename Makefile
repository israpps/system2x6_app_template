EE_BIN = ACRAM_TESTER.ELF

EE_LIBS += -L. -lpatches -lfileXio -lcdvd -lpadx -ldebug
EE_OBJS_DIR = iop/
IOP_MODULES = 
EE_SOURCE = main.o
EE_OBJS = $(addprefix src/, $(EE_SOURCE) ) $(addsuffix .o, $(addprefix $(EE_OBJS_DIR), $(IOP_MODULES)))
DEBUG ?= 0
EE_CFLAGS += -fdata-sections -ffunction-sections -DNEWLIB_PORT_AWARE
EE_LDFLAGS += -Wl,--gc-sections

ifdef PROGVER
  EE_CFLAGS = -DPROGVER=\"$(PROGVER)\"
endif

all: $(EE_BIN_DEPS) $(EE_BIN) 

clean:
	rm -rf $(EE_OBJS) $(EE_BIN)

vpath %.irx iop/
vpath %.irx $(PS2SDK)/iop/irx/
IRXTAG = $(notdir $(addsuffix _irx, $(basename $<)))
$(EE_OBJS_DIR)%.c: %.irx
	$(DIR_GUARD)
	@bin2c $< $@ $(IRXTAG)


include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal