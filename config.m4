dnl $Id$
dnl config.m4 for extension fast_excel

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(fast_excel, for fast_excel support,
dnl Make sure that the comment is aligned:
dnl [  --with-fast_excel             Include fast_excel support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(fast_excel, whether to enable fast_excel support,
Make sure that the comment is aligned:
[  --enable-fast_excel           Enable fast_excel support])

if test "$PHP_FAST_EXCEL" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-fast_excel -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/fast_excel.h"  # you most likely want to change this
  dnl if test -r $PHP_FAST_EXCEL/$SEARCH_FOR; then # path given as parameter
  dnl   FAST_EXCEL_DIR=$PHP_FAST_EXCEL
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for fast_excel files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       FAST_EXCEL_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$FAST_EXCEL_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the fast_excel distribution])
  dnl fi

  dnl # --with-fast_excel -> add include path
  dnl PHP_ADD_INCLUDE($FAST_EXCEL_DIR/include)

  dnl # --with-fast_excel -> check for lib and symbol presence
  dnl LIBNAME=fast_excel # you may want to change this
  dnl LIBSYMBOL=fast_excel # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $FAST_EXCEL_DIR/lib, FAST_EXCEL_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_FAST_EXCELLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong fast_excel lib version or lib not found])
  dnl ],[
  dnl   -L$FAST_EXCEL_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(FAST_EXCEL_SHARED_LIBADD)

  PHP_NEW_EXTENSION(fast_excel, 
      fast_excel.c  \
      mcb.h,  \
      mcb.c, 
      $ext_shared)
fi
