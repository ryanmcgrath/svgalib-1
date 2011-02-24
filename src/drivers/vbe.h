/*
Copyright (C) 1996 by Josh Vanderhoof

You are free to distribute and modify this file, as long as you
do not remove this copyright notice and clearly label modified
versions as being modified.

This software has NO WARRANTY.  Use it at your own risk.
*/

#ifndef _VBE_H
#define _VBE_H

/* structures for vbe 2.0 */

struct vbe_info_block
	{
	char vbe_signature[4];
	short vbe_version;
	unsigned short oem_string_off;
	unsigned short oem_string_seg;
	int capabilities;
	unsigned short video_mode_list_off;
	unsigned short video_mode_list_seg;
	short total_memory;
	short oem_software_rev;
	unsigned short oem_vendor_name_off;
	unsigned short oem_vendor_name_seg;
	unsigned short oem_product_name_off;
	unsigned short oem_product_name_seg;
	unsigned short oem_product_rev_off;
	unsigned short oem_product_rev_seg;
	char reserved[222];
	char oem_data[256];
	} __attribute__ ((packed));

#define VBE_ATTR_MODE_SUPPORTED 	(1 << 0)
#define VBE_ATTR_TTY 	(1 << 2)
#define VBE_ATTR_COLOR 	(1 << 3)
#define VBE_ATTR_GRAPHICS 	(1 << 4)
#define VBE_ATTR_NOT_VGA 	(1 << 5)
#define VBE_ATTR_NOT_WINDOWED 	(1 << 6)
#define VBE_ATTR_LINEAR 	(1 << 7)

#define VBE_WIN_RELOCATABLE 	(1 << 0)
#define VBE_WIN_READABLE 	(1 << 1)
#define VBE_WIN_WRITEABLE 	(1 << 2)

#define VBE_MODEL_TEXT 	0
#define VBE_MODEL_CGA 	1
#define VBE_MODEL_HERCULES 	2
#define VBE_MODEL_PLANAR 	3
#define VBE_MODEL_PACKED 	4
#define VBE_MODEL_256 	5
#define VBE_MODEL_RGB 	6
#define VBE_MODEL_YUV 	7

struct vbe_mode_info_block
	{
	unsigned short mode_attributes;
	uint8_t win_a_attributes;
	uint8_t win_b_attributes;
	unsigned short win_granularity;
	unsigned short win_size;
	unsigned short win_a_segment;
	unsigned short win_b_segment;
	unsigned short win_func_ptr_off;
	unsigned short win_func_ptr_seg;
	unsigned short bytes_per_scanline;
	unsigned short x_resolution;
	unsigned short y_resolution;
	uint8_t x_char_size;
	uint8_t y_char_size;
	uint8_t number_of_planes;
	uint8_t bits_per_pixel;
	uint8_t number_of_banks;
	uint8_t memory_model;
	uint8_t bank_size;
	uint8_t number_of_image_pages;
	uint8_t res1;
	uint8_t red_mask_size;
	uint8_t red_field_position;
	uint8_t green_mask_size;
	uint8_t green_field_position;
	uint8_t blue_mask_size;
	uint8_t blue_field_position;
	uint8_t rsvd_mask_size;
	uint8_t rsvd_field_position;
	uint8_t direct_color_mode_info;
	unsigned int phys_base_ptr;
	unsigned int offscreen_mem_offset;
	unsigned short offscreen_mem_size;
	uint8_t res2[206];
	} __attribute__ ((packed));

struct vbe_palette_entry
	{
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t align;
	} __attribute__ ((packed));

/* CRTC Info Block */
struct vbe_crtc_info_block
{
	unsigned short	horiz_total;
	unsigned short	horiz_start;
	unsigned short	horiz_end;
	unsigned short	vert_total;
	unsigned short	vert_start;
	unsigned short	vert_end;
	uint8_t	flags;
	unsigned int	pixel_clock;
	unsigned short	refresh_rate;
	uint8_t	reserved[40];
} __attribute__ ((packed));

#endif
