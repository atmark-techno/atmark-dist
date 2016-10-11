TARGET = rtnotifyd

CFLAGS = -Wall -O2
OBJS = rtnotifyd.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o $(TARGET)

romfs: $(TARGET)
	$(ROMFSINST) /usr/sbin/rtnotifyd
