#include <stdlib.h>
#include <string.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

static unsigned char *cursors[16];
static int formats[16];

static struct {
    unsigned char c8;
    unsigned short c15;
    unsigned short c16;
    unsigned int c32;
} cursor_colors[16*2];

static unsigned char *buf, *dbuf;
static int cur_x, cur_y;
static int cursor;
static int cur_show;
int __svgalib_software_cursor, __svgalib_cursor_status=-1;
static int palette[768];
static int pal=1;

static int sx, sy, sp;

static int findcolor(int rgb) {
   int i,j,k,l=0;

   if(pal)vga_getpalvec(0,256,palette);
   pal=0;
   k=0xffffff;
   for(i=0;i<256;i++) {
      j=((rgb&0xff)-(palette[i*3+2]<<2))*((rgb&0xff)-(palette[i*3+2]<<2))+
        (((rgb>>8)&0xff)-(palette[i*3+1]<<2))*(((rgb>>8)&0xff)-(palette[i*3+1]<<2))+
        (((rgb>>16)&0xff)-(palette[i*3]<<2))*(((rgb>>16)&0xff)-(palette[i*3]<<2));
      if(j==0) {
         return i;
      }
      if(j<k) {
         k=j;
         l=i;
      }
   }
   return l;
}

static void draw_cursor(unsigned char *p)
{
    int y;
    int w = CI.bytesperpixel * 32;
    int cw = (cur_x + 32 < CI.xdim ? 32 : CI.xdim - cur_x) * CI.bytesperpixel;
    int ch = cur_y + 32 < CI.ydim ? 32 : CI.ydim - cur_y;

    for (y = cur_y; ch-- > 0; y++, p += w)
        vga_drawscansegment(p, cur_x, y, cw);
}

static void get_cursor(unsigned char *p)
{
    int y;
    int w = CI.bytesperpixel * 32;
    int cw = (cur_x + 32 < CI.xdim ? 32 : CI.xdim - cur_x) * CI.bytesperpixel;
    int ch = cur_y + 32 < CI.ydim ? 32 : CI.ydim - cur_y;
    
    for (y = cur_y; ch-- > 0; y++, p += w)
        vga_getscansegment(p, cur_x, y, cw);
}

static int software_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j;
    unsigned short *ps, c0, c1;
    unsigned int *pi, *pattern;
    
    switch(cmd) {
        case CURSOR_INIT:
            buf=(unsigned char *)malloc(32*32*4);
            dbuf=(unsigned char *)malloc(32*32*4);
            cursor=0;
            return 0;
            break;
        case CURSOR_HIDE:
            if(cur_show==1) {
                draw_cursor(buf);
                cur_show=0;
            }
            break;
        case CURSOR_SHOW:
            if(cur_show==0) {
                pattern=(unsigned int *)cursors[cursor];
                get_cursor(buf);
                memcpy(dbuf,buf,CI.bytesperpixel*32*32);
                switch(CI.bytesperpixel) {
                    case 1:
                        for(i=0;i<32;i++) {
                            unsigned int l1,l2;
                            l1=*(pattern+i);
                            l2=*(pattern+i+32);
                            for(j=0;j<32;j++) {
                                if(l2&0x80000000) {
                                    *(dbuf+i*32+j)=l1&0x80000000 ? 
                                      cursor_colors[cursor*2+1].c8 : cursor_colors[cursor*2].c8;
                                }
                                l1<<=1;
                                l2<<=1;
                            }
                        }
                        draw_cursor(dbuf);
                        break;
                    case 2:
                        ps=(unsigned short *)dbuf;
                        if(CI.colors==32768) {
                            c0=cursor_colors[cursor*2].c15;
                            c1=cursor_colors[cursor*2+1].c15;
                        } else {
                            c0=cursor_colors[cursor*2].c16;
                            c1=cursor_colors[cursor*2+1].c16;
                        }
                        for(i=0;i<32;i++) {
                            unsigned int l1,l2;
                            l1=*(pattern+i);
                            l2=*(pattern+i+32);
                            for(j=0;j<32;j++) {
                                if(l2&0x80000000) {
                                    *(ps+i*32+j)=l1&0x80000000 ? c1 : c0;
                                }
                                l1<<=1;
                                l2<<=1;
                            }
                        }
                        draw_cursor(dbuf);
                        break;
                    case 3:
                        for(i=0;i<32;i++) {
                            unsigned int l1,l2;
                            l1=*(pattern+i);
                            l2=*(pattern+i+32);
                            for(j=0;j<32;j++) {
                                if(l2&0x80000000) {
                                    *(dbuf+i*96+3*j)=l1&0x80000000 ? 
                                      cursor_colors[cursor*2+1].c32&0xff : cursor_colors[cursor*2].c32&0xff;
                                    *(dbuf+i*96+3*j+1)=l1&0x80000000 ? 
                                      (cursor_colors[cursor*2+1].c32>>8)&0xff : (cursor_colors[cursor*2].c32>>8)&0xff;
                                    *(dbuf+i*96+3*j+2)=l1&0x80000000 ? 
                                      (cursor_colors[cursor*2+1].c32>>16)&0xff:(cursor_colors[cursor*2].c32>>16)&0xff;
                                }
                                l1<<=1;
                                l2<<=1;
                            }
                        }
                        draw_cursor(dbuf);
                        break;
                    case 4:
                        pi=(unsigned int *)dbuf;
                        for(i=0;i<32;i++) {
                            unsigned int l1,l2;
                            l1=*(pattern+i);
                            l2=*(pattern+i+32);
                            for(j=0;j<32;j++) {
                                if(l2&0x80000000) {
                                    *(pi+i*32+j)=l1&0x80000000 ?  
                                      cursor_colors[cursor*2+1].c32 : cursor_colors[cursor*2].c32;
                                }
                                l1<<=1;
                                l2<<=1;
                            }
                        }
                        draw_cursor(dbuf);
                        break;
                }
                cur_show=1;
            }
            break;
        case CURSOR_POSITION:
            if(cur_show) {
                software_cursor(CURSOR_HIDE,0,0,0,0,NULL);
                cur_x=p1;
                cur_y=p2;
                software_cursor(CURSOR_SHOW,0,0,0,0,NULL);
            } else {
                cur_x=p1;
                cur_y=p2;
            }   
            break;
        case CURSOR_SELECT:
            cursor=p1;
            break;
        case CURSOR_IMAGE:
            switch(p2){
                case 0:
                    if(cursors[p1]!=NULL) {
                        free(cursors[p1]);
                    }
                    cursors[p1]=malloc(256);
                    memcpy(cursors[p1],p5,256);
                    cursor_colors[p1*2].c8=findcolor(p3);
                    cursor_colors[p1*2].c32=p3;
                    cursor_colors[p1*2].c16=((p3&0xf80000)>>8)|((p3&0xfc00)>>5)|((p3&0xf8)>>3);
                    cursor_colors[p1*2].c15=((p3&0xf80000)>>9)|((p3&0xf800)>>5)|((p3&0xf8)>>3);
                    cursor_colors[p1*2+1].c8=findcolor(p4);
                    cursor_colors[p1*2+1].c32=p4;
                    cursor_colors[p1*2+1].c16=((p4&0xf80000)>>8)|((p4&0xfc00)>>5)|((p4&0xf8)>>3);
                    cursor_colors[p1*2+1].c15=((p4&0xf80000)>>9)|((p4&0xf800)>>5)|((p4&0xf8)>>3);
                    break;
            }
    }   
    return 0;
}

void vga_showcursor(int show) {
    if(chipset_cursor==software_cursor) {
        show&=1;
    }
    if(show==1) chipset_cursor(CURSOR_SHOW,0,0,0,0,NULL); 
        else if(show==0) chipset_cursor(CURSOR_HIDE,0,0,0,0,NULL);
    __svgalib_cursor_status=show;
}

void vga_setcursorposition(int x, int y) {
    sx=x;
    sy=y;
    chipset_cursor(CURSOR_POSITION,x,y,0,0,NULL);
}

void vga_setcursorimage(int cur, int format, int c0, int c1, unsigned char *buf) {
    if(chipset_cursor!=software_cursor) {
        /* save cursor for restoring on VT switch */
        software_cursor(CURSOR_IMAGE,cur,format,c0,c1,buf);
        formats[cur]=format;
    }
    chipset_cursor(CURSOR_IMAGE,cur,format,c0,c1,buf);
}

void vga_selectcursor(int cur) {
    sp=cur;
    chipset_cursor(CURSOR_SELECT,cur,0,0,0,NULL);
}

int vga_initcursor(int sw) {
    int i;
    cur_show=0;
    for (i=0;i<16;i++)cursors[i]=NULL;
    __svgalib_cursor_status=0;

    if(sw || (chipset_cursor==NULL)) chipset_cursor=software_cursor;
    i=chipset_cursor(CURSOR_INIT,0,0,0,0,NULL);
    if(!i) chipset_cursor=software_cursor;
    return chipset_cursor!=software_cursor;
}

void __svgalib_cursor_restore() {
    int i;
    
    vga_selectcursor(sp);
    vga_setcursorposition(sx, sy);
    for (i=0;i<16;i++)if(cursors[i]!=NULL) {
        chipset_cursor(CURSOR_IMAGE,i,formats[i],cursor_colors[i*2].c32,cursor_colors[i*2+1].c32,cursors[i]);
    }
    vga_showcursor(__svgalib_cursor_status&1);
}

