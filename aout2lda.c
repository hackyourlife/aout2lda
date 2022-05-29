#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define U16B(x)		(x)
#define U16L(x)		__builtin_bswap16(x)
#else
#define U16B(x)		__builtin_bswap16(x)
#define U16L(x)		(x)
#endif

typedef struct {
	u16		magic;
	u16		text;
	u16		data;
	u16		bss;
	u16		symbols;
	u16		entry;
	u16		unused;
	u16		flags;
} AOUT;

typedef struct {
	u16		module;
	u16		offset;
	u16		type;
	u16		value;
} SYM;

typedef struct {
	char		name[8];
	u16		type;
	u16		value;
} SHORTSYM;

typedef struct {
	u8		magic_one;
	u8		magic_zero;
	u16		length;
	u16		address;
} LDA_RECORD;

u8 sum(const void* data, size_t length)
{
	u8 s = 0;

	const u8* p = (const u8*) data;
	while(length--) {
		s += *(p++);
	}

	return s;
}

void write_lda_record(FILE* out, const u16 address, const char* data, const u16 length)
{
	LDA_RECORD header = {
		.magic_one = 1,
		.magic_zero = 0,
		.length = U16L(length) + sizeof(LDA_RECORD),
		.address = U16L(address)
	};

	u8 checksum = sum(&header, sizeof(header));
	checksum += sum(data, length);

	fwrite(&header, sizeof(header), 1, out);
	fwrite(data, length, 1, out);
	checksum = -checksum;
	fwrite(&checksum, 1, 1, out);
}

u16 parse_int(const char* s)
{
	u16 val = 0;
	const char* p = s;
	if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
		/* hex string */
		p += 2;
		while(*p) {
			if(*p >= '0' && *p <= '9') {
				val <<= 4;
				val |= *p - '0';
			} else if(*p >= 'A' && *p <= 'F') {
				val <<= 4;
				val |= *p - 'A' + 10;
			} else if(*p >= 'a' && *p <= 'f') {
				val <<= 4;
				val |= *p - 'a' + 10;
			} else {
				printf("Error: not a number \"%s\"\n", s);
				exit(1);
			}
			p++;
		}
		return val;
	} else if(s[0] == '0') {
		/* octal string */
		p++;
		while(*p) {
			if(*p >= '0' && *p <= '7') {
				val <<= 3;
				val |= *p - '0';
				p++;
			} else {
				printf("Error: not a number \"%s\"\n", s);
				exit(1);
			}
		}
		return val;
	} else {
		/* decimal string */
		while(*p) {
			if(*p >= '0' && *p <= '9') {
				val *= 10;
				val += *p - '0';
				p++;
			} else {
				printf("Error: not a number \"%s\"\n", s);
				exit(1);
			}
		}
		return val;
	}
}

static inline char* strip_underline(char* name)
{
	if(*name == '_' || *name == '~') {
		return name + 1;
	} else {
		return name;
	}
}

char* get_symbol(u16 type, char* name, char* t)
{
	switch(type) {
		case 000:		/* undefined symbol */
			*t = 'U';
			return name;
		case 037:		/* file name symbol (produced by ld) */
			*t = 'F';
			return name;
		case 040:		/* undefined external (.globl) symbol */
			*t = 'u';
			return name;
		default:
			*t = '?';
			return name;

		case 002:		/* text segment symbol */
			*t = 'T';
			if(*name == '/') {
				return name + 1;
			} else if(name[strlen(name) - 2] == '.' &&
					name[strlen(name) - 1] == 'o') {
				return name;
			} else {
				return strip_underline(name);
			}
		case 001:		/* absolute symbol */
			*t = 'A';
			return strip_underline(name);
		case 041:		/* absolute external symbol */
			*t = 'a';
			return strip_underline(name);
		case 042:		/* text segment external symbol */
			*t = 't';
			return strip_underline(name);
		case 003:		/* data segment symbol */
			*t = 'D';
			return strip_underline(name);
		case 004:		/* bss segment symbol */
			*t = 'B';
			return strip_underline(name);
		case 043:		/* data segment external symbol */
			*t = 'd';
			return strip_underline(name);
		case 044:		/* bss segment external symbol */
			*t = 'b';
			return strip_underline(name);
	}
}

int main(int argc, char** argv)
{
	const char* aout = "a.out";
	const char* lda = "a.lda";
	u16 text_addr = 01000;
	u16 data_align = 256;
	int vector0 = 0;
	int printsym = 0;

	u16 data_addr;
	u16 bss_addr;
	u16 total;

	const char* self = *argv;

	AOUT header;

	FILE* in;
	FILE* out;

	void* buffer;

	argc--;
	argv++;

	while(argc) {
		if(!strcmp(*argv, "--help") || !strcmp(*argv, "-h")) {
			printf("Usage: %s [options]\n"
				"\n"
				"  -h, --help          show this help message and exit\n"
				"  --aout AOUT         a.out input file\n"
				"  --lda LDA           LDA output file\n"
				"  --text TEXT         text load address\n"
				"  --data-align ALIGN  data alignment\n"
				"  --vector0           store JMP entry at vector 0\n"
				"  --sym               print symbol table\n",
				self);
			return 0;
		} else if(!strcmp(*argv, "--aout")) {
			argc--;
			argv++;

			if(!argc) {
				printf("Missing argument: AOUT\n");
				return 1;
			}

			aout = *argv;
		} else if(!strcmp(*argv, "--lda")) {
			argc--;
			argv++;

			if(!argc) {
				printf("Missing argument: LDA\n");
				return 1;
			}

			lda = *argv;
		} else if(!strcmp(*argv, "--text")) {
			argc--;
			argv++;

			if(!argc) {
				printf("Missing argument: TEXT\n");
				return 1;
			}

			text_addr = parse_int(*argv);
		} else if(!strcmp(*argv, "--data-align")) {
			argc--;
			argv++;

			if(!argc) {
				printf("Missing argument: ALIGN\n");
				return 1;
			}

			data_align = parse_int(*argv);
		} else if(!strcmp(*argv, "--vector0")) {
			vector0 = 1;
		} else if(!strcmp(*argv, "--sym")) {
			printsym = 1;
		} else {
			printf("Unknown option: %s\n", *argv);
			return 1;
		}

		argc--;
		argv++;
	}

	in = fopen(aout, "rb");
	if(!in) {
		perror("Cannot open input file");
		return 1;
	}

	out = fopen(lda, "wb");
	if(!out) {
		perror("Cannot open output file");
		return 1;
	}

	fread(&header, sizeof(header), 1, in);

	if(U16L(header.magic) == 0x0407 && data_align != 2) {
		printf("*** WARNING: impure executable, but alignment not 2\n");
	}

	data_addr = (text_addr + U16L(header.text) + data_align - 1) & ~(data_align - 1);
	bss_addr = data_addr + U16L(header.data);

	total = data_addr + U16L(header.data) + U16L(header.bss);

	printf("      Magic    Text +    Len    Data +    Len     BSS = MaxMem   Entry\n"
	       "Hex:   %04x    %04x +   %04x    %04x +   %04x    %04x     %04x    %04x\n"
	       "Dec:  %5d   %5d +  %5d   %5d +  %5d   %5d    %5d   %5d\n"
	       "Oct: %06o  %06o + %06o  %06o + %06o  %06o   %06o  %06o\n",
	       U16L(header.magic),
	       text_addr, U16L(header.text),
	       data_addr, U16L(header.data),
	       U16L(header.bss), total, U16L(header.entry),
	       U16L(header.magic),
	       text_addr, U16L(header.text),
	       data_addr, U16L(header.data),
	       U16L(header.bss), total, U16L(header.entry),
	       U16L(header.magic),
	       text_addr, U16L(header.text),
	       data_addr, U16L(header.data),
	       U16L(header.bss), total, U16L(header.entry));

	/* text section */
	buffer = malloc(U16L(header.text));
	if(!buffer) {
		printf("not enough memory\n");
		return 1;
	}

	fread(buffer, U16L(header.text), 1, in);
	write_lda_record(out, text_addr, buffer, U16L(header.text));
	free(buffer);

	/* data and BSS section */
	if(header.data) {
		buffer = malloc(U16L(header.data));
		if(!buffer) {
			printf("not enough memory\n");
			return 1;
		}

		fread(buffer, U16L(header.data), 1, in);
		write_lda_record(out, data_addr, buffer, U16L(header.data));
		free(buffer);
	}

	/* make sure BSS section starts evenly aligned and initialize with zeros */
	if(header.bss) {
		buffer = malloc(U16L(header.bss));
		if(!buffer) {
			printf("not enough memory\n");
			return 1;
		}

		memset(buffer, 0, U16L(header.bss));
		write_lda_record(out, bss_addr, buffer, U16L(header.bss));
		free(buffer);
	}

	/* entry */
	if(vector0) {
		const char buf[4] = { 0x5F, 0, (u8) U16L(header.entry),
			(u8) (U16L(header.entry) >> 8) };
		write_lda_record(out, 0, buf, 4);
	}

	/* end */
	write_lda_record(out, U16L(header.entry), NULL, 0);

	fclose(out);

	/* print symbol table */
	if(printsym) {
		void* symbols = malloc(U16L(header.symbols));
		if(!symbols) {
			printf("not enough memory\n");
			return 1;
		}
		fread(symbols, U16L(header.symbols), 1, in);
		if(*((u8*) symbols) == 0) {
			SYM* sym = (SYM*) symbols;
			u16 namelens[2];
			u32 namelen;
			char* names;
			char type;
			char* name;

			fread(&namelens, 2, 2, in);
			namelen = U16L(namelens[0]) << 16 | U16L(namelens[1]);
			names = (char*) malloc(namelen + 4);
			if(!names) {
				printf("not enough memory\n");
				return 1;
			}
			memcpy(names, namelens, 4);
			fread(names + 4, namelen, 1, in);

			for(unsigned int i = 0; i < U16L(header.symbols); i += 8, sym++) {
				name = get_symbol(U16L(sym->type), &names[sym->offset], &type);
				printf("%07o %6d %06o %02o [%c] %s\n",
						U16L(sym->module), U16L(sym->offset),
						U16L(sym->value), U16L(sym->type),
						type, name);
			}

			free(names);
		} else {
			SHORTSYM* sym = (SHORTSYM*) symbols;
			char name[9];
			name[8] = 0;
			for(unsigned int i = 0; i < U16L(header.symbols); i += 12) {
				memcpy(name, sym->name, 8);
				printf("%-8s %06o %02o\n", name, U16L(sym->value),
						U16L(sym->type));
			}
		}
		free(symbols);
	}

	fclose(in);
}
