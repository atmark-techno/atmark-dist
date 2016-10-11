CFLAGS=		-O2 -Wall -Werror -DHAVE_CLOCK_GETTIME=1 -DHAVE_GETTIMEOFDAY=1
CFLAGS+=	-I../../install/include 
CFLAGS+=	-Wno-format-security
LDFLAGS=	-lrt  -lexpat -lssl -lcrypto -larms
SRCS=		armsd.c compat.c sys_linux.c
OBJS=		armsd.o compat.o sys_linux.o

all: armsd

armsd.o: armsd.c armsd.h
	$(CC) $(CFLAGS) -c -o $@ $<

sys_linux.o: sys_linux.c armsd.h
	$(CC) $(CFLAGS) -c -o $@ $<

armsd: $(OBJS)
	$(CC) -o armsd $(OBJS) $(LDFLAGS)

clean:
	-rm -f armsd *.o
