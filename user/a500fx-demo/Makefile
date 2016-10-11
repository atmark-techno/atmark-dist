PROG = a500fx-demo

all: $(PROG)

$(PROG): main.c
	$(CC) `pkg-config --cflags --libs gtk+-2.0` $< -o $@

romfs:
	$(ROMFSINST) /usr/bin/$(PROG)

clean:
	rm -f *.o $(PROG)
