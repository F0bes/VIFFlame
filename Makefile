EE_BIN = VIFFlame.elf
EE_OBJS = VIFFlame.o
EE_LIBS = -lkernel -lgraph -ldma

EE_CFLAGS = -Werror

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

wsl: $(EE_BIN)
	$(PCSX2) -elf "$(shell wslpath -w $(shell pwd))/$(EE_BIN)"

emu: $(EE_BIN)
	$(PCSX2) -elf "$(shell pwd)/$(EE_BIN)"

reset:
	ps2client reset
	ps2client netdump

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
