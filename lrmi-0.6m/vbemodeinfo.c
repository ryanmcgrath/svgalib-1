/*
List the available VESA graphics and text modes.

This program is in the public domain.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <sys/stat.h>

#include "lrmi.h"
#include "vbe.h"

struct
	{
	struct vbe_info_block *info;
	struct vbe_mode_info_block *mode;
	} vbe;

int
main(int argc, char *argv[]) {
	struct LRMI_regs r;
        int mode;

        if((argc!=2)||((mode=atoi(argv[1]))==0)){
           printf("usage: vbemodeinfo <mode>\n"
                  "where <mode> is a vesa mode numder.\n"
                  "use vbetest to list available modes\n\n");
           return 0;
        };

	if (!LRMI_init())
		return 1;

	vbe.info = LRMI_alloc_real(sizeof(struct vbe_info_block)
	 + sizeof(struct vbe_mode_info_block));

	if (vbe.info == NULL) {
		fprintf(stderr, "Can't alloc real mode memory\n");
		return 1;
        }

	vbe.mode = (struct vbe_mode_info_block *)(vbe.info + 1);

	/*
	 Allow read/write to all IO ports
	*/
	ioperm(0, 0x400 , 1);
	iopl(3);

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f00;
	r.es = (unsigned int)vbe.info >> 4;
	r.edi = 0;

	memcpy(vbe.info->vbe_signature, "VBE2", 4);

	if (!LRMI_int(0x10, &r)) {
		fprintf(stderr, "Can't get VESA info (vm86 failure)\n");
		return 1;
        }

	if ((r.eax & 0xffff) != 0x4f || strncmp(vbe.info->vbe_signature, "VESA", 4) != 0) {
		fprintf(stderr, "No VESA bios\n");
		return 1;
	}

	printf("VBE Version %x.%x\n",
	 (int)(vbe.info->vbe_version >> 8) & 0xff,
	 (int)vbe.info->vbe_version & 0xff);

	printf("%s\n",
	 (char *)(vbe.info->oem_string_seg * 16 + vbe.info->oem_string_off));

        memset(&r, 0, sizeof(r));

	r.eax = 0x4f01;
	r.ecx = mode;
	r.es = (unsigned int)vbe.mode >> 4;
	r.edi = (unsigned int)vbe.mode & 0xf;

	if (!LRMI_int(0x10, &r)) {
		fprintf(stderr, "Can't get mode info (vm86 failure)\n");
		return 1;
	}

	printf("mode %i\n",mode);

        printf ("mode_attributes = %i\n", vbe.mode->mode_attributes);
        printf ("win_a_attributes = %i\n", vbe.mode->win_a_attributes);
        printf ("win_b_attributes = %i\n", vbe.mode->win_b_attributes);
        printf ("win_granularity = %i\n", vbe.mode->win_granularity);
        printf ("win_size = %i\n", vbe.mode->win_size);
        printf ("win_a_segment = %i\n", vbe.mode->win_a_segment);
        printf ("win_b_segment = %i\n", vbe.mode->win_b_segment);
        printf ("win_func_ptr_off = %i\n", vbe.mode->win_func_ptr_off);
        printf ("win_func_ptr_seg = %i\n", vbe.mode->win_func_ptr_seg);
        printf ("bytes_per_scanline = %i\n", vbe.mode->bytes_per_scanline);
        printf ("x_resolution = %i\n", vbe.mode->x_resolution);
        printf ("y_resolution = %i\n", vbe.mode->y_resolution);
        printf ("x_char_size = %i\n", vbe.mode->x_char_size);
        printf ("y_char_size = %i\n", vbe.mode->y_char_size);
        printf ("number_of_planes = %i\n", vbe.mode->number_of_planes);
        printf ("bits_per_pixel = %i\n", vbe.mode->bits_per_pixel);
        printf ("number_of_banks = %i\n", vbe.mode->number_of_banks);
        printf ("memory_model = %i\n", vbe.mode->memory_model);
        printf ("bank_size = %i\n", vbe.mode->bank_size);
        printf ("number_of_image_pages = %i\n", vbe.mode->number_of_image_pages);
        printf ("res1 = %i\n", vbe.mode->res1);
        printf ("red_mask_size = %i\n", vbe.mode->red_mask_size);
        printf ("red_field_position = %i\n", vbe.mode->red_field_position);
        printf ("green_mask_size = %i\n", vbe.mode->green_mask_size);
        printf ("green_field_position = %i\n", vbe.mode->green_field_position);
        printf ("blue_mask_size = %i\n", vbe.mode->blue_mask_size);
        printf ("blue_field_position = %i\n", vbe.mode->blue_field_position);
        printf ("rsvd_mask_size = %i\n", vbe.mode->rsvd_mask_size);
        printf ("rsvd_field_position = %i\n", vbe.mode->rsvd_field_position);
        printf ("direct_color_mode_info = %i\n", vbe.mode->direct_color_mode_info);
        printf ("phys_base_ptr = %i\n", vbe.mode->phys_base_ptr);
        printf ("offscreen_mem_offset = %i\n", vbe.mode->offscreen_mem_offset);
        printf ("offscreen_mem_size = %i\n", vbe.mode->offscreen_mem_size);

    
	LRMI_free_real(vbe.info);

	return 0;
}
