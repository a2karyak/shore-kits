m4_pattern_allow([^AC_PACKAGE])

# configure.in for shmtclient         -*- sh -*-
# Process this file with autoconf to produce a configure script. 
AC_INIT(THIS_IS_SHORE_KITS,2.1)

# make sure I have not forgotten to run autoconf
if test configure -ot configure.in; then
  AC_MSG_RESULT(configure is older than configure.in; running autoconf)
  autoconf
  ./configure
  exit
fi 

echo configure args: $* > tmpcfg.txt


# Remember the user set CXXFLAGS
USERCXXFLAGS="$CXXFLAGS"
#USERCXXFLAGS=""

# find system type (using this macro means we must include
# the files install-sh, config.sub, and config.guess (all from
# the autoconf distribution) in our source tree!)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE
AC_LANG(C++)

AC_PREREQ(2.50)
AC_PROG_CXX
AC_PROG_RANLIB

AC_MSG_NOTICE(Configuring Shore Kits)

# Set here the version numbers
KITS_VERSION_MAJOR="shore-kits"
KITS_VERSION_MINOR=2
KITS_VERSION_REV=1
KITS_VERSION=$KITS_VERSION_MAJOR.$KITS_VERSION_MINOR.$KITS_VERSION_REV

# check for egrep
AC_PROG_EGREP

# debugging diagnostic; set to 'echo' to debug or 'true' for production
diagnostic() {
  echo "$@"
  #true "$@"
}

# determine if a binary is in the path
binaryExists() {
  # on cygwin, 'which' always returns success, so use 'type' instead
  if type "$1" >/dev/null 2>&1; then
    return 0
  else
    return 1
  fi
}


# ---------- Compiler Checking ----------
KITS_CXX=`echo $CXX |  sed 's/.*\///'`
AC_MSG_CHECKING(CXX)
AC_MSG_RESULT($KITS_CXX)

KITS_CXX_VERSION="0.0"


if test "$KITS_CXX" = "CC"; then
   # ------ (SUN) C++ compiler ------

   # cc version
   AC_MSG_CHECKING(compiler version)
   cc_ver=`$CXX -V 2>&1 | grep 'C++' | sed 's/^.*C++ //' | sed 's/ .*//'`
   cc_major=`echo $cc_ver | sed 's/\..*$//'`
   [cc_mid=`echo $cc_ver | sed 's/[0-9]\.//' | sed 's/\..*$//'`]
   KITS_CXX_VERSION="$cc_major.$cc_mid"
   AC_MSG_RESULT($KITS_CXX_VERSION)

   # *** Checking the compiler version (CC>=5.8)
   reqcc_major=5
   reqcc_mid=8
   if test $cc_major -lt $reqcc_major -o $cc_mid -lt $reqcc_mid; then
      AC_MSG_WARN([CXX Compiler $KITS_CXX version $KITS_CXX_VERSION is currently not supported by Shore-kits])
      AC_MSG_WARN([Compiled successfully with cc Compiler $reqcc_major.$reqcc_mid or higher])
   fi   

else
   # ------ GNU C++ compiler ------
   # gcc
   if binaryExists g++33; then
      GCC_COMPILER=g++
   else
      if binaryExists g++; then
         GCC_COMPILER=g++
      else
         AC_MSG_ERROR([
            No suitable compiler found. 
	    Please make sure gcc is in your path.
         ])
      fi
   fi

   # gcc version
   AC_MSG_CHECKING(compiler version)

   gcc_ver=`$GCC_COMPILER --version | grep 'g++' | sed 's/^.*(.*) //'`
   gcc_major=`echo $gcc_ver | sed 's/\..*$//'`
   [gcc_mid=`echo $gcc_ver | sed 's/[0-9]\.//' | sed 's/\..*$//'`]
   [gcc_minor=`echo $gcc_ver | sed 's/^[0-9]\.[0-9]\.//' | sed 's/ .*$//'`]

   KITS_CXX_VERSION="$gcc_major.$gcc_mid.$gcc_minor"
   AC_MSG_RESULT($KITS_CXX_VERSION)

   # *** Checking the compiler version (g++>=4.1)
   reqgcc_major=4
   reqgcc_mid=1
   if test $gcc_major -lt $reqgcc_major -o $gcc_mid -lt $reqgcc_mid; then
      AC_MSG_WARN([GCC Compiler $GCC_COMPILER version $KITS_CXX_VERSION is currently not supported by Shore-kits])
      AC_MSG_WARN([Compiled successfully with GCC Compiler $reqgcc_major.$reqgcc_mid or higher])
   fi   
fi 
# ---------- EOF Compiler Checking ----------




# ----------- Readline location -------------
AC_ARG_WITH([readline],
  [AS_HELP_STRING([--with-readline],
                  [support fancy command line editing @<:@default=check@:>@])],
  [if test -z "$with_readline"; then with_readline=yes; fi],
  [with_readline=check])

if test "x$with_readline" != "xno"; then
  CPPFLAGS_SAVE="$CPP_FLAGS"
  LDFLAGS_SAVE="$LDFLAGS"
  case "$with_readline" in
    yes | no | check )
      CPPFLAGS=
      LDFLAGS=
      ;;
    * )
      READLINE_HOME=$with_readline
      CPPFLAGS="-I$with_readline/include"
      LDFLAGS="-L$with_readline/lib -L$with_readline/lib64 -L$with_readline/lib/64"
      ;;
  esac

  AC_CHECK_HEADERS([readline/readline.h],
    [],
    [if test "$with_readline" == "check"; then
       with_readline=no
     else
       AC_MSG_FAILURE([Readline requested but headers not found])
     fi])
     
  if test "$with_readline" != "no"; then
    AC_CHECK_LIB([readline], [readline],
      [],
      [if test "$with_readline" == "check"; then
         with_readline=no
       else
         AC_MSG_FAILURE([Readline requested but library not found])
       fi],
      [])
  fi

  if test "$with_readline" != "no"; then
    AC_DEFINE(HAVE_READLINE, [1], [Readline library is available])
    AC_SUBST([READLINE_INCLUDE], ["$CPPFLAGS"])
    AC_SUBST([READLINE_LIB], ["$LDFLAGS"])
  fi
  
  CPPFLAGS="$CPPFLAGS_SAVE"
  LDFLAGS="$LDFLAGS_SAVE"
fi
AC_MSG_CHECKING(for readline)
AC_MSG_RESULT($with_readline)

# ----------- Glibtop location -------------
AC_ARG_WITH([glibtop],
  [AS_HELP_STRING([--with-glibtop],
                  [Use glibtop-2.0 to report CPU utilization @<:@default=check@:>@])],
  [PKG_CHECK_MODULES([GTOP], [libgtop-2.0])
   AC_DEFINE(HAVE_GLIBTOP, [1], [Glibtop available])
   AC_SUBST([GLIBTOP_INCLUDE], ["$GTOP_CFLAGS"])
   AC_SUBST([GLIBTOP_LIB], ["$GTOP_LIBS"])
   ], [with_glibtop=no]
   )
AC_MSG_CHECKING(for glibtop)
AC_MSG_RESULT($with_glibtop)

# ----------- Solaris procfs support -------------
AC_ARG_WITH([procfs],
  [AS_HELP_STRING([--with-procfs],
                  [Use Solaris procfs to report CPU utilization @<:@default=check@:>@])],
  [if test -z "$with_procfs"; then with_procfs=yes; fi],
  [with_procfs=check])

if test "x$with_procfs" != "xno"; then
  AC_CHECK_HEADERS([procfs.h],
    [],
    [if test "$with_procfs" == "check"; then
       with_procfs=no
     else
       AC_MSG_FAILURE([Procfs requested but headers not found])
     fi])
     
  if test "$with_procfs" != "no"; then
    AC_DEFINE(HAVE_PROCFS, [1], [Procfs available])
    AC_SUBST([PROCFS_INCLUDE], [""])
    AC_SUBST([PROCFS_LIB], [""])
  fi
fi
AC_MSG_CHECKING(for procfs)
AC_MSG_RESULT($with_procfs)

AC_MSG_CHECKING(for CPU monitoring utility)
if test "$with_glibtop" != "no"; then
  if test "$with_procfs" != "no"; then
    AC_MSG_RESULT([glibtop procfs (using glibtop)])
  else
    AC_MSG_RESULT([glibtop])
  fi
  AM_CONDITIONAL(HAVE_GLIBTOP, true)
  AM_CONDITIONAL(HAVE_PROCFS, false)
  AC_DEFINE(HAVE_CPUMON, [1], [CPU monitoring available])
elif test "$with_procfs" != "no"; then
  AC_MSG_RESULT([procfs])
  AM_CONDITIONAL(HAVE_GLIBTOP, false)
  AM_CONDITIONAL(HAVE_PROCFS, true)
  AC_DEFINE(HAVE_CPUMON, [1], [CPU monitoring available])
else
  AM_CONDITIONAL(HAVE_GLIBTOP, false)
  AM_CONDITIONAL(HAVE_PROCFS, false)\
  AC_MSG_RESULT([none])
fi

# ----------- Shore-SM target dir -------------
AC_MSG_CHECKING(for Shore-MT)
AC_ARG_WITH(shore,
[AS_HELP_STRING([--with-shore=/path/to/shore],
[specify prefix directory for Shore-MT (required)])])

if test "x$with_shore" == x -o "x$with_shore" == xno -o "x$with_shore" == xyes; then
  SHORE_HOME=no
else
  SHORE_HOME=$with_shore
fi

AC_MSG_RESULT($SHORE_HOME)
AC_SUBST(SHORE_HOME)

# ----------- EOF Shore-SM target dir -------------



# ----------- Root of source tree -------------
# this specifies the root of the source tree; it's just the
# directory where ./configure runs, except on cygwin, which
# overrides this below
AC_MSG_CHECKING(Home directory)
KITS_HOME=`pwd`
AC_MSG_RESULT($KITS_HOME)
# ----------- EOF Root of source tree -------------


#AC_MSG_CHECKING([extra library needed for gethostbyname])
AC_SEARCH_LIBS([gethostbyname],
        [nsl],
        [AC_MSG_RESULT([$ac_cv_search])],
        [AC_MSG_ERROR([$ac_cv_search])])
                              



# ----------- Platform-specific code -------------


# Basic flags/libs every arch needs

# Careful, some of the flags below are mutually exclusive,for example cannot have
# the DEBUG and OPT flags together.
DBGSYMBOLS_FLAGS="-ggdb"
DEBUG_FLAGS="-g -O0"
PROFILE_FLAGS="-pg -O0"
OPT_FLAGS="-O3 -finline-limit=1200 -fomit-frame-pointer"
#OPT_DEBUG_FLAGS="-O3 -fno-inline"

CXXFLAGS="-Wall -Wcast-align -Wextra -Wfloat-equal -Wno-return-type \
	 -Wno-invalid-offsetof -Wno-conversion -Wno-system-headers -D_REENTRANT"

# Initial set of LDFLAGS
LDFLAGS="-rdynamic"
NORDYNAMIC_LDFLAGS="$LIBS -ldl -lm -lpthread -lrt"
dnl old_LDFLAGS="$LDFLAGS"
dnl LDFLAGS="$LDFLAGS -rdynamic"
dnl AC_MSG_CHECKING([If the compiler accepts -rdynamic])
dnl AC_LINK_IFELSE([int main(){}],
dnl        [AC_MSG_RESULT([rdynamic: yes])],
dnl        [LDFLAGS="$old_LDFLAGS" 
dnl        AC_MSG_RESULT([rdynamic: no])])


# $target is typically processor-vendor-os
case "$target" in
  # linux on i386
  *i*86*linux*)
    AC_MSG_RESULT(configuring for linux/i*86)
    ARCHOS=i386_LINUX
    OS=LINUX

    ## building with -m64    
    # CXXFLAGS="$CXXFLAGS -DARCH_LP64 -m64 "
    CXXFLAGS="$CXXFLAGS -UARCH_LP64 -m32 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 "
    ;;

  # linux on itanium? (not implem)
  *ia64*linux*)
    AC_MSG_RESULT(configuring for linux/ia64)
    AC_MSG_ERROR(not ready)
    ARCHOS=IA64_LINUX
    OS=LINUX

    CPU_FLAGS="-mtune=itanium2"
    ;;

  # linux on x86_64
  *x86_64*linux*)
    AC_MSG_RESULT(configuring for linux/x86_64)
    ARCHOS=x86_64_LINUX
    OS=LINUX

    CXXFLAGS="$CXXFLAGS -DARCH_LP64 -m64 "
    CPU_FLAGS="-mtune=native -march=native"    
    ;;

  # linux on power5
  *ppc*linux*)
    AC_MSG_RESULT(configuring for linux/ppc)
    AC_MSG_ERROR(not ready)
    ARCHOS=PPC_LINUX
    OS=LINUX

    CPU_FLAGS="-mcpu=power5"
    ;;


  # linux on power5-ppc64
  *powerpc64*linux*)
    AC_MSG_RESULT(configuring for linux/ppc64)
    AC_MSG_ERROR(not ready)
    ARCHOS=PPC64_LINUX
    OS=LINUX

    CPU_FLAGS="-mcpu=powerpc64"
    ;;

    
  # Sparc Solaris
  *sparc*sun*)
    AC_MSG_RESULT(configuring for sparc/sun)    
    ARCHOS=SPARC_SUN
    OS=SUN

    LDFLAGS="$NORDYNAMIC_LDFLAGS -lsocket -lkstat -L/usr/local/lib/sparcv9 -lmtmalloc"

    # On Sparc and CC we need to use completely different compilation flags
    CXXFLAGS="" 

    # optimize *a lot* flags - normal
    OPT_FLAGS="-xarch=sparcvis2 -m64 -features=extensions,zla -xthreadvar=no%dynamic -xdebugformat=stabs -xs -g0 -xO4 -mt -lpthread -library=stlport4"
      
    # regular debugging flags
    DEBUG_FLAGS="-xarch=sparcvis2 -m64 -features=extensions,zla -xdebugformat=stabs -mt -lpthread -library=stlport4"

    # debugging flags + datarace
    #    DEBUG_FLAGS="-xarch=sparcvis2 -m64 -features=extensions,zla -xdebugformat=stabs -xinstrument=datarace -mt -lpthread -library=stlport4"

    # debug symbols
    DBGSYMBOLS_FLAGS="-g -xs"

    ;;


  # x86 Cygwin
  *686*cygwin*)
    AC_MSG_RESULT(configuring for i686/cygwin)
    AC_MSG_ERROR(not ready)
    ARCHOS=x86_LINUX
    OS=LINUX

    ;;

 *)
    AC_MSG_ERROR([
      Unsupported platform $target -- sorry.
      SunOS/sparcv9, Linux/x86_64, Linux/i386 
      currently are supported platforms.
    ])
    ;;
esac


# Adding curses lib
if test "$ARCHOS" = "SPARC_SUN"; then
   LDFLAGS="$LDFLAGS -lcurses"
#else
#   LDFLAGS="$LDFLAGS -lncurses"
fi



# ----------- Compilation options -------------
# Get a list of compilation options (debug,profile,optimize)
MAX_COMP="maximum"
COMP_OPTIONS=""

# There are 3 supported options
# (1)  --enable-debug      : Compile for debugging (-g)
# (2)  --enable-profile    : Compile for profiling (e.g., -pg for oprofile)
# (3)  --enable-dbgsymbols : Compile with debug symbols (e.g., -ggdb)
#
# If none of them is enabled then the (default) compilation will be with
# the maximum optimizations possible (e.g., -O3 or -xO4)

# --- DEBUG ---
AC_MSG_CHECKING(whether will compile for debugging)
AC_ARG_ENABLE(debug, 
[  --enable-debug          Compile for debugging (-g)],
[case "${enableval}" in
  yes) debug=true ;;
  no)  debug=false ;;
  *) debug=false ;;
esac],[debug=false])

if test "$debug" = true
then 
     AC_MSG_RESULT(yes)
     COMP_OPTIONS="$COMP_OPTIONS debug"
     OPT_FLAGS=""
     MAX_COMP=""
else
     AC_MSG_RESULT(no)
     DEBUG_FLAGS=""
fi
# --- EOF DEBUG ---

# --- PROFILE ---
AC_MSG_CHECKING(whether will compile for profiling)
AC_ARG_ENABLE(profile, 
[  --enable-profile        Compile for profiling (-pg)],
[case "${enableval}" in
  yes) profile=true ;;
  no)  profile=false ;;
  *) profile=false ;;
esac],[profile=false])

if test "$profile" = true
then 
     AC_MSG_RESULT(yes)
     COMP_OPTIONS="$COMP_OPTIONS profile"
     OPT_FLAGS=""
     MAX_COMP=""
else
     AC_MSG_RESULT(no)
     PROFILE_FLAGS=""
fi
# --- EOF PROFILE ---


# --- DBGSYMBOLS ---
AC_MSG_CHECKING(whether will use debug symbols)
AC_ARG_ENABLE(dbgsymbols, 
[  --enable-dbgsymbols     With debug symbols],
[case "${enableval}" in
  yes) dbgsymbols=true ;;
  no)  dbgsymbols=false ;;
  *) dbgsymbols=false ;;
esac],[dbgsymbols=false])

if test "$dbgsymbols" = true -o "$debug" = true -o "$profile" = true
then 
     AC_MSG_RESULT(yes)
     COMP_OPTIONS="$COMP_OPTIONS dbgsymbols"
else
     AC_MSG_RESULT(no)
     DBGSYMBOLS_FLAGS=""
fi
# --- EOF DBGSYMBOLS ---

# Prepend to CXXFLAGS the selected {DEBUG,DBGSYMBOLS,PROFILE,OPT,CPU}_FLAGS

CXXFLAGS="$DEBUG_FLAGS $DBGSYMBOLS_FLAGS $PROFILE_FLAGS $OPT_FLAGS $CPU_FLAGS $CXXFLAGS $USERCXXFLAGS"

# ----------- Eof Compilation options ---------


# Create output directory
if ! test -d "obj/$ARCHOS"; then 
   mkdir -p "obj/$ARCHOS"
   AC_MSG_NOTICE([Creating output directory obj/$ARCHOS])
fi

AM_CONDITIONAL(SPARC_MACHINE, test "$ARCHOS" = "SPARC_SUN")
AM_CONDITIONAL(LINUX_OS, test "$OS" = "LINUX")

# ----------- EOF Platform-specific code -------------




# ----------- Shore-kits features -------------
# Give a list of features with the defaults
KITS_FEATURES=""

# Supported options:
# (1)  --enable-flusher     : defines CFG_FLUSHER
# (2)  --enable-qpipe       : includes QPipe files, (sets USE_QPIPE=1), defines CFG_QPIPE
# (3)  --enable-bt          : enables backtracing facility. defines CFG_BT
# (4)  --enable-simics      : adds the simics MAGIC instructions. defines CFG_SIMICS
# (5)  --enable-hacks       : enables the hacks (e.g., the padding in WH,DI of TPC-C, and the partitioned OL_IDX)
# (6)  --enable-vtune       : to pause/resume vtune within the program, (sets USE_VTUNE=1), defines CFG_VTUNE



# --- FLUSHER ---
AC_MSG_CHECKING(whether to enable FLUSHER/GROUPCOMMIT option)
AC_ARG_ENABLE(flusher, 
[  --enable-flusher        Enable FLUSHER/GROUPCOMMIT option],
[case "${enableval}" in
  yes) flusher=true ;;
  no)  flusher=false ;;
  *) flusher=true ;;
esac],[flusher=false])
AM_CONDITIONAL(USE_FLUSHER, test x$flusher = xtrue)

if test "$flusher" = true
then 
     AC_MSG_RESULT(yes)
     KITS_FEATURES="$KITS_FEATURES flusher"
     AC_MSG_WARN([FLUSHER enabled.])
     AC_DEFINE(CFG_FLUSHER, [1], [Flusher enabled])

     # Special case: warn about FLUSHER without ELR
     if test "$elr" = false
     then
        AC_MSG_WARN(FLUSHER without ERL)
     fi
else
     AC_MSG_RESULT(no)
fi
# --- EOF FLUSHER ---   


# --- CACHES (default==true) ---
AC_MSG_CHECKING(whether to use caches)
AC_ARG_ENABLE(caches, 
[  --enable-caches         Enable caches],
[case "${enableval}" in
  yes) caches=true ;;
  no)  caches=false ;;
  *) caches=true ;;
esac],[caches=true])
AM_CONDITIONAL(ADD_OPT_CACHES, test x$caches = xtrue)

if test "$caches" = true
then 
     AC_MSG_RESULT(yes)
     KITS_FEATURES="$KITS_FEATURES caches"
     AC_DEFINE(CFG_CACHES, 1, [Object caches enabled])
else
     AC_MSG_RESULT(no)
     AC_MSG_WARN([Used for debugging purposes])
fi
# --- EOF CACHES ---   


# --- SIMICS MAGIC INSTRUCTIONS ---
AC_MSG_CHECKING(whether to add the Simics magic instructions)
AC_ARG_ENABLE(simics, 
[  --enable-simics         Use Simics magic instructions],
[case "${enableval}" in
  yes) simics=true ;;
  no)  simics=false ;;
  *) simics=false ;;
esac],[simics=false])
AM_CONDITIONAL(USE_SIMICS, test x$simics = xtrue)

if test "$simics" = true
then 
     AC_MSG_RESULT(yes)
     KITS_FEATURES="$KITS_FEATURES simics"
     AC_MSG_WARN([Adding Simics magic instructions])
     AC_DEFINE(CFG_SIMICS, 1, [Using Simic magic instructions])
else
     AC_MSG_RESULT(no)
fi
# --- EOF SIMICS MAGIC INSTRUCTIONS ---   


# --- BACKTRACING ---
AC_MSG_CHECKING(whether to enable backtracing)
AC_ARG_ENABLE(bt, 
[  --enable-bt             Enable backtracing],
[case "${enableval}" in
  yes) bt=true ;;
  no)  bt=false ;;
  *) bt=false ;;
esac],[bt=false])
AM_CONDITIONAL(USE_BT, test x$bt = xtrue)

if test "$bt" = true
then 
     AC_MSG_RESULT(yes)
     KITS_FEATURES="$KITS_FEATURES backtracing"
     AC_MSG_WARN([Enabling backtracing])
     AC_DEFINE([CFG_BT], [1], [Backtracing enabled])
else
     AC_MSG_RESULT(no)
fi
# --- EOF BACKTRACING ---   



# --- HACKS ---
AC_MSG_CHECKING(whether to enable the hacks)
AC_ARG_ENABLE(hacks, 
[  --enable-hacks           Enable hacks],
[case "${enableval}" in
  yes) hacks=true ;;
  no)  hacks=false ;;
  *) hacks=false ;;
esac],[hacks=false])
AM_CONDITIONAL(USE_HACK, test x$hacks = xtrue)

if test "$hacks" = true
then 
     AC_MSG_RESULT(yes)
     KITS_FEATURES="$KITS_FEATURES hacks"
     AC_MSG_WARN([Enabling hacks])
     AC_DEFINE(CFG_HACK, 1, [Enabling various performance hacks])
else
     AC_MSG_RESULT(no)
fi
# --- EOF HACKS ---   


# --- QPIPE ---
AC_MSG_CHECKING(whether to enable qpipe)
AC_ARG_ENABLE(qpipe, 
[  --enable-qpipe          Enable QPipe],
[case "${enableval}" in
  yes) qpipe=true ;;
  no)  qpipe=false ;;
  *) qpipe=false ;;
esac],[qpipe=false])
AM_CONDITIONAL(USE_QPIPE, test x$qpipe = xtrue)

if test "$qpipe" = true
then 
     AC_MSG_RESULT(yes)
     KITS_FEATURES="$KITS_FEATURES qpipe"
     AC_MSG_WARN([Enabling QPipe])
     AC_DEFINE(CFG_QPIPE, 1, [Enable QPipe])
else
     AC_MSG_RESULT(no)
fi
# --- EOF QPIPE ---


# --- VTUNE ---
AC_MSG_CHECKING(whether to enable VTune)
AC_ARG_WITH([vtune],
  [AS_HELP_STRING([--with-vtune],
    [Specify location vtune install (empty means location is well-known)])],
  [AC_MSG_RESULT(yes)
    AC_DEFINE(CFG_VTUNE, 1, [Enabling VTune])
    KITS_FEATURES="$KITS_FEATURES vtune"
    if test "x$with_vtune" != "x" -a "x$with_vtune" != "xyes"; then
      AC_SUBST([VTUNE_INCLUDES], ["-I$with_vtune/include"])
      AC_SUBST([VTUNE_LIBS], ["-L$with_vtune/lib -L$with_vtune/lib/64 -L$with_vtune/lib64 -littnotify"])
    else
      AC_SUBST([VTUNE_LIBS], ["-littnotify"])
    fi
  ],
  [AC_MSG_RESULT(no)])
  
# --- EOF VTUNE ---


# ----------- EOF Shore-kit features -------------



# ----------------- finish up -------------------
# names of the variables that get substituted in files; for example,
# write @ARCHOS@ somewhere in a written file to get it substituted
AC_SUBST(ARCHOS)
AC_SUBST(GCC_COMPILER)
AC_SUBST(KITS_HOME)
AC_SUBST(KITS_VERSION_MAJOR)
AC_SUBST(KITS_VERSION_MINOR)
AC_SUBST(KITS_VERSION_REV)
AC_SUBST(KITS_VERSION)

AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)
    
# MY_AC_CONFIG_FILES(filename)
# do AC_CONFIG_FILES(filename, chmod a-w filename)
define([MY_AC_CONFIG_FILES],
[{
  if test -f [$1].in; then
    AC_CONFIG_FILES([$1], chmod a-w [$1])
  else
    true
    #echo "skipping [$1] because it's not in this distribution"
  fi
}])
define([MY_AC_CONFIG_EXE_FILES],
[{
  if test -f [$1].in; then
    AC_CONFIG_FILES([$1], [chmod a-w,a+x $1])
  else
    true
    #echo "skipping [$1] because it's not in this distribution"
  fi
}])
#MY_AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_HEADERS([include/kits-config.h])
AC_CONFIG_FILES([ Makefile ])

AC_OUTPUT()

# show the user what the variables have been set to
cat <<EOF | tee -a tmpcfg.txt

Shore Kits configuration:
  Version:         KITS_VERSION   $KITS_VERSION
  Home:            KITS_HOME      $KITS_HOME	
  Features:        KITS_FEATURES  $KITS_FEATURES
  Shore SM home:   SHORE_HOME     $SHORE_HOME
  Readline home:   READLINE_HOME  $READLINE_HOME
  CXX compiler:    CXX            $CXX
  CXX version:     CXX_VERSION    $KITS_CXX_VERSION
  Architecture/OS: ARCHOS         $ARCHOS
  Compilation:     COMP_OPTIONS   $MAX_COMP$COMP_OPTIONS
  CXXFLAGS         $CXXFLAGS
  LDFLAGS          $LDFLAGS
EOF
