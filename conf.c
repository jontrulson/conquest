#include "c_defs.h"

/************************************************************************
 *
 * conf.c - handle configuration issues, both personal and system wide.
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include "global.h"
#include "conf.h"

		/* For id purposes... */
static char *confId = "$Id$";


/* GetSysConf(int checkonly) - load system-wide configuration values */
/*  If checkonly is TRUE, then just load internel defaults, and the existing
    conquesrtc file, if there.  Don't output errmsgs since this will
    only be TRUE when 'priming the pump' in conqoper -C, before writing a
    new conquestrc file (preserving existing settings if possible). 
*/
int GetSysConf(int checkonly)
{
  FILE *conf_fd;
  int i, j;
  char conf_name[256];
  char buffer[BUFFER_SIZE];
  int FoundOne = FALSE;
  int buflen;
  char *bufptr;

				/* init some defaults */
  sysconf_DoLRTorpScan = TRUE;
  sysconf_DoLocalLRScan = TRUE;
  sysconf_DoETAStats = TRUE;
  sysconf_AllowFastUpdate = TRUE;
  sysconf_NoDoomsday = FALSE;
  sysconf_DoRandomRobotKills = FALSE;
  sysconf_AllowSigquit = FALSE;

				/* start building the filename */
  sprintf(conf_name, "%s/%s", CONQHOME, SYSCONFIG_FILE);

  if ((conf_fd = fopen(conf_name, "r")) == NULL)
    {
      int err;

      err = errno;

      clog("GetSysConf(): fopen(%s) failed: %s",
	       conf_name,
	       sys_errlist[err]);

      if (checkonly != TRUE)
	{
	  fprintf(stderr, "Error opening system-wide config file: %s: %s\n",
		  conf_name,
		  sys_errlist[err]);
	  
	  fprintf(stderr, "'conqoper -C' needs to be run. Using internal defaults.\n");
	  sleep(4);
	}

      return(ERR);
    }

#ifdef DEBUG_CONFIG
      clog("GetSysConf(): Opened '%s'", conf_name);
#endif

				/* Do it dude... */
  while (fgets(buffer, BUFFER_SIZE - 1, conf_fd) != NULL)
    {				/* get em one at a time */
      buflen = strlen(buffer);

      if (buffer[0] == '#')
	continue;

      if (buffer[0] == '\n')
	continue;

      if (buflen == 0)
	continue;		/* shouldn't happen...but the universe is
				   mysterious... */

      buffer[buflen - 1] = EOS;	/* remove trailing LF */
      buflen--;

	  
#ifdef DEBUG_CONFIG
      clog("GetSysConf(): got '%s'", buffer);
#endif

				/* check for everything else */

      FoundOne = FALSE;

      for (j = 0; (j < SYSCF_END) && (FoundOne == FALSE); j++)
	{
	  if (SysConfData[j].ConfName != NULL)
	    if (strncmp(SysConfData[j].ConfName, 
			buffer, 
			strlen(SysConfData[j].ConfName)) == 0)
	      {			/* we found a valid variable */
		if (buflen > 0)
		  {		/* split off the value */
		    bufptr = &buffer[strlen(SysConfData[j].ConfName)];
		    
		    switch(SysConfData[j].ConfType)
		      {
		      case CTYPE_NULL:
			/* special check for version */

			if (j == SYSCF_VERSION)
			  {
			    if(strncmp((char *)SysConfData[j].ConfValue, 
				       bufptr, 
				       strlen((char *)SysConfData[j].ConfValue)) == 0)
			      {
				SysConfData[j].Found = TRUE;
#ifdef DEBUG_CONFIG
				clog("GetSysConf(): got correct version = '%s'", buffer);
#endif
			      }
			  }

			break;
		      
		      case CTYPE_BOOL:
			i = process_bool(bufptr);
			if (i != ERR)
			  {
			    *((int *) SysConfData[j].ConfValue) = i;
			    SysConfData[j].Found = TRUE;
			    FoundOne = TRUE;
			  }
			break;
			
		      case CTYPE_MACRO:
			strcpy((char *)SysConfData[j].ConfValue, 
			       process_macrostr(bufptr));
			SysConfData[j].Found = TRUE;
			FoundOne = TRUE;
			break;
		      
		      
		      } /* switch */
		  } /* if */
	      } /* if */
	} /* for */
    } /* while */
  

  if (!feof(conf_fd))
    {
      fprintf(stderr, "GetSysConf(): Error reading %s: %s\n",
	      conf_name,
	      sys_errlist[errno]);
      fclose(conf_fd);
      
      return(ERR);
    }

  fclose(conf_fd);

				/* Now we check them all to see if one  */
				/* of the options wasn't read in. If one */
				/* is found, notify user that operator
				   needs to run conqoper -C . */

				/* see if we found the version */
  if (SysConfData[SYSCF_VERSION].Found == FALSE)
    {				/* conquestrc version not found */
#ifdef DEBUG_CONFIG
      clog("GetSysConf(): Incorrect version found.  Update needed.");
#endif
      
      if (checkonly != TRUE)
	{
	  fprintf(stderr, "The %s file needs to be updated by an operator with\n",
		  conf_name);
	  fprintf(stderr, "  'conqoper -C'\n");
	  sleep(4);
	}
    }
  else
    {				/* version found. check everything else */
      for (i=0; i<SYSCF_END; i++)
	{
	  if (SysConfData[i].ConfType != CTYPE_NULL)
	    if (SysConfData[i].Found != TRUE)
	      {
#ifdef DEBUG_CONFIG
		clog("GetSysConf(): option '%s' not found - Update neededy.",
		   SysConfData[i].ConfName);
#endif
		if (checkonly != TRUE)
		  {
		    fprintf(stderr, "The %s file needs to be updated by an operator with\n",
			    conf_name);
		    fprintf(stderr, "  'conqoper -C'\n");
		    sleep(4);
		  }

		break;		
	      }
	}
    }




  return(TRUE);
}

				/* get user's configuration */
int GetConf(int isremote, int usernum)
{
  FILE *conf_fd;
  int i, j;
  char conf_name[MID_BUFFER_SIZE];
  char *homevar;
  char buffer[BUFFER_SIZE];
  int buflen;
  char *bufptr;
  int FoundOne = FALSE;

				/* init some defaults */
  conf_MessageBell = TRUE;
  conf_NoColor = FALSE;
  conf_NoRobotMsgs = FALSE;
  conf_RecPlayerMsgs = TRUE;
  conf_DoFastUpdate = TRUE;
  conf_DoLimitBell = TRUE;
  conf_ClearOldMsgs = TRUE;

  for (i=0; i<MAX_MACROS; i++)
    conf_MacrosF[i][0] = EOS;

				/* start building the filename */
  if ((homevar = getenv("HOME")) == NULL)
    {
      clog("GetConf(): getenv(HOME) failed");

      fprintf(stderr, "Can't get HOME environment variable. Exiting\n");
      return(ERR);
    }

  if (isremote == TRUE && usernum > 0)
    {				/* build the remote user version */
      umask(007);
      sprintf(conf_name, "%s/%s.%d", homevar, CONFIG_FILE, usernum);
    }
  else
      sprintf(conf_name, "%s/%s", homevar, CONFIG_FILE);


  if ((conf_fd = fopen(conf_name, "r")) == NULL)
    {
      if (errno != ENOENT)
	{
	  clog("GetConf(): fopen(%s) failed: %s",
	       conf_name,
	       sys_errlist[errno]);
	  
	  fprintf(stderr, "Error opening config file: %s: %s\n",
		  conf_name,
		  sys_errlist[errno]);

	  return(ERR);
	}

#ifdef DEBUG_CONFIG
      clog("GetConf(): No config file.");
#endif
      
      if (MakeConf(conf_name) == ERR)
	return(ERR);

      return(FALSE);		/* no config file */
    }

#ifdef DEBUG_CONFIG
      clog("GetConf(): Opened '%s'", conf_name);
#endif


				/* We got it! Now lets process it. */

  while (fgets(buffer, BUFFER_SIZE - 1, conf_fd) != NULL)
    {				/* get em one at a time */
      buflen = strlen(buffer);

      if (buffer[0] == '#')
	continue;

      if (buffer[0] == '\n')
	continue;

      if (buflen == 0)
	continue;		/* shouldn't happen...but the universe is
				   mysterious... */

      buffer[buflen - 1] = EOS;	/* remove trailing LF */
      buflen--;

#ifdef DEBUG_CONFIG
      clog("GetConf(): got '%s'", buffer);
#endif

      FoundOne = FALSE;

      for (j = 0; (j < CF_END) && (FoundOne == FALSE); j++)
	{
	  if (ConfData[j].ConfName != NULL)
	    if(strncmp(ConfData[j].ConfName, 
		       buffer, 
		       strlen(ConfData[j].ConfName)) == 0)
	      {			/* we found a valid variable */
		if (buflen > 0)
		  {		/* split off the value */
		    bufptr = &buffer[strlen(ConfData[j].ConfName)];
		  
		    switch(ConfData[j].ConfType)
		      {
		      case CTYPE_NULL:
			/* special check for version */
			if (j == CF_VERSION)
			  {
			    if(strncmp((char *)ConfData[j].ConfValue, 
				       bufptr, 
				       strlen((char *)ConfData[j].ConfValue)) == 0)
			      {
				ConfData[j].Found = TRUE;
				FoundOne = TRUE;
#ifdef DEBUG_CONFIG
				clog("GetConf(): got correct version = '%s'", buffer);
#endif
			      }
			  }
			break;
			
		      case CTYPE_BOOL:
			i = process_bool(bufptr);
			if (i != ERR)
			  {
			    *((int *) ConfData[j].ConfValue) = i;
			    ConfData[j].Found = TRUE;
			    FoundOne = TRUE;
			  }
			break;
			
		      case CTYPE_MACRO:
			strcpy((char *)ConfData[j].ConfValue, process_macrostr(bufptr));
			ConfData[j].Found = TRUE;
			FoundOne = TRUE;
			break;
			
			
		      } /* switch */
		  } /* if */
	    } /* if */
	} /* for */
    } /* while */
  

  if (!feof(conf_fd))
    {
      fprintf(stderr, "GetConf(): Error reading %s: %s\n",
	      conf_name,
	      sys_errlist[errno]);
      fclose(conf_fd);
      
      return(ERR);
    }

  fclose(conf_fd);

				/* Now we check them all to see if one  */
				/* of the options wasn't read in. If one */
				/* is found, re-write the config file. */

				/* see if we found the version */
  if (ConfData[CF_VERSION].Found == FALSE)
    {				/* .conquestrc version not found */
#ifdef DEBUG_CONFIG
      clog("GetConf(): Incorrect version found. - rebuilding");
#endif
      MakeConf(conf_name);	/* rebuild */
    }
  else
    {				/* version found. check everything else */
      for (i=0; i<CF_END; i++)
	{
	  if (ConfData[i].ConfType != CTYPE_NULL)
	    if (ConfData[i].Found != TRUE)
	      {
#ifdef DEBUG_CONFIG
		clog("GetConf(): option '%s' not found - rebuilding.",
		   ConfData[i].ConfName);
#endif
		MakeConf(conf_name);	/* rebuild - one not found */
		break;		/* no need to cont if we rebuilt */
	      }
	}
    }
  return(TRUE);
}

/* process a macro value - converts it into a string */
static char *process_macrostr(char *str)
{
  static char retstr[BUFFER_SIZE];
  char *s;
  int i;

  i = 0;
  s = str;
  retstr[0] = EOS;

  while (*s && i < (BUFFER_SIZE - 1))
    {
      if (*s == '\\')
	{
	  s++;

	  if (*s)
	    {
	      switch (*s)
		{
		case 'r':
		  retstr[i++] = '\r';
		  s++;
		  break;
		case 't':
		  retstr[i++] = '\t';
		  s++;
		  break;
		case 'n':
		  retstr[i++] = '\n';
		  s++;
		  break;
		case '\\':
		default:
		  retstr[i++] = '\\';
                  s++;
                  break;
		}
	    }

	}
      else
	{
	  retstr[i++] = *s;
	  s++;
	}
    }
      
  retstr[i] = EOS;

  return(retstr);
}

/* process a macro value - converts it back into a printable string */
static char *string_to_macro(char *str)
{
  static char retstr[BUFFER_SIZE];
  char *s;
  int i;

  i = 0;
  s = str;
  retstr[0] = EOS;

#if defined(DEBUG_CONFIG)
  if (str != NULL )
    clog("string_to_macro('%s')", str);
  else
    clog("string_to_macro(NULL)");
#endif

  while (*s && i < (BUFFER_SIZE - 1))
    {
      switch (*s)
	{
	case '\r':
	  retstr[i++] = '\\';
	  retstr[i++] = 'r';
	  s++;
	  break;
	case '\t':
	  retstr[i++] = '\\';
	  retstr[i++] = 't';
	  s++;
	  break;
	case '\n':
	  retstr[i++] = '\\';
	  retstr[i++] = 'n';
	  s++;
	  break;
	case '\\':
	  retstr[i++] = '\\';
	  retstr[i++] = '\\';
	  s++;
	  break;
	default:
	  retstr[i++] = *s;
	  s++;
	  break;
	}
    }

  retstr[i] = EOS;
  
  return(retstr);
}


/* process a boolean value */  

static int process_bool(char *bufptr)
{
  lower(bufptr);
  
  if ((strstr("false", bufptr) != NULL) || 
      (strstr("no", bufptr) != NULL)    ||
      (strstr("off", bufptr) != NULL))
    {
      return(FALSE);
    }
  else if ((strstr("true", bufptr) != NULL) ||
	   (strstr("yes", bufptr) != NULL)  ||
	   (strstr("on", bufptr) != NULL))
    {
      return(TRUE);
    }
  else
    {
      fprintf(stderr, "process_bool(): error parsing '%s', \n\t%s\n",
	      bufptr,
	      "Value must be yes or no or true or false.");
      return(ERR);
    }
}

/* MakeConf(filename) - make a fresh, spiffy new conquestrc file. */

static int MakeConf(char *filename)
{
  FILE *conf_fd;
  int i, j;

  if ((conf_fd = fopen(filename, "w")) == NULL)
    {
      clog("Makeconf(): fopen(%s) failed: %s",
	   filename,
	   sys_errlist[errno]);

      fprintf(stderr, "Error creating %s: %s\n",
	      filename,
	      sys_errlist[errno]);

      return(ERR);
    }


  for (j=0; j<CF_END; j++)
    {
				/* option header first */
      i = 0;

      /* check for version - this won't be output in the loop below due to the
         fact that CF_VERSION is of type CTYPE_NULL */

      if (j == CF_VERSION)
	{
	  fprintf(conf_fd, "%s%s\n",
		  ConfData[j].ConfName,
		  (char *)ConfData[j].ConfValue);
	}	  

      while (ConfData[j].ConfComment[i] != NULL)
	fprintf(conf_fd, "%s\n", ConfData[j].ConfComment[i++]);

				/* now write the variable and value if not a
				   CTYPE_NULL*/
      if (ConfData[j].ConfType != CTYPE_NULL)
	switch (ConfData[j].ConfType)
	  {
	  case CTYPE_MACRO:
	    fprintf(conf_fd, "%s%s\n",
		    ConfData[j].ConfName,
		    string_to_macro((char *)ConfData[j].ConfValue));
	    break;

	  case CTYPE_BOOL:
	    fprintf(conf_fd, "%s%s\n",
		    ConfData[j].ConfName,
		    (*((int *)ConfData[j].ConfValue) == TRUE) ? "true" : "false");
	    break;
	  }
				/* output a blank line */
      fprintf(conf_fd, "\n");
    }
      
  fclose(conf_fd);

#ifdef DEBUG_CONFIG
  clog("MakeConf(%s): Succeeded.", filename);
#endif

				/* that's it! */
  return(TRUE);
}


/* MakeSysConf(filename) - make a fresh, spiffy new sys-wide conquestrc file.*/

int MakeSysConf()
{
  FILE *sysconf_fd;
  char conf_name[BUFFER_SIZE];
  int i, j;

  sprintf(conf_name, "%s/%s", CONQHOME, SYSCONFIG_FILE);
  umask(002);
  if ((sysconf_fd = fopen(conf_name, "w")) == NULL)
    {
      clog("MakeSysconf(): fopen(%s) failed: %s",
	   conf_name,
	   sys_errlist[errno]);
      
      fprintf(stderr, "Error creating %s: %s\n",
	      conf_name,
	      sys_errlist[errno]);
      return(ERR);
    }

  fprintf(stderr, "Updating %s file...", conf_name);

  for (j=0; j<SYSCF_END; j++)
    {
				/* option header first */
      i = 0;

      /* check for version - this won't be output in the loop below due to the
         fact that SYSCF_VERSION is of type CTYPE_NULL */

      if (j == SYSCF_VERSION)
	{
	  fprintf(sysconf_fd, "%s%s\n",
		  SysConfData[j].ConfName,
		  (char *)SysConfData[j].ConfValue);
	}	  

      while (SysConfData[j].ConfComment[i] != NULL)
	fprintf(sysconf_fd, "%s\n", SysConfData[j].ConfComment[i++]);

				/* now write the variable and value if not a
				   CTYPE_NULL*/
      if (SysConfData[j].ConfType != CTYPE_NULL)
	switch (SysConfData[j].ConfType)
	  {
	  case CTYPE_MACRO:
	    fprintf(sysconf_fd, "%s%s\n",
		    SysConfData[j].ConfName,
		    string_to_macro((char *)SysConfData[j].ConfValue));
	    break;

	  case CTYPE_BOOL:
	    fprintf(sysconf_fd, "%s%s\n",
		    SysConfData[j].ConfName,
		    (*((int *)SysConfData[j].ConfValue) == TRUE) ? "true" : "false");
	    break;
	  }
				/* output a blank line */
      fprintf(sysconf_fd, "\n");
    }
      
  fclose(sysconf_fd);

#ifdef DEBUG_CONFIG
  clog("MakeSysConf(%s): Succeeded.", conf_name);
#endif

  fprintf(stderr, "Done.\n");

				/* that's it! */
  return(TRUE);
}

