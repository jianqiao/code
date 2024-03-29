/* File:      prolog_includes/char_defs.h
** Author(s): kifer
** Contact:   xsb-contact@cs.sunysb.edu
** 
** Copyright (C) The Research Foundation of SUNY, 1999
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
** $Id: char_defs.h,v 1.5 2010/08/19 15:03:40 spyrosh Exp $
** 
*/

#define CH_NULL	     	     0

#define CH_ALARM	     7	      /*  \a     */
#define CH_BACKSPACE         8	      /*  \b	 */
#define CH_TAB               9	      /*  \t     */
#define CH_NEWLINE           10       /*  \n     */
#define CH_VERTAB            11	      /*  \v     */
#define CH_FORMFEED          12       /*  \f     */
#define CH_RETURN            13       /*  \r     */

/* These 4 names are from ASCII(7) */
#define CH_DC1               17       /*         */
#define CH_DC2               18       /*         */
#define CH_DC3               19       /*         */
#define CH_DC4               20       /*         */

#define CH_ESC               27	      /*  ^[	 */

#define CH_SPACE             32	      /*  ' '	*/
#define CH_EXCLAMATION       33	      /*  !     */
#define CH_DOUBLEQUOTE       34	      /*  "     */
#define CH_HASH 	     35	      /*  #     */
#define CH_DOLLAR 	     36	      /*  $     */
#define CH_PERCENT	     37	      /*  %     */
#define CH_AMPERSAND         38	      /*  &     */
#define CH_QUOTE             39	      /*  '     */
#define CH_LPAREN            40	      /*  (     */
#define CH_RPAREN            41	      /*  )     */
#define CH_STAR	             42	      /*  *     */
#define CH_PLUS	     	     43	      /*  +     */
#define CH_COMMA	     44	      /*  ,     */
#define CH_MINUS	     45	      /*  -     */
#define CH_DOT	     	     46	      /*  .     */
#define CH_SLASH 	     47	      /*  /     */
#define CH_0	     	     48	      /*  0     */
#define CH_1	     	     49	      /*  1     */
#define CH_2	     	     50	      /*  2     */
#define CH_3	     	     51	      /*  3     */
#define CH_4	     	     52	      /*  4     */
#define CH_5	     	     53	      /*  5     */
#define CH_6	     	     54	      /*  6     */
#define CH_7	     	     55	      /*  7     */
#define CH_8	     	     56	      /*  8     */
#define CH_9	     	     57	      /*  9     */
#define CH_COLON     	     58	      /*  :     */
#define CH_SEMICOL     	     59	      /*  ;     */
#define CH_LESS     	     60	      /*  <     */
#define CH_EQUAL     	     61	      /*  =     */
#define CH_GREATER     	     62	      /*  >     */
#define CH_QUESTION    	     63	      /*  ?     */
#define CH_AT	     	     64	      /*  @     */
#define CH_A	     	     65	      /*  A     */
#define CH_B	     	     66	      /*  B     */
#define CH_C	     	     67	      /*  C     */
#define CH_D	     	     68	      /*  D     */
#define CH_E	     	     69	      /*  E     */
#define CH_F	     	     70	      /*  F     */
#define CH_G	     	     71	      /*  G     */
#define CH_H	     	     72	      /*  H     */
#define CH_I	     	     73	      /*  I     */
#define CH_J	     	     74	      /*  J     */
#define CH_K	     	     75	      /*  K     */
#define CH_L	     	     76	      /*  L     */
#define CH_M	     	     77	      /*  M     */
#define CH_N	     	     78	      /*  N     */
#define CH_O	     	     79	      /*  O     */
#define CH_P	     	     80	      /*  P     */
#define CH_Q	     	     81	      /*  Q     */
#define CH_R	     	     82	      /*  R     */
#define CH_S	     	     83	      /*  S     */
#define CH_T	     	     84	      /*  T     */
#define CH_U	     	     85	      /*  U     */
#define CH_V	     	     86	      /*  V     */
#define CH_W	     	     87	      /*  W     */
#define CH_X	     	     88	      /*  X     */
#define CH_Y	     	     89	      /*  Y     */
#define CH_Z	     	     90	      /*  Z     */
#define CH_LBRACKET 	     91	      /*  [     */
#define CH_BACKSLASH   	     92	      /*  \     */
#define CH_RBRACKET    	     93	      /*  ]     */
#define CH_HAT	      	     94	      /*  ^     */
#define CH_UNDERSCORE  	     95	      /*  _     */
#define CH_BACKQUOTE   	     96	      /*  `     */
#define CH_a    	     97	      /*  a     */
#define CH_b    	     98	      /*  b     */
#define CH_c    	     99	      /*  c     */
#define CH_d    	     100      /*  d     */
#define CH_e    	     101      /*  e     */
#define CH_f    	     102      /*  f     */
#define CH_g    	     103      /*  g     */
#define CH_h    	     104      /*  h     */
#define CH_i    	     105      /*  i     */
#define CH_j    	     106      /*  j     */
#define CH_k    	     107      /*  k     */
#define CH_l    	     108      /*  l     */
#define CH_m    	     109      /*  m     */
#define CH_n    	     110      /*  n     */
#define CH_o    	     111      /*  o     */
#define CH_p    	     112      /*  p     */
#define CH_q    	     113      /*  q     */
#define CH_r    	     114      /*  r     */
#define CH_s    	     115      /*  s     */
#define CH_t    	     116      /*  t     */
#define CH_u    	     117      /*  u     */
#define CH_v    	     118      /*  v     */
#define CH_w    	     119      /*  w     */
#define CH_x    	     120      /*  x     */
#define CH_y    	     121      /*  y     */
#define CH_z    	     122      /*  z     */
#define CH_LBRACE    	     123      /*  {     */
#define CH_BAR       	     124      /*  |     */
#define CH_RBRACE    	     125      /*  }     */
#define CH_TILDE     	     126      /*  ~     */

#define CH_DELETE      	     127      /*  ^?	*/

#define CH_EOF_C	     255      /* The C EOF character: (char) -1  */
#define CH_EOF_P	     -1       /* The Prolog EOF character: -1    */
