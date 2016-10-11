
all: gen_ha1

gen_ha1: gen_ha1.o calc.o ../../md5.o
	$(HOSTCC) $(HOSTCFLAGS) -o gen_ha1 gen_ha1.o calc.o ../../md5.o

gen_ha1.o: gen_ha1.c
	$(HOSTCC) $(HOSTCFLAGS) -c gen_ha1.c

calc.o: calc.c calc.h
	$(HOSTCC) $(HOSTCFLAGS) -c calc.c


clean:
	rm -f *~ *.o gen_ha1

