#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

/*;;; conqfig - Conquest configuration
;
;            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres
;
;    Permission to use, copy, modify, and distribute this software and
;    its documentation for any purpose and without fee is hereby granted,
;    provided that this copyright notice appear in all copies and in all
;    supporting documentation. Jef Poskanzer and Craig Leres make no
;    representations about the suitability of this software for any
;    purpose. It is provided "as is" without express or implied warranty.
;
;	The Vax-11 Fortran "external" statement and "%loc()" construct
;	are used to allow access to these global defines and strings.
;
; Useful local defines
;
	no = 0
	yes = 1
;
;	REQUIRED CONFIGURATION SECTION: The following must be modified.
;
; Conqdriv: A VMS file specification which defines the location of
; the Conquest driver. This file MUST be defined.
;
*/
const char c_conq_conqdriv[] =	"bin/conqdriv";

const char *c_conq_commonblk = "etc/conquest_common.img";
const char *c_conq_errlog = "etc/conquest.log";

/*;
; Newsfile: A VMS file specification which defines the location of
; the Conquest news file. If there is no news file, define as "".
;
*/
const char *c_conq_newsfile = "etc/conqnews.doc";
/*;
; Helpfile: A VMS file specification which defines the location of
; the Conquest help file. If there is no help file, define as "".
;
*/
const char *c_conq_helpfile = "etc/conquest.doc";
/*;
; Gamcron: A VMS file specification which defines the location of the
; gamcron file. The gamcron file is used to decide when it's allowed
; to play Conquest. If it is desired to disable this feature, define as "".
;
*/
const char *c_conq_gamcron = "";
/*;
;	OPTIONAL CONFIGURATION SECTION: The following default to useful values.
;
; Conquest: A simple file name which specifies the true name of the
; conquest binary. If the conquest binary is run with a simple name
; that is different from the name defined here, it will be deleted.
; This prevents people from making copies of the conquest binary
; and calling them things like "edt.exe". To disable this feature,
; define as "".
;
*/
const char *c_conq_conquest = "conquest";
/*;
; Antigods: A list of usernames (separated by colons) never allowed to play
; Conquest. This string may be defined as "".
;
*/
const char *c_conq_antigods = "";
/*;
; Badttys: A list of terminal names (separated by colons) from which Conquest
; may not be played. For example, "ttb1" disallows only ttb1 while "ttb"
; disallows all ttb ports. This string may be defined as "".
;
*/
const char *c_conq_badttys = "";
/*;
; Fdial: Controls dialup use. If dialup access is to be permitted,
; define to yes. Otherwise, define to no.
;
*/
const int c_conq_fdial = yes;
/*;
; Fsubdcl: Controls spawning to DCL. If spawning to DCL is desired, define
; to yes. Otherwise, define to no.
;
*/
const int c_conq_fsubdcl = no;
/*;
; Fprio and despri: If it is desired that Conquest change its priority when
; executing, define c_conq_fprio to yes. If Conquest should run at the users
; default priority, define to no. If c_conq_fprio is defined to yes, then
; a positive value of c_conq_despri specifies an absolute priority and a
; negative value specifies how much to subtract from the current priority.
;
*/
const int c_conq_fprio = no;
const int c_conq_despri = 0;


