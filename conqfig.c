#include "c_defs.h"

/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 06:36:09  jon
 * Initial revision
 *
 * Revision 1.7  1996/07/02  19:53:44  jon
 * - code cleanup
 *
 * Revision 1.6  1996/05/25  00:38:12  jon
 * - removed c_conq_gods var - this is handled by new isagod() rewrite
 * - changed absolute path references to relative.  (to CONQHOME)
 * - changed c_conq_newsfile to point toward new conqnews.doc
 *   file instead of ChangeLog
 *
 * Revision 1.5  1996/05/02  01:11:27  jon
 * - placed values in c_conq_newsfile and c_conq_helpfile.  Yes,
 *   there is actually documentation now ;-)
 *
 * Revision 1.4  1996/03/16  21:54:08  jon
 * added root and davidp to godlist.  eventually this stuff
 * should be put in an external config file so recompiles aren't
 * neccessary for  these types of changes.
 *
 * Revision 1.3  1995/01/14  22:53:29  jon
 * *** empty log message ***
 *
 * Revision 1.2  1995/01/02  00:58:58  jon
 * Changed references to NUMPLANETS and MAXSHIPS so that valid
 * values would be between 1 and NUMPLANETS/MAXSHIPS.
 *
 * Revision 1.2  1995/01/02  00:58:58  jon
 * Changed references to NUMPLANETS and MAXSHIPS so that valid
 * values would be between 1 and NUMPLANETS/MAXSHIPS.
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 *
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


