dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.52)

dnl ****************************************************************************
dnl MAIN BASTILE VERSION (update after release)

AC_INIT(bastile, 1.1.0)

dnl ****************************************************************************
dnl LIBMATECRYPTUI libtool versioning
dnl CURRENT : REVISION : AGE
dnl   +1    :     0    : +1   == new interface that does not break old one.
dnl   +1    :     0    :  0   == removed an interface. Breaks old apps.
dnl    ?    :    +1    :  ?   == internal changes that doesn't break anything.

LIBMATECRYPTUI_CURRENT=0
LIBMATECRYPTUI_REVISION=0
LIBMATECRYPTUI_AGE=0

dnl ****************************************************************************

AC_CONFIG_SRCDIR(src/bastile-main.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INTLTOOL([0.35.0])
AM_PROG_LIBTOOL
AM_MATECONF_SOURCE_2
MATE_DOC_INIT
GTK_DOC_CHECK([1.9])

dnl Get the off_t size right
AC_SYS_LARGEFILE
AC_TYPE_OFF_T
AC_FUNC_FSEEKO

AC_CHECK_FUNCS(setresuid setresgid)
AC_CHECK_FUNCS(strsep)

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

AC_MSG_CHECKING([which GTK+ version to compile against])
AC_ARG_WITH([gtk],
	[AS_HELP_STRING([--with-gtk=2|3],[which GTK+ version to compile against (default: 3)])],
	[case "$with_gtk" in
		2*|3*) ;;
		*) AC_MSG_ERROR([invalid GTK+ version specified]) ;;
	esac],
	[with_gtk=3])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
	2*)
		GCR_LIBRARY=gcr-0
		GTK_API_VERSION=2.0
		GTK_REQUIRED=2.20.0
		with_gtk=2.0
	;;
	3*)
		GCR_LIBRARY=gcr-3
		GTK_API_VERSION=3.0
		GTK_REQUIRED=2.90.0
		with_gtk=3.0
	;;
esac

AC_SUBST(GTK_API_VERSION)
AM_CONDITIONAL([WITH_GTK3],[test "$with_gtk" = "3.0"])

PKG_CHECK_MODULES(GTK, gtk+-$GTK_API_VERSION >= $GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(BASTILE, gmodule-2.0 gio-2.0 mateconf-2.0 gthread-2.0 gtk+-$GTK_API_VERSION >= $GTK_REQUIRED)

AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-cygwin*|*-*-mingw*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")

if test "$platform_win32" = "yes" ; then
	PLUGIN_LIBTOOL_FLAGS="-module -avoid-version -no-undefined"
else
	PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
fi
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

dnl **************************************************************
dnl GPG / GPGME CHECKS
dnl **************************************************************

AC_ARG_ENABLE(pgp,
	AC_HELP_STRING([--enable-pgp], [enable PGP support (default is auto)]))
	
have_gpg=""
have_gpgme=""

if test "$enable_pgp" = "no"; then
	echo "disabling PGP support"
else
	AC_ARG_ENABLE(gpg-check, 
		AC_HELP_STRING([--disable-gpg-check], [check GPG version (default is yes)]),
		DO_CHECK=$enableval, DO_CHECK=yes)
	
	if test	"$DO_CHECK" = "yes"; then
		accepted_versions="1.2 1.4 2.0"
		AC_PATH_PROGS(GNUPG, [gpg gpg2], no)
		AC_DEFINE_UNQUOTED(GNUPG, "$GNUPG", [Path to gpg executable.])
		ok="no"
		if test "$GNUPG" != "no"; then
			AC_MSG_CHECKING(for appropriate GnuPG version)
			gnupg_version=`$GNUPG --version | grep gpg`
			major=`echo $gnupg_version | \
				sed 's/^gpg (GnuPG) \([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\1/'`
			minor=`echo $gnupg_version | \
				sed 's/^gpg (GnuPG) \([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\2/'`
			micro=`echo $gnupg_version | \
				sed 's/^gpg (GnuPG) \([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\3/'`
    
			for ver in $accepted_versions; do
				if test "$ver" = "$major.$minor"; then
					AC_DEFINE_UNQUOTED(GPG_MAJOR, [$major], [GPG Major Version])
					AC_DEFINE_UNQUOTED(GPG_MINOR, [$minor], [GPG Minor Version])
					AC_DEFINE_UNQUOTED(GPG_MICRO, [$micro], [GPG Micro Version])
					ok="yes"
					break
				fi
			done
		fi
  
		if test "$ok" = "yes"; then
			AC_MSG_RESULT([yes])
			have_gpg="$gnupg_version"
		else
			AC_MSG_RESULT([no])
		fi
	fi


	ok="no"
	min_gpgme_version=1.0.0
	AC_PATH_PROG(GPGME_CONFIG, gpgme-config, "failed")
	if test $GPGME_CONFIG != "failed" ; then
      		AC_MSG_CHECKING(for GPGME - version >= $min_gpgme_version)
		req_major=`echo $min_gpgme_version | \
			sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\1/'`
		req_minor=`echo $min_gpgme_version | \
			sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\2/'`
		req_micro=`echo $min_gpgme_version | \
			sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\3/'`
		gpgme_config_version=`$GPGME_CONFIG --version`
		major=`echo $gpgme_config_version | \
			sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\1/'`
		minor=`echo $gpgme_config_version | \
			sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\2/'`
		micro=`echo $gpgme_config_version | \
			sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\3/'`

		if test "$major" -eq "$req_major"; then
			if test "$minor" -ge "$req_minor"; then
				if test "$micro" -ge "$req_micro"; then
					ok="yes"
				fi
			fi
		fi
	fi

	if test $ok = "yes"; then
		GPGME_CFLAGS=`$GPGME_CONFIG --cflags`
		GPGME_LIBS=`$GPGME_CONFIG --libs`
		AC_MSG_RESULT(yes)
		have_gpgme="$gpgme_config_version"
	else
		AC_MSG_RESULT(no)
	fi
fi

if test "$enable_pgp" != "no" -a -n "$have_gpg" -a -n "$have_gpgme"; then
	enable_pgp="yes"
fi

if test "$enable_pgp" = "yes"; then
	if test -z "$have_gpg"; then
		AC_MSG_ERROR([Appropriate version of GnuPG not found. Please install one of versions: $accepted_versions])
	fi
			
	if test -z "$have_gpgme"; then
		AC_MSG_ERROR(GPGME $min_gpgme_version or later needed)
	fi

	BASTILE_CFLAGS="$BASTILE_CFLAGS $GPGME_CFLAGS"
	BASTILE_LIBS="$BASTILE_LIBS $GPGME_LIBS"
        AC_DEFINE(WITH_PGP, 1, [With PGP Support])
fi

AM_CONDITIONAL(WITH_PGP, test "$enable_pgp" = "yes")

dnl ******************************************************************************
dnl LDAP [from gpg]
dnl ******************************************************************************

AC_MSG_CHECKING([LDAP keyserver support])
AC_ARG_ENABLE(ldap,
[  --disable-ldap          disable LDAP keyserver interface],
    try_ldap=$enableval, try_ldap=yes)

if test "$enable_pgp" != "yes"; then
	echo "LDAP keyserver interface requires PGP support"
	try_ldap=no
fi

AC_MSG_RESULT($try_ldap)
with_ldap=no

dnl Must check for network library requirements before doing link tests
dnl for ldap, for example. If ldap libs are static (or dynamic and without
dnl ELF runtime link paths), then link will fail and LDAP support won't
dnl be detected.

AC_CHECK_FUNC(gethostbyname, , AC_CHECK_LIB(nsl, gethostbyname,
	[NETLIBS="-lnsl $NETLIBS"]))
AC_CHECK_FUNC(setsockopt, , AC_CHECK_LIB(socket, setsockopt,
	[NETLIBS="-lsocket $NETLIBS"]))
	
dnl Try and link a LDAP test program to weed out unusable LDAP
dnl libraries.  -lldap [-llber [-lresolv]] is for OpenLDAP.  OpenLDAP in
dnl general is terrible with creating weird dependencies.  If all else
dnl fails, the user can play guess-the-dependency by using something
dnl like LDAP_LIBS="-lfoo" ./configure

if test "$try_ldap" = yes ; then
  for MY_LDAPLIBS in ${LDAP_LIBS+"$LDAP_LIBS"} "-lldap" "-lldap -llber" "-lldap -llber -lresolv"; do
    _ldap_save_libs=$LIBS
    LIBS="$MY_LDAPLIBS $NETLIBS $LIBS"

    AC_MSG_CHECKING([whether LDAP via \"$MY_LDAPLIBS\" is present and sane])
    AC_TRY_LINK([#include <ldap.h>],[ldap_open("foobar",1234);],
                [cv_func_ldap_init=yes],[cv_func_ldap_init=no])
    AC_MSG_RESULT([$cv_func_ldap_init])

    if test $cv_func_ldap_init = no; then
      AC_MSG_CHECKING([whether I can make LDAP be sane with lber.h])
      AC_TRY_LINK([#include <lber.h>
#include <ldap.h>],[ldap_open("foobar",1234);],
         [cv_func_ldaplber_init=yes],[cv_func_ldaplber_init=no])
      AC_MSG_RESULT([$cv_func_ldaplber_init])
    fi

    if test "$cv_func_ldaplber_init" = yes ; then
       AC_DEFINE(NEED_LBER_H,1,[Define if the LDAP library requires including lber.h before ldap.h])
    fi

    if test "$cv_func_ldap_init" = yes || \
  	test "$cv_func_ldaplber_init" = yes ; then
       LDAP_LIBS=$MY_LDAPLIBS
    fi

    LIBS=$_ldap_save_libs

    if test "$LDAP_LIBS" != ""; then 
    	with_keyserver=yes
        with_ldap=yes
		AC_DEFINE(WITH_LDAP, 1, [Support for LDAP operations])
    	break
    fi
  done
fi	

AM_CONDITIONAL(WITH_LDAP, test "$with_ldap" = "yes")
BASTILE_LIBS="$BASTILE_LIBS $LDAP_LIBS"

AC_SUBST(NETLIBS)


dnl ****************************************************************************
dnl HKP (libsoup)
dnl ****************************************************************************

# Allow hkp to be disabled 
AC_ARG_ENABLE(hkp,
        AC_HELP_STRING([--disable-hkp],
        [disable HKP keyserver interface]))

# HKP tests
if test "$enable_pgp" != "yes"; then
	echo "HKP keyserver interface requires PGP support"
	enable_hkp=no
fi
 
if test "$enable_hkp" = "no"; then
    echo "disabling HKP keyserver interface"
else
    
    echo "checking for supported versions of libsoup..."

    PKG_CHECK_MODULES(SOUP, libsoup-2.4, 
        [enable_hkp=yes],  
        [enable_hkp=no])

    if test "$enable_hkp" = yes; then
        with_keyserver=yes
        
        AC_DEFINE(WITH_HKP, 1, [Support for HKP operations])
        BASTILE_LIBS="$BASTILE_LIBS $SOUP_LIBS"
        BASTILE_CFLAGS="$BASTILE_CFLAGS $SOUP_CFLAGS"
        
        # libsoup is also used for async lookups
        AC_DEFINE(WITH_SOUP, 1, [With libsoup])
    fi
fi

AM_CONDITIONAL(WITH_HKP, test "$enable_hkp" = "yes")


dnl ***************
dnl KEYSERVER
dnl ***************

AM_CONDITIONAL(WITH_KEYSERVER, test "$with_keyserver" = "yes")

if test "$with_keyserver" = "yes"; then
	echo "enabling key server support"
	AC_DEFINE(WITH_KEYSERVER, 1, [Support for key server actions])
else
    with_keyserver=no
	echo "disabling key server support"
fi


dnl ****************************************************************************
dnl MATE-KEYRING
dnl ****************************************************************************

PKG_CHECK_MODULES([MATE_KEYRING], mate-keyring-1 >= 1.1.0,
                  [enable_mate_keyring=yes],[enable_mate_keyring=no])

if test "$enable_mate_keyring" = "no"; then
    AC_MSG_ERROR([mate-keyring library or headers not found.])
fi

BASTILE_CFLAGS="$BASTILE_CFLAGS $MATE_KEYRING_CFLAGS"
BASTILE_LIBS="$BASTILE_LIBS $MATE_KEYRING_LIBS"
        

dnl *******************
dnl KEY SHARING 
dnl *******************

AVAHI_GLIB_REQUIRED=0.6

# Allow sharing to be disabled
AC_ARG_ENABLE(sharing, 
        AC_HELP_STRING([--disable-sharing],
        [Don't compile in DNS-SD key sharing]))

if test "$enable_pgp" != "yes"; then
	echo "DNS-SD key sharing requires PGP support"
	enable_sharing=no
fi
	
if test "$enable_hkp" = "no"; then
    	echo "DNS-SD key sharing requires HKP support"
	enable_sharing=no
fi

if test "$enable_sharing" = "no"; then
    echo "disabling DNS-SD key sharing"
else

    PKG_CHECK_MODULES(SHARING, avahi-client avahi-glib >= $AVAHI_GLIB_REQUIRED, 
        [enable_sharing=yes],  
        [enable_sharing=no])
		
	if test "$enable_sharing" = "yes"; then
		echo "enabling key sharing"
		
		AC_DEFINE_UNQUOTED(WITH_SHARING, 1, [Compile bonjour key sharing code])
		BASTILE_CFLAGS="$BASTILE_CFLAGS $SHARING_CFLAGS"
		BASTILE_LIBS="$BASTILE_LIBS $SHARING_LIBS"
    fi
fi

AM_CONDITIONAL(WITH_SHARING, test "$enable_sharing" = "yes")


dnl ****************************************************************************
dnl DBUS
dnl ****************************************************************************

DBUS_GLIB_REQUIRED=0.35

PKG_CHECK_MODULES([DBUS], [dbus-glib-1 >= $DBUS_GLIB_REQUIRED], [enable_dbus=yes],[enable_dbus=no])
if test "$enable_dbus" = "no"; then
    AC_MSG_ERROR([dbus or dbus-glib-1 bindings not found.])
fi

AC_SUBST([DBUS_CFLAGS])
AC_SUBST([DBUS_LIBS])

AC_PATH_PROG([DBUS_BINDING_TOOL], [dbus-binding-tool], [no])
if test x$DBUS_BINDING_TOOL = "xno" ; then
    AC_MSG_ERROR([dbus-binding-tool executable not found in your path - should be installed with dbus glib bindings])
fi

BASTILE_CFLAGS="$BASTILE_CFLAGS $DBUS_CFLAGS"
BASTILE_LIBS="$BASTILE_LIBS $DBUS_LIBS"

dnl *******************
dnl LIBMATENOTIFY
dnl *******************

LIBMATENOTIFY_REQUIRED=0.3

# Allow libmatenotify to be disabled
AC_ARG_ENABLE(libmatenotify, 
        AC_HELP_STRING([--disable-libmatenotify],
        [Don't compile in libmatenotify support]))   
        
if test "x$enable_libmatenotify" = "xno"; then
    echo "disabling libmatenotify support"
else

    PKG_CHECK_MODULES(LIBMATENOTIFY, [libmatenotify >= $LIBMATENOTIFY_REQUIRED], 
                      enable_libmatenotify=yes, enable_libmatenotify=no)
    if test "$enable_libmatenotify" = "yes"; then
        
        AC_DEFINE(HAVE_LIBMATENOTIFY, 1, [libmatenotify available])
        
        BASTILE_CFLAGS="$BASTILE_CFLAGS $LIBMATENOTIFY_CFLAGS"
        BASTILE_LIBS="$BASTILE_LIBS $LIBMATENOTIFY_LIBS"
    fi
fi

dnl ****************************************************************************
dnl  PKCS11
dnl ****************************************************************************

AC_ARG_ENABLE(pkcs11,
	AC_HELP_STRING([--disable-pkcs11],
		[Support keys and certificates via PKCS#11]))

if test "$enable_pkcs11" = "no"; then
	echo "disabling pkcs11 support"
	enable_pkcs11="no"
else
	PKG_CHECK_MODULES(GCR, $GCR_LIBRARY >= 2.25.0, gcr_is_present="yes", gcr_is_present="no")
	if test "$gcr_is_present" = "no"; then
		echo "disabling pkcs11 support"
	fi
	PKG_CHECK_MODULES(GCK, gck-0 >= 2.91.2, enable_pkcs11="yes", enable_pkcs11="no")
	if test "$enable_pkcs11" = "yes" && test "$gcr_is_present" = "yes"; then
		AC_DEFINE(WITH_PKCS11, 1, [gck library available])
		BASTILE_CFLAGS="$BASTILE_CFLAGS $GCK_CFLAGS $GCR_CFLAGS"
		BASTILE_LIBS="$BASTILE_LIBS $GCK_LIBS $GCR_LIBS"
	else
		enable_pkcs11="no"
	fi
fi

AM_CONDITIONAL(WITH_PKCS11, test "$enable_pkcs11" = "yes")

dnl ****************************************************************************
dnl  SSH 
dnl ****************************************************************************

AC_ARG_ENABLE(ssh,
        AC_HELP_STRING([--disable-ssh], 
        [Support for managing SSH keys]))
        
if test "x$enable_ssh" = "xno"; then
    echo "disabling ssh support"
else

#    AC_PATH_PROG(SSH_KEYGEN_PATH, ssh-keygen, "failed")
#    AC_PATH_PROG(SSH_PATH, ssh, "failed")
    
#    if test $SSH_KEYGEN_PATH != "failed" && i
#       test $SSH_PATH != "failed"; then
        enable_ssh="yes"
        AC_DEFINE(WITH_SSH, 1, [With SSH Support])
        AC_DEFINE_UNQUOTED(SSH_KEYGEN_PATH, "/usr/bin/ssh-keygen", [Path to ssh-keygen])
        AC_DEFINE_UNQUOTED(SSH_PATH, "/usr/bin/ssh", [Path to ssh])
#    fi
    
fi

AM_CONDITIONAL(WITH_SSH, test "$enable_ssh" = "yes")

dnl ****************************************************************************

# Debug mode
AC_ARG_ENABLE(debug, 
	    AC_HELP_STRING([--enable-debug],
	    [Compile binaries in debug mode]))

if test "$enable_debug" = "yes"; then
  CFLAGS="$CFLAGS -g -O0 -DMATECOMPONENT_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED -Wall"
  AC_DEFINE_UNQUOTED(_DEBUG, 1, [In debug mode])
  echo "enabling debug compile mode"
else 
  dnl AC_DEFINE_UNQUOTED(G_DISABLE_ASSERT, 1, [Disable glib assertions])
  echo "disabling debug compile mode"
fi

# Fatal Messages mode
AC_ARG_ENABLE(fatal-messages, 
	    AC_HELP_STRING([--enable-fatal-messages],
	    [Don't make compile and or runtime messages fatal in debug mode.]))

if test "$enable_fatal_messages" = "yes" && test "$enable_debug" = "yes"; then
    CFLAGS="$CFLAGS -Wall -Werror"
    AC_DEFINE_UNQUOTED(FATAL_MESSAGES, 1, [Fatal runtime messages])
    echo "enabling fatal runtime messages"
fi

# Build Tests
AC_ARG_ENABLE(tests,
        AC_HELP_STRING([--enable-tests],
        [Compile Test Programs]))
        
if test "$enable_tests" != "yes"; then
    enable_tests="no"
fi

AM_CONDITIONAL(WITH_TESTS, test "$enable_tests" = "yes")

dnl ****************************************************************************
dnl LIBMATECRYPTUI
dnl ****************************************************************************
   
LIBMATECRYPTUI_LT_RELEASE=$LIBMATECRYPTUI_CURRENT:$LIBMATECRYPTUI_REVISION:$LIBMATECRYPTUI_AGE
AC_SUBST(LIBMATECRYPTUI_LT_RELEASE)

LIBMATECRYPTUI_MAJOR=$LIBMATECRYPTUI_CURRENT.$LIBMATECRYPTUI_REVISION
AC_SUBST(LIBMATECRYPTUI_MAJOR)

PKG_CHECK_MODULES(LIBMATECRYPTUI, gtk+-$GTK_API_VERSION >= $GTK_REQUIRED mateconf-2.0)

cryptuitargetlib=libmatecryptui.la
AC_SUBST(cryptuitargetlib)

LIBMATECRYPTUI_CFLAGS="$LIBMATECRYPTUI_CFLAGS $DBUS_CFLAGS"
LIBMATECRYPTUI_LIBS="$LIBMATECRYPTUI_LIBS $DBUS_LIBS"
AC_SUBST(LIBMATECRYPTUI_CFLAGS)
AC_SUBST(LIBMATECRYPTUI_LIBS)

GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
ATK_PREFIX="`$PKG_CONFIG --variable=prefix atk`"
PANGO_PREFIX="`$PKG_CONFIG --variable=prefix pango`"
CAIRO_PREFIX="`pkg-config --variable=prefix cairo`" 

AC_SUBST(GLIB_PREFIX)
AC_SUBST(ATK_PREFIX)
AC_SUBST(PANGO_PREFIX)
AC_SUBST(CAIRO_PREFIX)

dnl ****************************************************************************
dnl GObject Introspection
dnl ****************************************************************************

GOBJECT_INTROSPECTION_CHECK([0.6.4])

dnl ****************************************************************************

AC_SUBST(BASTILE_CFLAGS)
AC_SUBST(BASTILE_LIBS)

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

AC_ARG_ENABLE(update-mime-database,
	AC_HELP_STRING([--disable-update-mime-database],
	[do not run the update-mime-database utility (mostly useful for package maintainers) ]))
AM_CONDITIONAL(UPDATE_MIME_DATABASE, test "$enable_update_mime_database" != "no")

dnl *******************
dnl ***** gettext *****
dnl *******************

GETTEXT_PACKAGE=bastile
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the gettext domain.])

AM_GLIB_GNU_GETTEXT

dnl ****************************************************************************
## here we get the flags we'll actually use
PKG_CHECK_MODULES(EGG, gtk+-$GTK_API_VERSION >= $GTK_REQUIRED)
AC_SUBST(EGG_LIBS)
AC_SUBST(EGG_CFLAGS)

PKG_CHECK_MODULES(EGG_SMCLIENT, gtk+-$GTK_API_VERSION)
AC_SUBST(EGG_SMCLIENT_LIBS)
AC_SUBST(EGG_SMCLIENT_CFLAGS)

dnl ****************************************************************************

AC_OUTPUT([
Makefile
bastile.spec
common/Makefile
gkr/Makefile
libmatecryptui/matecryptui.pc
libmatecryptui/Makefile
libmatecryptui/tests/Makefile
libegg/Makefile
libbastile/Makefile
pgp/Makefile
po/Makefile.in
help/Makefile
data/Makefile
data/icons/Makefile
pixmaps/Makefile
pixmaps/22x22/Makefile
pixmaps/48x48/Makefile
pixmaps/scalable/Makefile
pkcs11/Makefile
src/Makefile
src/bastile.desktop.in
ssh/Makefile
daemon/Makefile
reference/Makefile
reference/libmatecryptui/Makefile
reference/libmatecryptui/version.xml
reference/libbastile/Makefile
reference/libbastile/version.xml
])

dnl ****************************************************************************
dnl  SUMMARY
dnl ****************************************************************************

echo "
GTK+ Version:               $GTK_API_VERSION
PKCS11 Support:             $enable_pkcs11
PGP Support:                $enable_pgp
  GnuPG Version:            $have_gpg
  GPGME Version:            $have_gpgme
SSH Support:                $enable_ssh
Keyserver Support:          $with_keyserver
  LDAP:                     $with_ldap
  HKP:                      $enable_hkp
  Key Sharing:              $enable_sharing
Notification Support:       $enable_libmatenotify
Build introspection data:   ${enable_introspection}
"
