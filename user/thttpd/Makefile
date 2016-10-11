
EXEC = thttpd
OBJS = thttpd.o libhttpd.o fdwatch.o mmc.o timers.o match.o tdate_parse.o

HTPASSEXEC = htpasswd
HTPASSOBJS = htpasswd.o

VPATH= extras

CFLAGS += -DHAVE_FCNTL_H=1 -DHAVE_MEMORY_H=1 \
	-DHAVE_PATHS_H=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_DIRENT_H=1 \
	-DHAVE_LIBCRYPT=1 -DHAVE_STRERROR=1 -DHAVE_WAITPID=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_GETPAGESIZE=1 -DHAVE_MMAP=1 \
	-DHAVE__PROGNAME=1 -DHAVE_GRP_H=1 -DHAVE_MEMORY_H=1 \
	-DHAVE_POLL_H=1 -DHAVE_SYS_POLL_H=1 -DHAVE_VSNPRINTF=1 \
	-DHAVE_SOCKLENT=1 -DHAVE_ATOLL=1 -DHAVE_SETSID=1 \
	-DHAVE_POLL=1 -DHAVE_GETADDRINFO=1 -DHAVE_GETNAMEINFO=1 \
	-DHAVE_GAI_STRERROR=1 -DHAVE_SELECT=1 \
	-DHAVE_TM_GMTOFF=1 -DHAVE_INT64T=1
FLTFLAGS += -s 16384

target_y :=
target_$(CONFIG_USER_THTTPD_THTTPD) += $(EXEC)
target_$(CONFIG_USER_THTTPD_HTPASSWD) += $(HTPASSEXEC)

all: $(target_y)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(LIBCRYPT) $(LDLIBS_$@)

$(HTPASSEXEC): $(HTPASSOBJS)
	$(CC) $(LDFLAGS) -o $@ $(HTPASSOBJS) $(LDLIBS) $(LIBCRYPT) $(LDLIBS_$@)

romfs:
	$(ROMFSINST) -e CONFIG_USER_THTTPD_THTTPD /bin/$(EXEC)
	$(ROMFSINST) -e CONFIG_USER_THTTPD_HTPASSWD /bin/$(HTPASSEXEC)

clean:
	-rm -f $(EXEC) $(HTPASSEXEC) *.elf *.gdb *.o

libhttpd.o: mime_encodings.h mime_types.h

mime_encodings.h: mime_encodings.txt
	rm -f mime_encodings.h
	sed < mime_encodings.txt > mime_encodings.h \
	  -e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
	  -e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'

mime_types.h: mime_types.txt
	rm -f mime_types.h
	sed < mime_types.txt > mime_types.h \
	  -e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
	  -e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'

thttpd.o: config.h version.h libhttpd.h mmc.h timers.h match.h

libhttpd.o: config.h version.h libhttpd.h mime_encodings.h mime_types.h \
		mmc.h timers.h match.h tdate_parse.h

mmc.o: mmc.h

timers.o: timers.h

match.o: match.h

tdate_parse.o: tdate_parse.h

