# comment these to toggle them as one sees fit.
# WARNINGS will spam hundreds of warnings, mostly safe, if turned on
# DEBUG is best turned on if you plan to debug in gdb -- please do!
# PROFILE is for use with gprof or a similar program -- don't bother generally
#WARNINGS = -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter -Wno-char-subscripts
#DEBUG = -g
#PROFILE = -pg
OTHERS = -Os

ODIR = objwin
DDIR = .deps

TARGET = cataclysm.exe

CXX = g++

LINKER = i486-mingw32-ld 
LINKERFLAGS = -Wl,--subsystem,windows

CFLAGS = $(WARNINGS) $(DEBUG) $(PROFILE) $(OTHERS)

LDFLAGS = -static -lSDL.dll -lSDL_mixer.dll -mwindows
#LDFLAGS = -static -lSDL.dll -lSDL_mixer.dll -lSDL_ttf.dll -lfreetype.dll 



#ifeq ($(OS), Msys)
#LDFLAGS = -static -lpdcurses
#else
#LDFLAGS = -lncurses
#endif

SOURCES = $(wildcard *.cpp)
_OBJS = $(SOURCES:.cpp=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

all: $(TARGET)
	@

$(TARGET): $(ODIR) $(DDIR) $(OBJS)
	$(CXX) $(LINKERFLAGS) -o $(TARGET) $(CFLAGS) $(OBJS) $(LDFLAGS) -DMINGW

$(ODIR):
	mkdir $(ODIR)

$(DDIR):
	@mkdir $(DDIR)

$(ODIR)/%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(ODIR)/*.o

-include $(SOURCES:%.cpp=$(DEPDIR)/%.P)
