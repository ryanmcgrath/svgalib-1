PROGRAM ScrollTest;

Uses svgalib;

type
  VideoMemType = array[0..65535] of byte;

var
  i,mode,StartupMode	   : longint;
  SeparateReadWriteWindows : boolean;
  modeinfo		   : ^vga_modeinfo;
  VideoMem		   : ^VideoMemType;
  p : pointer;


Procedure DrawRectangle(x1,y1,x2,y2 : integer);
begin
  vga_drawline(x1,y1,x2,y1);
  vga_drawline(x1,y2,x2,y2);
  vga_drawline(x1,y1,x1,y2);
  vga_drawline(x2,y1,x2,y2);
end;  {DrawRectangle}


Procedure Scroll;
{ copies first bank of screen to the last bank (for 1024x768x256 mode) }
var  i : word;
begin
  vga_setreadpage(0);
  vga_setwritepage(11);
  for i := 65535 downto 0 do VideoMem^[i] := VideoMem^[i];
end;  {Scroll}
(*  
Procedure Scroll;  Assembler;
{ copies first bank of screen to the last bank (for 1024x768x256 mode) }
var  popreturn : longint;
asm
       mov   esi,SegA000
       mov   edi,esi
       mov   eax,0
       push  eax
       call  vga_setreadpage  {set read bank}
       pop   popreturn        {tidy stack}
       mov   eax,11
       push  eax
       call  vga_setwritepage {set write bank}
       pop   popreturn        {tidy stack}
       mov   ecx,16384
       rep   movsd            {copy all of bank 0 to bank 11}
end;  {Scroll}
*)

begin
  mode := 12;  {1024x768x256}
  i := vga_init;
  StartupMode := vga_getcurrentmode;
  vga_setmode(mode);
  gl_setcontextvga(mode);
  p := vga_getgraphmem; 
  VideoMem := p;
  modeinfo := vga_getmodeinfo(mode);
  SeparateReadWriteWindows := (modeinfo^.flags and HAVE_RWPAGE <> 0);
  if SeparateReadWriteWindows <> False then begin
     vga_setcolor(14);
     gl_fillbox(100,0,823,63,13);

     DrawRectangle(100,0,923,63);
     vga_setcolor(3);
     vga_drawline(0,0,1023,767);
     vga_drawline(1023,0,0,767);
     for i := 0 to 5000000 do vga_drawpixel(10,10);  {delay}

     Scroll;  {copy first 64 lines down to last 64 lines of screen}

     for i := 0 to 5000000 do vga_drawpixel(10,10);  {delay}

  end;
   
  vga_setmode(StartupMode);

  writeln('SeparateReadWriteWindows = ',SeparateReadWriteWindows);
  writeln('vgamode = ',mode);
end.
