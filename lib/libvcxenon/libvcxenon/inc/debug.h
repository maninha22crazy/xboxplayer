/*****************************************************************************\
*              efs - General purpose Embedded Filesystem library              *
*          ---------------------------------------------------------          *
*                                                                             *
* Filename : debug.h                                                          *
* Description : Headerfile for debug.c                                        *
*                                                                             *
* This program is free software; you can redistribute it and/or               *
* modify it under the terms of the GNU General Public License                 *
* as published by the Free Software Foundation; version 2                     *
* of the License.                                                             *
                                                                              *
* This program is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
* GNU General Public License for more details.                                *
*                                                                             *
* As a special exception, if other files instantiate templates or             *
* use macros or inline functions from this file, or you compile this          *
* file and link it with other works to produce a work based on this file,     *
* this file does not by itself cause the resulting work to be covered         *
* by the GNU General Public License. However the source code for this         *
* file must still be made available in accordance with section (3) of         *
* the GNU General Public License.                                             *
*                                                                             *
* This exception does not invalidate any other reasons why a work based       *
* on this file might be covered by the GNU General Public License.            *
*                                                                             *
*                                                    (c)2006 Lennart Yseboodt *
*                                                    (c)2006 Michael De Nil   *
\*****************************************************************************/

/* Contributions
 *                               LPC2000 ARM7 Interface (c)2005 Martin Thomas *
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

/*****************************************************************************/
#include "types.h"
#include "config.h"

//#include "shell.h"

/*****************************************************************************/

#ifndef DEBUG
	#define DEBUG  
#endif

#ifndef DEBUG
	#define TXT(x)      ;
	#define DBG         ;
	#define FUNC_IN(x)  ;
	#define FUNC_OUT(x) ;
#else
    #define TXT(x)      x
	#define DBG         Shell_OutTXT
	#define FUNC_IN(x)  ;
	#define FUNC_OUT(x) ;
	#define debug       Shell_OutTXT
#endif

#endif
