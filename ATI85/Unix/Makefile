include ../../EMULib/Rules.Unix

DEFINES+= -DATI85 -DLSB_FIRST -DNO_CONDEBUG -DDEBUG -DBPP32
CFLAGS += -I$(LIBZ80)
OBJECTS+= $(Z80) ../ATI85.o ../TI85.o Unix.o lodepng.o

all:    ati85

ati85:	Makefile $(OBJECTS)
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)
