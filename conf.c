#include "c_defs.h"

/************************************************************************
 *
 * conf.c - handle configuration issues, both personal and system wide.
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include "global.h"
#include "conqcom.h"
#include "color.h"
#include "datatypes.h"
#include "conqnet.h"

#define CONF_NOEXTERN
#include "conf.h"
#undef CONF_NOEXTERN

#include "protocol.h"

#define HOME_BUFSZ 1024

/* if this is set, then the conquest client is running as the shell on
   a telnet based server.  In this case, we will never try to load or save
   the user config file.   GetConf() will only set the defaults. */
static int telnetClient = FALSE;

void confSetTelnetClientMode(int telnetc)
{
  telnetClient = telnetc;
  return;
}

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
  char conf_name[MID_BUFFER_SIZE];
  char buffer[BUFFER_SIZE];
  int FoundOne = FALSE;
  int buflen;
  char *bufptr;

				/* init some defaults */
  SysConf.NoDoomsday = FALSE;
  SysConf.DoRandomRobotKills = FALSE;
  SysConf.AllowSigquit = FALSE;
  SysConf.AllowSwitchteams = TRUE;
  SysConf.UserExpiredays = DEFAULT_USEREXPIRE;
  SysConf.LogMessages = FALSE;
  SysConf.AllowRefits = TRUE;
  SysConf.AllowSlingShot = FALSE;

  strncpy(SysConf.ServerName, "Generic Conquest Server", 
	  CONF_SERVER_NAME_SZ);
  strncpy(SysConf.ServerMotd, "Keep your shields up in battle.",
	  CONF_SERVER_MOTD_SZ);

				/* start building the filename */
  sprintf(conf_name, "%s/%s", CONQETC, SYSCONFIG_FILE);

  if ((conf_fd = fopen(conf_name, "r")) == NULL)
    {
      int err;

      err = errno;

      clog("GetSysConf(): fopen(%s) failed: %s",
	       conf_name,
	       strerror(err));

      if (checkonly != TRUE)
	{
	  fprintf(stderr, "Error opening system-wide config file: %s: %s\n",
		  conf_name,
		  strerror(err));
	  
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

      for (j = 0; (j < SysCfEnd) && (FoundOne == FALSE); j++)
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
			
		      case CTYPE_NUMERIC:
			if (alldig(bufptr))
			  {
			    *((int *) SysConfData[j].ConfValue) = atoi(bufptr);
			    SysConfData[j].Found = TRUE;
			    FoundOne = TRUE;
			  }
			break;

		      case CTYPE_STRING:
			memset((char *)(SysConfData[j].ConfValue), 0,
			       SysConfData[j].max);
			strncpy((char *)(SysConfData[j].ConfValue), 
				bufptr, SysConfData[j].max);
			((char *)SysConfData[j].ConfValue)[SysConfData[j].max - 1] = 0;

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
	      strerror(errno));
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
      for (i=0; i<SysCfEnd; i++)
	{
	  if (SysConfData[i].ConfType != CTYPE_NULL)
	    if (SysConfData[i].Found != TRUE)
	      {
#ifdef DEBUG_CONFIG
		clog("GetSysConf(): option '%s' not found - Update needed.",
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
int GetConf(int usernum)
{
  FILE *conf_fd;
  int i, j, n;
  char conf_name[MID_BUFFER_SIZE];
  char *homevar, *cptr;
  char home[HOME_BUFSZ];
  char buffer[BUFFER_SIZE];
  int buflen;
  char *bufptr;
  int FoundOne = FALSE;

				/* init some defaults */
  UserConf.DoAlarms = TRUE;
  UserConf.ShowPhasers = TRUE;
  UserConf.ShowPlanNames = TRUE;
  UserConf.DoIntrudeAlert = TRUE;
  UserConf.DoNumMap = TRUE;
  UserConf.Terse = FALSE;
  UserConf.DoExplode = TRUE;
  UserConf.MessageBell = TRUE;
  UserConf.NoColor = FALSE;
  UserConf.NoRobotMsgs = FALSE;
  UserConf.UpdatesPerSecond = 5;	/* default of 5 per sec */
  UserConf.DistressToFriendly = FALSE;
  UserConf.AltHUD = FALSE;
  UserConf.DoLRTorpScan = TRUE;
  UserConf.DoLocalLRScan = TRUE;
  UserConf.DoETAStats = TRUE;

  for (i=0; i<MAX_MACROS; i++)
    {
      UserConf.MacrosF[i][0] = EOS;
    }


  /* a telnet client leaves here after the defaults are set */
  if (telnetClient)
    return TRUE;

				/* start building the filename */
  if ((homevar = getenv("HOME")) == NULL)
    {
      clog("GetConf(): getenv(HOME) failed");

      fprintf(stderr, "Can't get HOME environment variable. Exiting\n");
      return(ERR);
    }

  memset(home, 0, HOME_BUFSZ);
  strncpy(home, homevar, HOME_BUFSZ - 1);

  sprintf(conf_name, "%s/%s", home, CONFIG_FILE);

  if ((conf_fd = fopen(conf_name, "r")) == NULL)
    {
      if (errno != ENOENT)
	{
	  clog("GetConf(): fopen(%s) failed: %s, using defaults",
	       conf_name,
	       strerror(errno));
	  
	  fprintf(stderr, "Error opening config file: %s: %s, using defaults\n",
		  conf_name,
		  strerror(errno));

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

      for (j = 0; (j < CfEnd) && (FoundOne == FALSE); j++)
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
				/* need to parse out macro number. */
			cptr = strchr(bufptr, '=');
			if (cptr != NULL)
			  {	/* valid entry */
			    *cptr = '\0';
			    n = atoi(bufptr);
			    if (n > 0 && n <= MAX_MACROS)
			      { /* valid macro number */
#ifdef DEBUG_CONFIG
				clog("GetConf(): Macro %d[%d], value '%s' read",
				     n, n - 1, (char *)cptr + 1);
#endif
				/* clean it first... */
				memset((char *)(((char *)ConfData[j].ConfValue)
					 + ((n - 1) * MAX_MACRO_LEN)), 
				       0,
				       MAX_MACRO_LEN);
				strncpy((char *)(((char *)ConfData[j].ConfValue)
					+ ((n - 1) * MAX_MACRO_LEN)), 
					Str2Macro((char *)cptr + 1), 
					MAX_MACRO_LEN - 1);
				ConfData[j].Found = TRUE;
				FoundOne = TRUE;
			      }
			  }
				
			break;

		      case CTYPE_NUMERIC:
			if (alldig(bufptr))
			  {
			    *((int *) ConfData[j].ConfValue) = atoi(bufptr);
			    ConfData[j].Found = TRUE;
			    FoundOne = TRUE;
			  }
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
	      strerror(errno));
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
      for (i=0; i<CfEnd; i++)
	{
	  if (ConfData[i].ConfType != CTYPE_NULL && 
	      ConfData[i].ConfType != CTYPE_MACRO )
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

/* SaveUserConfig(int unum) - do what the name implies ;-) */
int SaveUserConfig(int unum)
{
  char conf_name[MID_BUFFER_SIZE];
  char *homevar;
  char home[HOME_BUFSZ];

  /* telnet shell clients can't save their configs :( */
  if (telnetClient)
    {
      InitColors();
      return TRUE;
    }

				/* start building the filename */
  if ((homevar = getenv("HOME")) == NULL)
    {
      clog("SaveUserConfig(): getenv(HOME) failed");

      fprintf(stderr, "SaveUserConfig(): Can't get HOME environment variable. Exiting\n");
      return(ERR);
    }

  memset(home, 0, HOME_BUFSZ);
  strncpy(home, homevar, HOME_BUFSZ - 1);

  sprintf(conf_name, "%s/%s", home, CONFIG_FILE);

#ifdef DEBUG_OPTIONS
  clog("SaveUserConfig(): saving user config: conf_name = '%s'", conf_name);
#endif

				/* re-init in case it was changed */
  InitColors();

  return(MakeConf(conf_name));
}

/* SaveSysConfig() - do what the name implies ;-) */
int SaveSysConfig(void)
{
#ifdef DEBUG_OPTIONS
  clog("SaveSysConfig(): saving system config");
#endif

  return(MakeSysConf());
}

/* process a string value - converts it into a macro string */
char *Str2Macro(char *str)
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
char *Macro2Str(char *str)
{
  static char retstr[BUFFER_SIZE];
  char *s;
  int i;

  i = 0;
  s = str;
  retstr[0] = EOS;

#if defined(DEBUG_CONFIG)
  if (str != NULL )
    clog("Macro2Str('%s')", str);
  else
    clog("Macro2Str(NULL)");
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

int process_bool(char *bufptr)
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

int MakeConf(char *filename)
{
  FILE *conf_fd;
  int i, j, n;


  /* a telnet client should never get here, but... */
  if (telnetClient)
    return TRUE;

  unlink(filename);

  if ((conf_fd = fopen(filename, "w")) == NULL)
    {
      clog("Makeconf(): fopen(%s) failed: %s",
	   filename,
	   strerror(errno));

      fprintf(stderr, "Error creating %s: %s\n",
	      filename,
	      strerror(errno));

      return(ERR);
    }


  for (j=0; j<CfEnd; j++)
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
				   CTYPE_NULL */
      if (ConfData[j].ConfType != CTYPE_NULL)
	switch (ConfData[j].ConfType)
	  {
	  case CTYPE_MACRO:
	    for (n=0; n < MAX_MACROS; n++)
	      {
		if (strlen((char *)(((char *)ConfData[j].ConfValue)
					+ (n * MAX_MACRO_LEN))) != 0)
		  {
		    fprintf(conf_fd, "%s%d=%s\n",
			    ConfData[j].ConfName,
			    n + 1,
			    Macro2Str((char *)(((char *)ConfData[j].ConfValue)
					+ (n * MAX_MACRO_LEN))));
		  }
	      }
	    break;

	  case CTYPE_BOOL:
	    fprintf(conf_fd, "%s%s\n",
		    ConfData[j].ConfName,
		    (*((int *)ConfData[j].ConfValue) == TRUE) ? "true" : "false");
	    break;

	  case CTYPE_NUMERIC:
	    fprintf(conf_fd, "%s%d\n",
                    ConfData[j].ConfName,
		    *((int *)ConfData[j].ConfValue));
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


/* MakeSysConf() - make a fresh, spiffy new sys-wide conquestrc file.*/

int MakeSysConf()
{
  FILE *sysconf_fd;
  char conf_name[BUFFER_SIZE];
  int i, j, n;

  sprintf(conf_name, "%s/%s", CONQETC, SYSCONFIG_FILE);
  umask(002);
  unlink(conf_name);

  if ((sysconf_fd = fopen(conf_name, "w")) == NULL)
    {
      clog("MakeSysconf(): fopen(%s) failed: %s",
	   conf_name,
	   strerror(errno));
      
      fprintf(stderr, "Error creating %s: %s\n",
	      conf_name,
	      strerror(errno));
      return(ERR);
    }

  clog("OPER: Updating %s file...", conf_name);
  fprintf(stderr, "Updating %s file...", conf_name);

  for (j=0; j<SysCfEnd; j++)
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
	  case CTYPE_STRING:
	    fprintf(sysconf_fd, "%s%s\n",
		    SysConfData[j].ConfName,
		    (char *)SysConfData[j].ConfValue);
	    break;
	  case CTYPE_MACRO:
	    for (n=0; n < MAX_MACROS; n++)
	      {
		if (strlen(((char **)SysConfData[j].ConfValue)[n]) != 0)
		  {
		    fprintf(sysconf_fd, "%s%d=%s\n",
			    SysConfData[j].ConfName,
			    n + 1,
			    Macro2Str(((char **)SysConfData[j].ConfValue)[n]));
		  }
	      }
	    break;

	  case CTYPE_BOOL:
	    fprintf(sysconf_fd, "%s%s\n",
		    SysConfData[j].ConfName,
		    (*((int *)SysConfData[j].ConfValue) == TRUE) ? "true" : "false");
	    break;

	  case CTYPE_NUMERIC:
	    fprintf(sysconf_fd, "%s%d\n",
                    SysConfData[j].ConfName,
		    *((int *)SysConfData[j].ConfValue));
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

Unsgn32 getServerFlags(void)
{
  Unsgn32 f;

  /* get the current flags */
  f = SPSSTAT_FLAGS_NONE;

  if (SysConf.AllowRefits)
    f |= SPSSTAT_FLAGS_REFIT;

  if (SysConf.AllowSigquit)
    f |= SPSSTAT_FLAGS_VACANT;

  if (SysConf.AllowSlingShot)
    f |= SPSSTAT_FLAGS_SLINGSHOT;

  if (SysConf.NoDoomsday)
    f |= SPSSTAT_FLAGS_NODOOMSDAY;

  if (SysConf.DoRandomRobotKills)
    f |= SPSSTAT_FLAGS_KILLBOTS;

  if (SysConf.AllowSwitchteams)
    f |= SPSSTAT_FLAGS_SWITCHTEAM;

  return f;
}
