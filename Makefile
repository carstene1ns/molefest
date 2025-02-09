# molefest
TARGET  = molefest

DEFINES = -DSOUNDON -DMUSICON
CFLAGS ?= -g -Wall -O2
CFLAGS += `allegro-config --cflags` -Ilib/jgmod
LIBS    = `allegro-config --libs` -lm

OBJS    = src/molefest.o \
          src/msound.o

# jgmod
JGMOD_CFLAGS = -O3 -W -Wno-unused -Wall -ffast-math -fomit-frame-pointer -funroll-loops -fPIC
OBJS   += lib/jgmod/file_io.o \
          lib/jgmod/load_mod.o \
          lib/jgmod/load_s3m.o \
          lib/jgmod/load_xm.o \
          lib/jgmod/load_it.o \
          lib/jgmod/load_jgm.o \
          lib/jgmod/save_jgm.o \
          lib/jgmod/mod.o \
          lib/jgmod/player.o \
          lib/jgmod/player2.o \
          lib/jgmod/player3.o \
          lib/jgmod/player4.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFINES) -o $@ -c $<

lib/jgmod/%.o: lib/jgmod/%.c
	$(CC) $(JGMOD_CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJS) $(TARGET)
