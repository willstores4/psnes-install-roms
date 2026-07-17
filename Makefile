ifdef OO_PS4_TOOLCHAIN
CC      := clang
CXX     := clang++
LD      := ld.lld
ODIR    := build
SDIR    := source
IDIRS   := -I$(OO_PS4_TOOLCHAIN)/include -I$(OO_PS4_TOOLCHAIN)/include/c++/v1
LDIRS   := -L$(OO_PS4_TOOLCHAIN)/lib
CFLAGS  := -target x86_64-pc-freebsd-elf -fPIC -funwind-tables -c $(IDIRS) -Os
CXXFLAGS := $(CFLAGS) -nostdinc++ -nostdlibinc -std=c++11
LDFLAGS := -m elf_x86_64 -pie --script $(OO_PS4_TOOLCHAIN)/link.x --eh-frame-hdr -L$(OO_PS4_TOOLCHAIN)/lib $(OO_PS4_TOOLCHAIN)/lib/crt1.o

LIBS    := -lc -lkernel

CFILES   := $(wildcard $(SDIR)/*.c)
CPPFILES := $(wildcard $(SDIR)/*.cpp)
OBJS     := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) $(patsubst $(SDIR)/%.cpp, $(ODIR)/%.o, $(CPPFILES))

TARGET = eboot.bin

$(TARGET): $(ODIR) $(OBJS)
	$(LD) $(ODIR)/*.o -o $(ODIR)/$(TARGET).elf $(LDFLAGS) $(LIBS)
	$(OO_PS4_TOOLCHAIN)/bin/linux/create-fself -in=$(ODIR)/$(TARGET).elf -out=$(TARGET)
	@cp $(TARGET) pkg_structure/$(TARGET)
	@echo "Build successful. eboot.bin copied to pkg_structure/"

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(ODIR):
	@mkdir -p $@

clean:
	rm -rf $(TARGET) $(ODIR) pkg_structure/$(TARGET)

else
$(error OO_PS4_TOOLCHAIN is not set. Please set it to the root of your OpenOrbis PS4 Toolchain.)
endif
