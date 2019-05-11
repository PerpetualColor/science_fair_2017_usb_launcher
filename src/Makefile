IDIR=../include
CC=gcc
CFLAGS=-I$(IDIR) -O3
ODIR=obj
LIBS=-lusb-1.0 -lm -lpthread
_DEPS=launcher.h camera.h server.h imgproc.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o launcher.o camera.o server.o imgproc.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o ../bin/$@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean: 
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
