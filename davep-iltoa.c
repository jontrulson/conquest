
/* itoa() shamelessly ripped off from FreeBSD  - dwp */
#define INT_DIGITS 19		/* enough for 64 bit integer */

char *itoa(int i)
{
  /* Room for INT_DIGITS digits, - and '\0' */
  static char buf[INT_DIGITS + 2];
  char *p = buf + INT_DIGITS + 1;	/* points to terminating '\0' */
  if (i >= 0) {
    do {
      *--p = '0' + (i % 10);
      i /= 10;
    } while (i != 0);
    return p;
  }
  else {			/* i < 0 */
    do {
      *--p = '0' - (i % 10);
      i /= 10;
    } while (i != 0);
    *--p = '-';
  }
  return p;
}

/* I did look at the FreeBSD itoa()! */
#define MAXDIGITS 19		/* enough for 64 bit integer */
char * ltoa(long snum)
{
  long tmp_num;
  /* Room for MAXDIGITS digits, - and '\0' */
  static char buf[MAXDIGITS + 2];
  char *p = buf + MAXDIGITS + 1;	/* points to terminating '\0' */
  *p = '\0';
  
  tmp_num = snum;
  do
    {
      if(tmp_num>=0)
	*--p = '0' + (tmp_num % 10);
      else
	*--p = '0' - (tmp_num % 10);
      tmp_num /= 10;
#ifdef LTOA_DEBUG
      clog("ltoa(): tmp_num: %u, snum: %u, (*p), %c\n",tmp_num, snum, (*p));
#endif
    } while (tmp_num!=0);
  if(snum<0)
    *--p ='-';
  
  return(p);

}
