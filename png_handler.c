/*
 * Guillaume Cottenceau (gc at mandrakesoft.com)
 *
 * Copyright 2002 MandrakeSoft
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 *
 * modified by cwbowron for pspChess
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

#include "bce.h"
#include "piece_bitmaps.h"
#include "colors.h"

int abort_(char *s)
{
    user_message_wait_key(s);
    return -1;
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

// return 0 if it went well
int read_png_file(char* file_name)
{
    char header[8];	// 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(file_name, "rb");
    if (!fp)
	return abort_("[read_png_file] File could not be opened for reading");
    fread(header, 1, 8, fp);
    if (png_sig_cmp((png_bytep) header, 0, 8))
	return abort_("[read_png_file] File is not recognized as a PNG file");


    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
    if (!png_ptr)
	return abort_("[read_png_file] png_create_read_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	return abort_("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
	return abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    width = info_ptr->width;
    height = info_ptr->height;
    color_type = info_ptr->color_type;
    bit_depth = info_ptr->bit_depth;

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
	return abort_("[read_png_file] Error during read_image");

    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (y=0; y<height; y++)
	row_pointers[y] = (png_byte*) malloc(info_ptr->rowbytes);

    png_read_image(png_ptr, row_pointers);

    fclose(fp);

    return 0;
}


int write_png_file(char* file_name)
{
    /* create file */
    FILE *fp = fopen(file_name, "wb");
    if (!fp)
	return abort_("[write_png_file] file could not be opened for writing");


    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
    if (!png_ptr)
	return abort_("[write_png_file] png_create_write_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	return abort_("[write_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
	return abort_("[write_png_file] Error during init_io");

    png_init_io(png_ptr, fp);


    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
	return abort_("[write_png_file] Error during writing header");

    png_set_IHDR(png_ptr, info_ptr, width, height,
		 bit_depth, color_type, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);


    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
	return abort_("[write_png_file] Error during writing bytes");

    png_write_image(png_ptr, row_pointers);


    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
	return abort_("[write_png_file] Error during end of write");

    png_write_end(png_ptr, NULL);

    /* cleanup heap allocation */
    for (y=0; y<height; y++)
	free(row_pointers[y]);
    free(row_pointers);

    fclose(fp);

    return 0;
}


void process_pieces_file(void)
{
    int byte_count = 4;
    
    switch (info_ptr->color_type)
    {
	case PNG_COLOR_TYPE_RGBA:
	    byte_count = 4;
	    break;
	case PNG_COLOR_TYPE_RGB:
	    byte_count = 3;
	    break;
	default:
	    user_message_wait_key("Unknown color type in PNG");
	    return;
    }

    if (info_ptr->width < 7*32 ||
	info_ptr->height < 4*32)
    {
	user_message_wait_key("Image is too small");
	return;
    }
    
    struct {
	unsigned short *bitmap;
	int offsetx;
	int offsety;
    } piece_mapping[] =	{
	{white_emptyData,0,32},
	{white_w_pawn_pngData,32,96},
	{white_b_pawn_pngData,32,32},
	{white_w_knight_pngData ,64,96},
	{white_b_knight_pngData,64,32},
	{white_w_bishop_pngData,96,96},
	{white_b_bishop_pngData,96,32},
	{white_w_rook_pngData,128,96},
	{white_b_rook_pngData,128,32},
	{white_w_queen_pngData,160,96},
	{white_b_queen_pngData,160,32},
	{white_w_king_pngData,192,96},
	{white_b_king_pngData,192,32},
	{ black_emptyData,0,0},
	{ black_w_pawn_pngData,32,64},
	{ black_b_pawn_pngData,32,0},
	{ black_w_knight_pngData ,64,64},
	{ black_b_knight_pngData,64,0},
	{ black_w_bishop_pngData,96,64},
	{ black_b_bishop_pngData,96,0},
	{ black_w_rook_pngData,128,64},
	{ black_b_rook_pngData,128,0},
	{ black_w_queen_pngData,160,64},
	{ black_b_queen_pngData,160,0},
	{ black_w_king_pngData,192,64},
	{ black_b_king_pngData,192,0},
    };
    
    int i;
    int max_i = sizeof(piece_mapping)/sizeof(piece_mapping[0]);

    for (i=0;i<max_i;i++)
    {
	for (y=0;y<32;y++)
	{
	    int ab_row = y+piece_mapping[i].offsety;
	    
	    png_byte* row = row_pointers[ab_row];
	    for (x=0; x<32; x++)
	    {
		int ab_x = (piece_mapping[i].offsetx+x)*byte_count;
		
		png_byte* ptr = &(row[ab_x]);

		unsigned short color = rgb2col(ptr[0],ptr[1],ptr[2]);

		piece_mapping[i].bitmap[y*32+x] = color;
	    }
	}
    }
}

#if 0
void process_file(void)
{
    int byte_count = 4;
    
    switch (info_ptr->color_type)
    {
	case PNG_COLOR_TYPE_RGBA:
	    byte_count = 4;
	    break;
	case PNG_COLOR_TYPE_RGB:
	    byte_count = 3;
	    break;
	default:
	    user_message_wait_key("Unknown color type in PNG");
	    return;
    }

    //clear_screen();
    
    for (y=0; y<height; y++)
    {
	png_byte* row = row_pointers[y];
	for (x=0; x<width; x++)
	{
	    png_byte* ptr = &(row[x*byte_count]);

	    unsigned short color = rgb2col(ptr[0],ptr[1],ptr[2]);
	    
	    set_pixel(x,y,color);
	}
    }

    pgScreenFlipV();

    while (0 == Read_Key());
}

#endif

/* int main(int argc, char **argv) */
/* { */
/* 	if (argc != 3) */
/* 		return abort_("Usage: program_name <file_in> <file_out>"); */

/* 	read_png_file(argv[1]); */
/* 	process_file(); */
/* 	write_png_file(argv[2]); */

/*         return 0; */
/* } */

