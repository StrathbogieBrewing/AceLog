CC         = gcc

PROGRAM    = putlog

BUILD       = $(shell date +'%y%m%d%H%M')

CSRC       = putlog.c tinframe.c tinux.c msg.c log.c mkdir.c

BUILDDIR   = bin
SOURCEDIR  = src
EXTDIR     = ext

# include directories
INCLUDE    = -I../src -I../../
VPATH      = ../../ ../src ../../AcePlot/src

CFLAGS     = -DBUILD="$(BUILD)" -DPROGRAM="$(PROGRAM)" $(INCLUDE) -g

COBJ := $(notdir $(CSRC:.c=.o) )
OBJS := $(patsubst %.o, $(BUILDDIR)/%.o, $(COBJ) )

all: dir $(BUILDDIR)/$(PROGRAM)

run:
	-pkill tinlog
	nohup ./bin/tinlog /dev/ttyUSB0 </dev/null >/dev/null 2>&1 &

TARGET	= pi@raspberrypi:software

upload:
	rsync -r ../../ $(TARGET)/Tinduino

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(BUILDDIR)/$(PROGRAM)
	rm $(BUILDDIR)/*.o

# clean files
clean:
	rm -f $(BUILDDIR)/*.o
