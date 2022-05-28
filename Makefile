CFLAGS	:=	-std=c99 -pedantic \
		-Wall -Wextra \
		-O3 -s \
		-ffunction-sections -fdata-sections \
		-flto -fuse-linker-plugin

LDFLAGS	:=	-Wl,-x -Wl,--gc-sections

.PHONY:	all clean

all: aout2lda.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o aout2lda aout2lda.c

clean:
	rm -f aout2lda
