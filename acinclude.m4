dnl http://www.gnu.org/software/ac-archive/htmldoc/ac_func_snprintf.html

AC_DEFUN([AC_FUNC_SNPRINTF],
[AC_CHECK_FUNCS(snprintf vsnprintf)
AC_MSG_CHECKING(for working snprintf)
AC_CACHE_VAL(ac_cv_have_working_snprintf,
[AC_TRY_RUN(
[#include <stdio.h>

int main(void)
{
    char bufs[5] = { 'x', 'x', 'x', '\0', '\0' };
    char bufd[5] = { 'x', 'x', 'x', '\0', '\0' };
    int i;
    i = snprintf (bufs, 2, "%s", "111");
    if (strcmp (bufs, "1")) exit (1);
    if (i != 3) exit (1);
    i = snprintf (bufd, 2, "%d", 111);
    if (strcmp (bufd, "1")) exit (1);
    if (i != 3) exit (1);
    exit(0);
}], ac_cv_have_working_snprintf=yes, ac_cv_have_working_snprintf=no, ac_cv_have_working_snprintf=cross)])
AC_MSG_RESULT([$ac_cv_have_working_snprintf])
AC_MSG_CHECKING(for working vsnprintf)
AC_CACHE_VAL(ac_cv_have_working_vsnprintf,
[AC_TRY_RUN(
[#include <stdio.h>
#include <stdarg.h>

int my_vsnprintf (char *buf, const char *tmpl, ...)
{
    int i;
    va_list args;
    va_start (args, tmpl);
    i = vsnprintf (buf, 2, tmpl, args);
    va_end (args);
    return i;
}

int main(void)
{
    char bufs[5] = { 'x', 'x', 'x', '\0', '\0' };
    char bufd[5] = { 'x', 'x', 'x', '\0', '\0' };
    int i;
    i = my_vsnprintf (bufs, "%s", "111");
    if (strcmp (bufs, "1")) exit (1);
    if (i != 3) exit (1);
    i = my_vsnprintf (bufd, "%d", 111);
    if (strcmp (bufd, "1")) exit (1);
    if (i != 3) exit (1);
    exit(0);
}], ac_cv_have_working_vsnprintf=yes, ac_cv_have_working_vsnprintf=no, ac_cv_have_working_vsnprintf=cross)])
AC_MSG_RESULT([$ac_cv_have_working_vsnprintf])
if test x$ac_cv_have_working_snprintf$ac_cv_have_working_vsnprintf != "xyesyes"; then
  AC_LIBOBJ(snprintf)
  AC_MSG_WARN([Replacing missing/broken (v)snprintf() with version from http://www.ijs.si/software/snprintf/.])
  AC_DEFINE(PREFER_PORTABLE_SNPRINTF, 1, "enable replacement (v)snprintf if system (v)snprintf is broken")
fi])

dnl borrowed from INN

define([_CONQ_HEADER_SOURCE],
[#include <stdio.h>
#include <sys/types.h>
#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#else
# if HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif])

dnl Source used by CONQ_FUNC_MMAP.
define([_CONQ_FUNC_MMAP_SOURCE],
[_CONQ_HEADER_SOURCE()]
[[#include <fcntl.h>
#include <sys/mman.h>

int
main()
{
  int *data, *data2;
  int i, fd;

  /* First, make a file with some known garbage in it.  Use something
     larger than one page but still an odd page size. */
  data = malloc (20000);
  if (!data) return 1;
  for (i = 0; i < 20000 / sizeof (int); i++)
    data[i] = rand();
  umask (0);
  fd = creat ("conftestmmaps", 0600);
  if (fd < 0) return 1;
  if (write (fd, data, 20000) != 20000) return 1;
  close (fd);

  /* Next, try to mmap the file and make sure we see the same garbage. */
  fd = open ("conftestmmaps", O_RDWR);
  if (fd < 0) return 1;
  data2 = mmap (0, 20000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data2 == (int *) -1) return 1;
  for (i = 0; i < 20000 / sizeof (int); i++)
    if (data[i] != data2[i])
      return 1;

  close (fd);
  unlink ("conftestmmaps");
  return 0;
}]])


dnl This portion is similar to what AC_FUNC_MMAP does, only it tests shared,
dnl non-fixed mmaps.
AC_DEFUN([CONQ_FUNC_MMAP],
[AC_CACHE_CHECK(for working mmap MAP_SHARED, conq_cv_func_mmap,
[AC_TRY_RUN(_CONQ_FUNC_MMAP_SOURCE(),
            conq_cv_func_mmap=yes,
            conq_cv_func_mmap=no,
            conq_cv_func_mmap=no)])
if test $conq_cv_func_mmap = yes ; then
    AC_DEFINE(HAVE_MMAP, 1, [mmap supports MAP_SHARED])
fi])


