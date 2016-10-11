target = functester

objs += main.o
objs += task_backlight.o task_button.o task_led.o task_sound.o task_touch.o task_rtc.o

CFLAGS += $(shell pkg-config --cflags gtk+-2.0 alsa)
LDFLAGS += $(shell pkg-config --libs gtk+-2.0 alsa)
CFLAGS += -O2 -Wall

ifdef V
  quiet =
else
  quiet = quiet_
endif

all: $(target)

quiet_cmd_ld ?= LD      $@
      cmd_ld ?= $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
$(target): $(objs)
	@$(if $($(quiet)cmd_ld), echo '  $($(quiet)cmd_ld)' &&) $(cmd_ld)

quiet_cmd_cc ?= CC      $@
      cmd_cc ?= $(CC) $(CFLAGS) -c -o $@ $<
%.o: %.c
	@$(if $($(quiet)cmd_cc), echo '  $($(quiet)cmd_cc)' &&) $(cmd_cc)

clean:
	rm -f *.o $(target)

romfs: $(target)
	$(ROMFSINST) /usr/bin/functester
	$(ROMFSINST) /usr
