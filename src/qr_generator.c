/**
 * mod_qr - QR generator module for Apache
 *
 * mod_qr
 * Copyright (C) 2011, Siavash Ghorbani <siavash@tricycle.se>
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <getopt.h>
#include <qrencode.h>

#include "qr_config.h"
#include "qr_generator.h"

static int casesensitive = 1;
static int eightbit = 0;
static int version = 0;
static int size = 4;
static int margin = 4;
static int structured = 0;
static QRecLevel level = QR_ECLEVEL_L;
static QRencodeMode hint = QR_MODE_8;


#define MAX_DATA_SIZE (7090 * 16) /* from the specification */

void my_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct mem_encode *p = (struct mem_encode*)png_get_io_ptr(png_ptr);
  size_t nsize = p->size + length;

  /* allocate or grow buffer */
  if(p->buffer){
    p->buffer = realloc(p->buffer, nsize);
	} else {
    p->buffer = malloc(nsize);
	}

  if(!p->buffer)
    png_error(png_ptr, "Write Error");
  /* copy new bytes to end of buffer */
  memcpy(p->buffer + p->size, data, length);
  p->size += length;
}


static int createPNG(QRcode *qrcode, struct mem_encode *state)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char *row, *p, *q;
	int x, y, xx, yy, bit;
	int realwidth;

	state->buffer = NULL;
	state->size = 0;

	realwidth = (qrcode->width + margin * 2) * size;
	row = (unsigned char *)malloc((realwidth + 7) / 8);
	if(row == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL) {
		fprintf(stderr, "Failed to initialize PNG writer.\n");
		exit(EXIT_FAILURE);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) {
		fprintf(stderr, "Failed to initialize PNG write.\n");
		exit(EXIT_FAILURE);
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fprintf(stderr, "Failed to write PNG image.\n");
		exit(EXIT_FAILURE);
	}

	png_init_io(png_ptr, NULL);
	png_set_write_fn(png_ptr, state, my_png_write_data, NULL);
	png_set_IHDR(png_ptr, info_ptr,
			realwidth, realwidth,
			1,
			PNG_COLOR_TYPE_GRAY,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	/* top margin */
	memset(row, 0xff, (realwidth + 7) / 8);
	for(y=0; y<margin * size; y++) {
		png_write_row(png_ptr, row);
	}

	/* data */
	p = qrcode->data;
	for(y=0; y<qrcode->width; y++) {
		bit = 7;
		memset(row, 0xff, (realwidth + 7) / 8);
		q = row;
		q += margin * size / 8;
		bit = 7 - (margin * size % 8);
		for(x=0; x<qrcode->width; x++) {
			for(xx=0; xx<size; xx++) {
				*q ^= (*p & 1) << bit;
				bit--;
				if(bit < 0) {
					q++;
					bit = 7;
				}
			}
			p++;
		}
		for(yy=0; yy<size; yy++) {
			png_write_row(png_ptr, row);
		}
	}
	/* bottom margin */
	memset(row, 0xff, (realwidth + 7) / 8);
	for(y=0; y<margin * size; y++) {
		png_write_row(png_ptr, row);
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(row);

	return 0;
}

static QRcode *encode(const char *intext)
{
	QRcode *code;

	if(eightbit) {
		code = QRcode_encodeString8bit(intext, version, level);
	} else {
		code = QRcode_encodeString(intext, version, level, hint, casesensitive);
	}

	return code;
}

void qrPNG(const char *intext, struct mem_encode *png)
{
	QRcode *qrcode;
	
	qrcode = encode(intext);
	if(qrcode == NULL) {
		perror("Failed to encode the input data:");
		exit(EXIT_FAILURE);
	}
	createPNG(qrcode, png);
	QRcode_free(qrcode);
}

static QRcode_List *encodeStructured(const char *intext)
{
	QRcode_List *list;

	if(eightbit) {
		list = QRcode_encodeString8bitStructured(intext, version, level);
	} else {
		list = QRcode_encodeStringStructured(intext, version, level, hint, casesensitive);
	}

	return list;
}
