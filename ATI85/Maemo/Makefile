include ../../EMULib/Rules.Maemo

DEFINES+= -DATI85 -DNO_CONDEBUG -DDEBUG
CFLAGS += -I$(LIBZ80)
OBJECTS+= $(Z80) ../TI85.o Maemo.o LoadPNG.o

all:    ati85

ati85:	Makefile $(OBJECTS)
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)
