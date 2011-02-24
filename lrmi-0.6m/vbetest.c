/*
List the available VESA graphics and text modes.

This program is in the public domain.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

void *
save_state(void)
	{
	struct LRMI_regs r;
	void *buffer;

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f04;
	r.ecx = 0xf; 	/* all states */
	r.edx = 0; 	/* get buffer size */

	if (!LRMI_int(0x10, &r))
		{
		fprintf(stderr, "Can't get video state buffer size (vm86 failure)\n");
		return NULL;
		}

	if ((r.eax & 0xffff) != 0x4f)
		{
		fprintf(stderr, "Get video state buffer size failed\n");
		return NULL;
		}

	buffer = LRMI_alloc_real((r.ebx & 0xffff) * 64);

	if (buffer == NULL)
		{
		fprintf(stderr, "Can't allocate video state buffer\n");
		return NULL;
		}

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f04;
	r.ecx = 0xf; 	/* all states */
	r.edx = 1; 	/* save state */
	r.es = (unsigned int)buffer >> 4;
	r.ebx = (unsigned int)buffer & 0xf;

	if (!LRMI_int(0x10, &r))
		{
		fprintf(stderr, "Can't save video state (vm86 failure)\n");
		return NULL;
		}

	if ((r.eax & 0xffff) != 0x4f)
		{
		fprintf(stderr, "Save video state failed\n");
		return NULL;
		}

	return buffer;
	}

void
restore_state(void *buffer)
	{
	struct LRMI_regs r;

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f04;
	r.ecx = 0xf; 	/* all states */
	r.edx = 2; 	/* restore state */
	r.es = (unsigned int)buffer >> 4;
	r.ebx = (unsigned int)buffer & 0xf;

	if (!LRMI_int(0x10, &r))
		{
		fprintf(stderr, "Can't restore video state (vm86 failure)\n");
		}
	else if ((r.eax & 0xffff) != 0x4f)
		{
		fprintf(stderr, "Restore video state failed\n");
		}

	LRMI_free_real(buffer);
	}

void
text_mode(void)
	{
	struct LRMI_regs r;

	memset(&r, 0, sizeof(r));

	r.eax = 3;

	if (!LRMI_int(0x10, &r))
		{
		fprintf(stderr, "Can't set text mode (vm86 failure)\n");
		}
	}

void
set_mode(int n)
	{
	struct LRMI_regs r;

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f02;
	r.ebx = n;

	if (!LRMI_int(0x10, &r))
		{
		fprintf(stderr, "Can't set video mode (vm86 failure)\n");
		}
	else if ((r.eax & 0xffff) != 0x4f)
		{
		fprintf(stderr, "Set video mode failed\n");
		}

	memset(&r, 0, sizeof(r));

	r.eax = 0x4f01;
	r.ecx = n;
	r.es = (unsigned int)vbe.mode >> 4;
	r.edi = (unsigned int)vbe.mode & 0xf;

	if (!LRMI_int(0x10, &r))
		{
		fprintf(stderr, "Can't get mode info (vm86 failure)\n");
		return;
		}

	if ((r.eax & 0xffff) != 0x4f)
		{
		fprintf(stderr, "Get mode info failed\n");
		return;
		}

	/*
	 Draw a blue/red fade on top line
	*/
	if (vbe.mode->memory_model == VBE_MODEL_RGB)
		{
		char *p = (char *)(vbe.mode->win_a_segment << 4);
		int pixel_size = (vbe.mode->bits_per_pixel + 7) / 8;
		int x_res = vbe.mode->x_resolution;
		int max_r = (1 << vbe.mode->red_mask_size) - 1;
		int max_b = (1 << vbe.mode->blue_mask_size) - 1;
		int shift_r = vbe.mode->red_field_position;
		int shift_b = vbe.mode->blue_field_position;
		int i;

		for (i = 0; i < x_res; i++)
			{
			int c;
			c = (i * max_r / x_res) << shift_r;
			c |= ((x_res - i) * max_b / x_res) << shift_b;

			memcpy(p + i * pixel_size, &c, pixel_size);
			}
		for (i=2048;i<0x7fff;i++)*(p+i)=(i&0xff);
		for(i=0x8000;i<0xffff;i++)*(p+i)=85;
		}
	}

void
interactive_set_mode(void)
	{
	int n;
	void *state;
	struct stat stat;

	if (fstat(0, &stat) != 0)
		{
		fprintf(stderr, "Can't stat() stdin\n");
		return;
		}

	if ((stat.st_rdev & 0xff00) != 0x400
	 || (stat.st_rdev & 0xff) > 63)
		{
		fprintf(stderr, "To switch video modes, "
		 "this program must be run from the console\n");
		return;
		}

	printf("mode? ");
	scanf("%d", &n);

	printf("setting mode %d\n", n);

	ioctl(0, KDSETMODE, KD_GRAPHICS);

	state = save_state();

	if (state == NULL)
		return;

	set_mode(n);

	sleep(5);

	text_mode();
	restore_state(state);

	ioctl(0, KDSETMODE, KD_TEXT);
	}

int
main(void)
	{
	struct LRMI_regs r;
	short int *mode_list;

	if (!LRMI_init())
		return 1;

	vbe.info = LRMI_alloc_real(sizeof(struct vbe_info_block)
	 + sizeof(struct vbe_mode_info_block));

	if (vbe.info == NULL)
		{
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

	if (!LRMI_int(0x10, &r))
		{
		fprintf(stderr, "Can't get VESA info (vm86 failure)\n");
		return 1;
		}

	if ((r.eax & 0xffff) != 0x4f || strncmp(vbe.info->vbe_signature, "VESA", 4) != 0)
		{
		fprintf(stderr, "No VESA bios\n");
		return 1;
		}

	printf("VBE Version %x.%x\n",
	 (int)(vbe.info->vbe_version >> 8) & 0xff,
	 (int)vbe.info->vbe_version & 0xff);

	printf("%s\n",
	 (char *)(vbe.info->oem_string_seg * 16 + vbe.info->oem_string_off));

	mode_list = (short int *)(vbe.info->video_mode_list_seg * 16 + vbe.info->video_mode_list_off);

	while (*mode_list != -1)
		{
		memset(&r, 0, sizeof(r));

		r.eax = 0x4f01;
		r.ecx = *mode_list;
		r.es = (unsigned int)vbe.mode >> 4;
		r.edi = (unsigned int)vbe.mode & 0xf;

		if (!LRMI_int(0x10, &r))
			{
			fprintf(stderr, "Can't get mode info (vm86 failure)\n");
			return 1;
			}

		if (vbe.mode->memory_model == VBE_MODEL_RGB)
			printf("[%3d] %dx%d (%d:%d:%d)\n",
			 *mode_list,
			 vbe.mode->x_resolution,
			 vbe.mode->y_resolution,
			 vbe.mode->red_mask_size,
			 vbe.mode->green_mask_size,
			 vbe.mode->blue_mask_size);
		else if (vbe.mode->memory_model == VBE_MODEL_256)
			printf("[%3d] %dx%d (256 color palette)\n",
			 *mode_list,
			 vbe.mode->x_resolution,
			 vbe.mode->y_resolution);
		else if (vbe.mode->memory_model == VBE_MODEL_PACKED)
			printf("[%3d] %dx%d (%d color palette)\n",
			 *mode_list,
			 vbe.mode->x_resolution,
			 vbe.mode->y_resolution,
			 1 << vbe.mode->bits_per_pixel);
		else if (vbe.mode->memory_model == VBE_MODEL_TEXT)
			printf("[%3d] %dx%d (TEXT)\n",
			 *mode_list,
			 vbe.mode->x_resolution,
			 vbe.mode->y_resolution);
                else printf("[%3d] %dx%d (model=%d)\n",
			 *mode_list,
			 vbe.mode->x_resolution,
			 vbe.mode->y_resolution,
                         vbe.mode->memory_model);
		mode_list++;
		}

	LRMI_free_real(vbe.info);

	interactive_set_mode();

	return 0;
	}
