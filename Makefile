IDIR = include
SDIR = src
CC=gcc
CFLAGS=-Wall -I$(IDIR)

ODIR=obj
LDIR=lib

LIBS=

_DEPS = protocol.h encoding.h dict.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = protocol.o encoding.o ping-responder.o dict.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ping-responder: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ .*~ $(IDIR)/*~ $(IDIR)/.*~ $(SDIR)/*~ $(SDIR)/.*~ ping-responder

