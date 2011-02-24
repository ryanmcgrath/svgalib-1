svgalib-1
-----------------------------------------------------------------------------------
[SVGAlib](http://svgalib.org/) is (was?) a low level graphics library for
the C programming language. It seemingly stopped seeing active development
in the early 2000's...

...because, y'know, it's also been largely superseded by drawing routines
in other environments (X, etc). I have a side project I'm working on for
emulator routines from the Super Nintendo, and to further support this
I wanted something that could run right in the framebuffer, without a 
need for X/GTK/anything else.

SDL can be compiled to use svgalib, which pretty much gives me what I need.
Problem is that svgalib, as it stands, doesn't appear to compile against
recent versions of the Linux kernel. After hunting around for a bit, I 
found cases where others seem to have to support this stuff as well, and
have thrown their own patches into the fray. To the best of my knowledge, I
don't know of anyone bringing said patches together.

Now, I don't want to maintain this library. I've better things to do with my
time; that said, Github is great for this kind of thing. If you want to contribute,
I'm happy to press a button to merge in your pull request.

This should compile on any recent Linux distribution now (tested on 32-bit Ubuntu, Maverick whatever).
This was 'forked' off of svgalib-1.9.25, a seemingly 'alpha' distribution.


Questions, Comments, Complaints?
-----------------------------------------------------------------------------------
I don't have time to actively support or maintain this in any true capacity, but
if you'd like to get in touch, I'm happy to look things over or try and help if I can
(I just make no guarantees, sadly). This is also where you should look if you think
you deserve credit for something here and it's not given.


**Email**: ryan [at] venodesigns (dot) net  
**Twitter**: **[@ryanmcgrath](http://twitter.com/ryanmcgrath)**  
**Web**: **[Veno Designs](http://venodesigns.net/)**


Legacy Notes
-----------------------------------------------------------------------------------
_Program using svgalib 1.9.0 or later don't need root privileges (suid root). 
They do need access to /dev/svga, which is a char device with major 209 and minor 0.
The module svgalib_helper need also be inserted.

To make the devices, and the module (kernel 2.4 or newer), change to directory
kernel/svgalib_helper, and type make install.

There is a compile time option to return to old behaviour, of using root
privileges to access /dev/mem, instead of svgalib helper. To compile for this
select the NO_HELPER option in Makefile.cfg._
