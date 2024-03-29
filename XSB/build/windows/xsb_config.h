/* windows/xsb_config.h.  Generated from def_config.in by configure.  */
/*  @configure_input@
**
**   This file contains definitions for Windows
**
**   Some variable may have to be changed manually.
*/

/* File:      def_config.in
** Author(s): kifer
** Contact:   xsb-contact@cs.sunysb.edu
**
** Copyright (C) The Research Foundation of SUNY, 1998-2011
**
** XSB is free software; you can redistribute it and/or modify it under the
** terms of the GNU Library General Public License as published by the Free
** Software Foundation; either version 2 of the License, or (at your option)
** any later version.
**
** XSB is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License for
** more details.
**
** You should have received a copy of the GNU Library General Public License
** along with XSB; if not, write to the Free Software Foundation,
** Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
**
*/



/******* VARIABLES THAT MIGHT NEED TO BE SET MANUALLY ****************/


#define HAVE_SOCKET 1

/* Define as __inline__ if that's what the C compiler calls it.  */
#define inline __inline


/******* END Variables to be set manually ****************/

#include "xsb_config_aux.h"

/**** DO NOT EDIT BELOW THIS LINE!!! *********************/


#define WIN_NT 1
#define RELEASE_DATE 2013-05-01
#define XSB_VERSION "3.4.0 (Soy mILK)"

/* this is used by many to check if config.h was included in proper order */
#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED 1
#endif

/* Single precision floats */
/* #undef FAST_FLOATS */

/* Use local eval strategy. This is the default. */
#define LOCAL_EVAL 1

/* Define, if XSB is built with support for ORACLE DB */
/* #undef ORACLE */
/* #undef ORACLE_DEBUG */

#define FOREIGN_WIN32
#define FOREIGN

#define bcopy(A,B,L) memmove(B,A,L)

#define SLASH '\\'

#define GC 1

#define MULTI_THREAD 1

#define SHARED_COMPL_TABLES 1

/* #undef CONC_COMPL */

/* Define if you have the gethostbyname function.  */
#define HAVE_GETHOSTBYNAME 1


/* Define if you have the mkdir function.  */
#define HAVE_MKDIR 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* GC on SLG-WAM! ;) */
#define SLG_GC 1

/* #undef NON_OPT_COMPILE */
