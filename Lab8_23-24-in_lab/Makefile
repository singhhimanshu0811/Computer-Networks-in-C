CC = gcc
CFLAGS = -I./impl -I./shared -I./sim
DEPS = ./shared/types.h 
OBJ = ./sim/main.o ./impl/impl.o

default: dv

./impl/impl.o: ./impl/impl.c ./impl/impl.h $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

./sim/main.o: ./sim/main.c ./sim/header.h $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

dv: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

test: dv
	./dv 4 topo.txt traffic.txt link.txt

test-inf: dv
	./dv 50 topo_inf.txt traffic_inf.txt link_inf.txt

clean:
	rm -r $(OBJ) dv

.PHONY: test test-inf clean