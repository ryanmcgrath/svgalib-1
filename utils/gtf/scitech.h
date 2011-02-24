/****************************************************************************
*
*					SciTech Multi-platform Graphics Library
*
*  ========================================================================
*
*    The contents of this file are subject to the SciTech MGL Public
*    License Version 1.0 (the "License"); you may not use this file
*    except in compliance with the License. You may obtain a copy of
*    the License at http://www.scitechsoft.com/mgl-license.txt
*
*    Software distributed under the License is distributed on an
*    "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
*    implied. See the License for the specific language governing
*    rights and limitations under the License.
*
*    The Original Code is Copyright (C) 1991-1998 SciTech Software, Inc.
*
*    The Initial Developer of the Original Code is SciTech Software, Inc.
*    All Rights Reserved.
*
*  ========================================================================
*
* Language:     ANSI C
* Environment:  any
*
* Description:  General header file for operating system portable code.
*
****************************************************************************/

#ifndef __SCITECH_H
#define __SCITECH_H

/* We have the following defines to identify the compilation environment:
 *
 *	__16BIT__		Compiling for 16 bit code (any environment)
 *  __32BIT__       Compiling for 32 bit code (any environment)
 *	__MSDOS__		Compiling for MS-DOS (includes __WINDOWS16__, __WIN386__)
 *  __REALDOS__     Compiling for MS-DOS (excludes __WINDOWS16__)
 *  __MSDOS16__ 	Compiling for 16 bit MS-DOS
 *  __MSDOS32__ 	Compiling for 32 bit MS-DOS
 *  __WINDOWS__ 	Compiling for Windows
 *	__WINDOWS16__	Compiling for 16 bit Windows (__MSDOS__ also defined)
 *	__WINDOWS32__	Compiling for 32 bit Windows
 *	__WIN32_VXD__	Compiling for a 32-bit C based VxD
 *  __OS2__     	Compiling for OS/2
 *  __OS2_16__  	Compiling for 16 bit OS/2
 *  __OS2_32__ 		Compiling for 32 bit OS/2
 *  __UNIX__   		Compiling for Unix
 *  __QNX__         Compiling for the QNX realtime OS (Unix compatible)
 *  __LINUX__		Compiling for the Linux OS (Unix compatible)
 *  __FREEBSD__		Compiling for the FreeBSD OS (Unix compatible)
 *	__BEOS__		Compiling for the BeOS (Unix compatible)
 *  __SMX32__       Compiling for the SMX 32-bit Real Time OS
 *  __DRIVER__      Compiling for a 32-bit binary compatible driver
 *
 *  __INTEL__       Compiling for Intel CPU's
 *  __ALPHA__       Compiling for DEC Alpha CPU's
 *  __MIPS__        Compiling for MIPS CPU's
 *  __PPC__         Compiling for PowerPC CPU's
 *  __MC68K__		Compiling for Motorola 680x0
 *
 */

#ifdef	__SC__
#if		__INTSIZE == 4
#define __SC386__
#endif
#endif

#ifdef	__GNUC__
#if !defined(__BEOS__)
#define	__cdecl			/* GCC doesn't know about __cdecl modifiers		*/
#endif
#define	__FLAT__		/* GCC is always 32 bit flat model				*/
#define	__HAS_BOOL__	/* Latest GNU C++ has ibool type					*/
#include <stdio.h>		/* Bring in for definition of NULL				*/
#endif

#ifdef	__BORLANDC__
#if	(__BORLANDC__ >= 0x500) || defined(CLASSLIB_DEFS_H)
#define	__HAS_BOOL__	/* Borland C++ 5.0 and later define ibool type	*/
#endif
#endif

/*---------------------------------------------------------------------------
 * Determine the compile time environment. This must be done for each
 * supported platform so that we can determine at compile time the target
 * environment, hopefully without requiring #define's from the user.
 *-------------------------------------------------------------------------*/

/* 32-bit binary compatible driver. Compiled as Win32, but as OS neutral */
#ifdef	__DRIVER__
#ifndef	__32BIT__
#define	__32BIT__
#endif
#undef	__WINDOWS__
#undef	_WIN32
#undef 	__WIN32__
#undef	__NT__

/* 32-bit Windows VxD compile environment */
#elif	defined(__vtoolsd_h_) || defined(VTOOLSD)
#include <vtoolsc.h>
#define	__WIN32_VXD__
#ifndef	__32BIT__
#define __32BIT__
#endif
#define	_MAX_PATH	256
#ifndef	__WINDOWS32__
#define	__WINDOWS32__
#endif

/* 32-bit SMX compile environment */
#elif   defined(__SMX32__)
#ifndef __MSDOS__
#define __MSDOS__
#endif
#ifndef	__32BIT__
#define __32BIT__
#endif
#ifndef __REALDOS__
#define __REALDOS__
#endif

/* 32-bit extended DOS compile environment */
#elif	defined(__MSDOS__) || defined(__MSDOS32__) || defined(__DOS__) || defined(__DPMI32__) || (defined(M_I86) && !defined(__SC386__)) || defined(TNT)
#ifndef __MSDOS__
#define __MSDOS__
#endif
#if     defined(__MSDOS32__) || defined(__386__) || defined(__FLAT__) || defined(__NT__) || defined(__SC386__)
#ifndef	__MSDOS32__
#define __MSDOS32__
#endif
#ifndef	__32BIT__
#define __32BIT__
#endif
#ifndef __REALDOS__
#define __REALDOS__
#endif

/* 16-bit Windows compile environment */
#elif   (defined(_Windows) || defined(_WINDOWS)) && !defined(__DPMI16__)
#ifndef	__16BIT__
#define	__16BIT__
#endif
#ifndef __WINDOWS16__
#define	__WINDOWS16__
#endif
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#ifndef	__MSDOS__
#define	__MSDOS__
#endif

/* 16-bit DOS compile environment */
#else
#ifndef	__16BIT__
#define	__16BIT__
#endif
#ifndef __MSDOS16__
#define __MSDOS16__
#endif
#ifndef __REALDOS__
#define __REALDOS__
#endif
#endif

/* 32-bit Windows compile environment */
#elif   defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#ifndef	__32BIT__
#define __32BIT__
#endif
#ifndef	__WINDOWS32__
#define	__WINDOWS32__
#endif
#ifndef	_WIN32
#define	_WIN32					/* Microsoft Win32 SDK headers use _WIN32 */
#endif
#ifndef	WIN32
#define	WIN32					/* OpenGL headers use WIN32 */
#endif
#ifndef __WINDOWS__
#define __WINDOWS__
#endif

/* 16-bit OS/2 compile environment */
#elif   defined(__OS2_16__)
#ifndef __OS2__
#define	__OS2__
#endif
#ifndef	__16BIT__
#define __16BIT__
#endif

/* 32-bit OS/2 compile environment */
#elif   defined(__OS2__) || defined(__OS2_32__)
#ifndef __OS2__
#define	__OS2__
#endif
#ifndef __OS2_32__
#define	__OS2_32__
#endif
#ifndef	__32BIT__
#define __32BIT__
#endif

/* 32-bit QNX compile environment */
#elif	defined(__QNX__)
#ifndef	__32BIT__
#define __32BIT__
#endif
#ifndef	__UNIX__
#define __UNIX__
#endif

/* 32-bit Linux compile environment */
#elif	defined(__LINUX__) || defined(linux)
#ifndef	__LINUX__
#define	__LINUX__
#endif
#ifndef	__32BIT__
#define __32BIT__
#endif
#ifndef	__UNIX__
#define __UNIX__
#endif

/* 32-bit FreeBSD compile environment */
#elif	defined(__FREEBSD__)
#ifndef	__32BIT__
#define __32BIT__
#endif
#ifndef	__UNIX__
#define __UNIX__
#endif

/* 32-bit BeOS compile environment */
#elif	defined(__BEOS__)
#ifndef	__32BIT__
#define	__32BIT__
#endif
#ifndef	__UNIX__
#define __UNIX__
#endif
#else
#error	This platform is not currently supported!
#endif

/* Determine the CPU type that we are compiling for */

#if		defined(__M_ALPHA) || defined(__ALPHA_) || defined(__ALPHA) || defined(__alpha)
#ifndef	__ALPHA__
#define	__ALPHA__
#endif
#elif	defined(__M_PPC) || defined(__POWERC)
#ifndef	__PPC__
#define	__PPC__
#endif
#elif	defined(__M_MRX000)
#ifndef	__MIPS__
#define	__MIPS__
#endif
#else
#ifndef	__INTEL__
#define	__INTEL__				/* Assume Intel if nothing found */
#endif
#endif

/* We have the following defines to define the calling conventions for
 * publicly accesible functions:
 *
 *  _PUBAPI - Compiler default calling conventions for all public 'C' functions
 *  _ASMAPI - Calling conventions for all public assembler functions
 *  _DLLAPI - Calling conventions for all DLL exported functions
 *  _DLLVAR - Modifier to export/import globals in 32 bit DLL's
 *  _EXPORT - Expands to _export when compiling a DLL
 *  _VARAPI - Modifiers for variables; Watcom C++ mangles C++ globals
 */

#if defined(_MSC_VER) && defined(_WIN32) && !defined(__SC__)
#define __PASCAL    __stdcall
#define __export
#define	__import
#else
#define __PASCAL    __pascal
#endif

#ifdef	__WATCOMC__
#if (__WATCOMC__ >= 1050)
#define	_VARAPI		__cdecl
#else
#define	_VARAPI
#endif
#else
#define	_VARAPI
#endif

#if	defined(__IBMC__) || defined(__IBMCPP__)
#define PTR_DECL_IN_FRONT
#endif

#if     defined(__WINDOWS__)
#ifdef  BUILD_DLL
#define _DLLASM __export __cdecl
#define _EXPORT __export
#ifdef  __WINDOWS32__
#define _DLLAPI __export __PASCAL
#define _DLLVAR __export
#else
#define _DLLAPI  __export __far __pascal
#define _DLLVAR
#endif
#else
#define _DLLASM __cdecl
#define	_EXPORT
#ifdef  __WINDOWS32__
#define _DLLAPI __PASCAL
#define _DLLVAR __import
#else
#define _DLLAPI __far __pascal
#define _DLLVAR
#endif
#endif
#else
#if !defined(__BEOS__)
#define _EXPORT
#endif
#define _DLLAPI
#define _DLLVAR
#endif

/* Define the calling conventions for all public functions. For simplicity
 * we define all public functions as __cdecl calling conventions, so that
 * they are the same across all compilers and runtime DLL's.
 */

#define _PUBAPI __cdecl
#define _ASMAPI __cdecl

/* Determine the syntax for declaring a function pointer with a
 * calling conventions override. Most compilers require the calling
 * convention to be declared in front of the '*', but others require
 * it to be declared after the '*'. We handle both in here depending
 * on what the compiler requires.
 */

#ifdef	PTR_DECL_IN_FRONT
#define	_DLLAPIP	* _DLLAPI
#define	_DLLASMP	* _DLLASM
#define	_PUBAPIP	* _PUBAPI
#define	_ASMAPIP	* _ASMAPI
#else
#define	_DLLAPIP	_DLLAPI *
#define	_DLLASMP	_DLLASM *
#define	_PUBAPIP	_PUBAPI *
#define	_ASMAPIP	_ASMAPI *
#endif

/* Useful macros */

#define PRIVATE static
#define PUBLIC

/* This HAS to be 0L for 16-bit real mode code to work!!! */

#ifndef NULL
#       define NULL 0L
#endif

#ifndef MAX
#       define MAX(a,b) ( ((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#       define MIN(a,b) ( ((a) < (b)) ? (a) : (b))
#endif
#ifndef ABS
#       define ABS(a)   ((a) >= 0 ? (a) : -(a))
#endif
#ifndef	SIGN
#		define SIGN(a)	((a) > 0 ? 1 : -1)
#endif

/* General typedefs */

#ifndef __GENDEFS
#define __GENDEFS
#if defined(__BEOS__)
#include <SupportDefs.h>
#else
#ifdef __LINUX__
#include <sys/types.h>
#ifdef __STRICT_ANSI__
typedef unsigned short  ushort;
typedef unsigned long   ulong;
typedef unsigned int    uint;
#endif
#else
#if !(defined(__QNXNTO__) && defined(GENERAL_STRUCT))
typedef unsigned short  ushort;
typedef unsigned long   ulong;
#endif
typedef unsigned int    uint;
#endif
typedef unsigned char   uchar;
#endif
typedef int             ibool;		/* Integer boolean type			*/
#ifdef	USE_BOOL					/* Only for older code			*/
#ifndef	__cplusplus
#define	bool			ibool		/* Standard C					*/
#else
#ifndef	__HAS_BOOL__
#define	bool			ibool		/* Older C++ compilers			*/
#endif
#endif	/* __cplusplus */
#endif	/* USE_BOOL */
#endif  /* __GENDEFS */

/* Boolean truth values */

#undef	false
#undef	true
#undef	NO
#undef	YES
#undef	FALSE
#undef	TRUE
#define false       0
#define true        1
#define NO          0
#define YES         1
#define FALSE       0
#define TRUE        1

/* Inline debugger interrupts for Watcom C++ and Borland C++ */

#ifdef	__WATCOMC__
void DebugInt(void);
#pragma aux DebugInt =				\
	"int	3";
void DebugVxD(void);
#pragma aux DebugVxD =				\
	"int	1";
#elif	defined(__BORLANDC__)
#define	DebugInt()	__emit__(0xCC)
#define	DebugVxD()	{__emit__(0xCD); __emit__(0x01);}
#elif	defined(_MSC_VER)
#define	DebugInt()	_asm int 0x3
#define	DebugVxD()	_asm int 0x1
#else
#define	DebugInt()
#define	DebugVxD()
#endif

/*---------------------------------------------------------------------------
 * Set of debugging macros used by the libraries. If the debug flag is
 * set, they are turned on depending on the setting of the flag. User code
 * can override the default functions called when a check fails, and the
 * MGL does this so it can restore the system from graphics mode to display
 * an error message. These functions also log information to the
 * scitech.log file in the root directory of the hard drive when problems
 * show up.
 *
 * If you set the value of CHECKED to be 2, it will also enable code to
 * insert hard coded debugger interrupt into the source code at the line of
 * code where the check fail. This is useful if you run the code under a
 * debugger as it will break inside the debugger before exiting with a
 * failure condition.
 *
 * Also for code compiled to run under Windows, we also call the
 * OutputDebugString function to send the message to the system debugger
 * such as Soft-ICE or WDEB386. Hence if you get any non-fatal warnings you
 * will see those on the debugger terminal as well as in the log file.
 *-------------------------------------------------------------------------*/

extern void (*_CHK_fail)(int fatal,const char *msg,const char *cond,const char *file,int line);
void _CHK_defaultFail(int fatal,const char *msg,const char *cond,const char *file,int line);

#ifdef	CHECKED
#       define  CHK(x)  	x
#if		CHECKED > 1
#		define	CHECK(p)											\
		((p) ? (void)0 : DebugInt(),                                \
			_CHK_fail(1,"Check failed: '%s', file %s, line %d\n",   \
			#p, __FILE__, __LINE__))
#		define	WARN(p)												\
		((p) ? (void)0 : DebugInt(),                  				\
			_CHK_fail(0,"Warning: '%s', file %s, line %d\n",   		\
			#p, __FILE__, __LINE__))
#else
#		define	CHECK(p)											\
		((p) ? (void)0 :                                 			\
			_CHK_fail(1,"Check failed: '%s', file %s, line %d\n",   \
			#p, __FILE__, __LINE__))
#		define	WARN(p)												\
		((p) ? (void)0 : 											\
			_CHK_fail(0,"Warning: '%s', file %s, line %d\n",   		\
			#p, __FILE__, __LINE__))
#endif
#else
#       define  CHK(x)
#		define	CHECK(p)	((void)0)
#		define	WARN(p)		((void)0)
#endif

#endif  /* __SCITECH_H */
