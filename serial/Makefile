all: serial compare

serial: cmp.c pgm.c utils.c cmp.h main.c 
	ppu-gcc $^ -O3 -Wall -lm -o serial

compare: cmp.c pgm.c utils.c cmp.h compare.c
	ppu-gcc $^ -O3 -Wall -Werror -lm -o compare

clean:
	rm -f compare serial *.pgm *.cmp
