#include "utf8.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* BLOCK FUNCTIONS */

void utf8_block_init(utf8_block_t *block) {
	block->nbytes = 0;
}
unsigned char *utf8_block_decode(utf8_block_t *block, unsigned char *nbytes) {
	unsigned char *data = malloc(sizeof(unsigned char) * block->nbytes);
	*nbytes = 0;
	if (data != 0) {
		*nbytes = block->nbytes;
		memcpy(data, block->data, block->nbytes);
	}
	return data;
}
int utf8_block_encode(utf8_block_t *block, unsigned char *data, unsigned char nbytes) {
	int ret = UTF8_ERR_NONE;
	if (nbytes >= 1 && nbytes <= 4) {
		memcpy(block->data, data, nbytes);
	} else {
		ret = UTF8_ERR_NBYTES;
	}
	return ret;
}
void utf8_block_free(utf8_block_t *block) {
	// set bytes usage back to 0
	utf8_block_init(block);
}

/* STRING FUNCTIONS */

void utf8_string_init(utf8_string_t *str) {
	str->blocks = 0;
	str->block_count = 0;
	str->size = 0;
}

unsigned int utf8_string_get_length(utf8_string_t *str) {
	return str->block_count;
}

int utf8_string_from_bytes(utf8_string_t *str, unsigned char *data, unsigned int nbytes) {
	int ret = UTF8_ERR_NONE;
	// allocate blocks with nbytes => assume every block is 1 byte => faster
	str->blocks = malloc(sizeof(utf8_block_t) * nbytes);
	if (str->blocks != 0) {
		str->size = nbytes;
		// if needed => realloc at the end for how many blocks counted
		unsigned char *ptr = data;
		while (*ptr) {
			// walk through the whole file once and copy contents of characters
			if (*ptr < 128) {
				str->blocks[str->block_count].nbytes = 1;
				utf8_block_encode(&str->blocks[str->block_count], ptr, 1);
			} else if (*ptr < 224) {
				str->blocks[str->block_count].nbytes = 2;
				utf8_block_encode(&str->blocks[str->block_count], ptr, 2);
			} else if (*ptr < 240) {
				str->blocks[str->block_count].nbytes = 3;
				utf8_block_encode(&str->blocks[str->block_count], ptr, 3);
			} else {
				str->blocks[str->block_count].nbytes = 4;
				utf8_block_encode(&str->blocks[str->block_count], ptr, 4);
			}
			ptr += str->blocks[str->block_count].nbytes;
			++str->block_count;
		}
		// assure that the whole file is parsed
		if (ptr != (data + nbytes)) {
			// parsing or some other error
			ret = UTF8_ERR_INTERNAL;
		}
	} else {
		ret = UTF8_ERR_ALLOC;
	}
	return ret;
}

unsigned char *utf8_string_get_C_str(utf8_string_t *str) {
	unsigned char *buff = malloc(sizeof(unsigned char) * (str->size + 1));
	if (buff) {
		char *ptr = buff;
		for (unsigned int i = 0; i < str->block_count; i++) {
			memcpy(ptr, str->blocks[i].data, str->blocks[i].nbytes);
			ptr += str->blocks[i].nbytes;
		}
		// end with 0
		*ptr = 0;
	}
	return buff;
}

int utf8_string_print(utf8_string_t *str, FILE *out) {
	// iterate blocks and output each one
	utf8_block_t *block = 0;
	for (unsigned int i = 0; i < str->block_count; i++) {
		block = &str->blocks[i];
		switch (block->nbytes) {
			case 1:
				fprintf(out, "%c", block->data[0]);
				break;
			case 2:
				fprintf(out, "%c%c", block->data[0], block->data[1]);
				break;
			case 3:
				fprintf(out, "%c%c%c", block->data[0], block->data[1], block->data[2]);
				break;
			case 4:
				fprintf(out, "%c%c%c%c", block->data[0], block->data[1], block->data[2], block->data[3]);
				break;
		}
	}
}

void utf8_string_free(utf8_string_t *str) {
	if (str->blocks) {
		for (unsigned int i = 0; i < str->block_count; i++) {
			utf8_block_free(&str->blocks[i]);
		}
		free(str->blocks);
	}
	// set all back to 0
	utf8_string_init(str);
}

/* FILE FUNCTIONS */

void utf8_file_init(utf8_file_t *file) {
	utf8_string_init(&file->str);
}

int utf8_file_open(utf8_file_t *file, const char *fn) {
	FILE *fptr = fopen(fn, "rb");
	long flen = 0;
	unsigned char *data = 0;

	if (fptr == 0) {
		return UTF8_ERR_FOPEN;
	}

	fseek(fptr, 0, SEEK_END);
	flen = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	data = malloc(sizeof(unsigned char) * (flen + 1));
	if (data == 0) {
		return UTF8_ERR_ALLOC;
	}
	fread(data, sizeof(unsigned char), flen, fptr);
	// close file after reading
	fclose(fptr);
	// make last char 0
	data[flen] = 0;

	// convert from bytes to utf8 encoded
	utf8_string_from_bytes(&file->str, data, flen);

	// free data after conversion
	free(data);
	return UTF8_ERR_NONE;
}

int utf8_file_print(utf8_file_t *file, FILE *out) {
	return utf8_string_print(&file->str, out);
}

void utf8_file_free(utf8_file_t *file) {
	utf8_string_free(&file->str);
}