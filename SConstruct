#
# SConstruct for building trajectory
#
# Usage:
#       scons [-j 4] [-Q] [debug=1|profile=1] [objdir=<path>] smartmet_trajectory.a|lib
#
# Notes:
#       The three variants share the same output and object file names;
#       changing from one version to another will cause a full recompile.
#       This is intentional (instead of keeping, say, out/release/...)
#       for compatibility with existing test suite etc. and because normally
#       different machines are used only for debug/release/profile.
#
# Windows usage:
#	Run 'vcvars32.bat' or similar before using us, to give right
#	env.var. setups for Visual C++ 2008 command line tools & headers.
#

import os.path

Help(""" 
    Usage: scons [-j 4] [-Q] [debug=1|profile=1] [objdir=<path>] [prefix=<path>] smartmet_trajectory.a|lib
    
    Or just use 'make release|debug|profile', which point right back to us.
""") 

Decider("MD5-timestamp") 

DEBUG=      int( ARGUMENTS.get("debug", 0) ) != 0
PROFILE=    int( ARGUMENTS.get("profile", 0) ) != 0
RELEASE=    (not DEBUG) and (not PROFILE)     # default

OBJDIR=     ARGUMENTS.get("objdir","obj")
PREFIX=     ARGUMENTS.get("prefix","/usr/")

env= Environment()

LINUX=  env["PLATFORM"]=="posix"
OSX=    env["PLATFORM"]=="darwin"
WINDOWS= env["PLATFORM"]=="win32"

#
# SCons does not pass env.vars automatically through to executing commands.
# On Windows, we want it to get them all (Visual C++ 2008).
#
if WINDOWS:
    env.Replace( ENV= os.environ )

env.Append( CPPPATH= [ "./include" ] )

if WINDOWS: 
    if env["CC"]=="cl":
        env.Append( CXXFLAGS= ["/EHsc"] )
else:
    env.Append( CPPDEFINES= ["UNIX"] )
    env.Append( CXXFLAGS= [
        # MAINFLAGS from orig. Makefile ('-fPIC' is automatically added by SCons)
        #
	"-std=c++0x",
        "-fPIC", 
        "-Wall", 
        "-Wno-unused-parameter",
#       "-Wno-variadic-macros",
	    
	    # DIFFICULTFLAGS from orig. Makefile (flags that cause some choking,
	    # would be good but not used)
	    #
	    #"-Weffc++",
	    #"-Wredundant-decls",
	    #"-Wshadow",
	    #"-Woverloaded-virtual",
	    #"-Wctor-dtor-privacy",
	    #"-Wold-style-cast",
	    #"-pedantic",
    ] )

BOOST_POSTFIX=""
BOOST_PREFIX=""

if WINDOWS:
    # Installed from 'boost_1_35_0_setup.exe' from BoostPro Internet page.
    #
    BOOST_INSTALL_PATH= "D:/Boost/1_35_0"
    env.Append( CPPPATH= [ BOOST_INSTALL_PATH ] )
    env.Append( LIBPATH= [ BOOST_INSTALL_PATH + "/lib" ] )
    if DEBUG:
        BOOST_POSTFIX= "-vc90-gd-1_35"
    else:
        BOOST_POSTFIX= "-vc90-1_35"
        BOOST_PREFIX= "lib"
    env.Append( LIBS= [ BOOST_PREFIX+"boost_iostreams"+BOOST_POSTFIX,
                        BOOST_PREFIX+"boost_date_time"+BOOST_POSTFIX ] )

if WINDOWS:
    env.Append( CPPPATH= [ "../newbase/include" ] )
    env.Append( LIBPATH= [ "../newbase" ] )
    env.Append( CPPPATH= [ "../smarttools/include" ] )
    env.Append( LIBPATH= [ "../smarttools" ] )

elif LINUX:
    # newbase and smarttools from system install
    #
    env.Append( CPPPATH= [ PREFIX+"/include/mysql" ] )
    env.Append( CPPPATH= [ PREFIX+"/include/smartmet" ] )
    env.Append( CPPPATH= [ PREFIX+"/include/smartmet/newbase" ] )
    env.Append( CPPPATH= [ PREFIX+"/include/smartmet/smarttools" ] )

env.Append( LIBS= [ "smartmet_smarttools",
	            "smartmet_newbase",
		    "smartmet_fminames",
		    "smartmet_macgyver",
		    "boost_filesystem-mt",
		    "boost_iostreams-mt",
		    "boost_locale-mt",
		    "boost_program_options-mt",
		    "boost_regex-mt",
		    "boost_thread-mt",
		    "boost_system-mt",
		    "mysqlpp",
		    "bz2",
		    "rt"
		    ] )

env.ParseConfig("pkg-config --libs icu-i18n")

# Debug settings
#
if DEBUG:
    if WINDOWS:
        if env["CC"]=="cl":
            env.AppendUnique( CPPDEFINES=["_DEBUG","DEBUG"] )
            # Debug multithreaded DLL runtime, no opt.
            env.AppendUnique( CCFLAGS=["/MDd", "/Od"] )
            # Each obj gets own .PDB so parallel building (-jN) works
            env.AppendUnique( CCFLAGS=["/Zi", "/Fd${TARGET}.pdb"] )
    else:
        env.Append( CXXFLAGS=[ "-O0", "-g", "-Werror",
    
            # EXTRAFLAGS from orig. Makefile (for 'debug' target)
            #
	    # "-ansi",  DO NOT USE THIS UNTIL c++11 is supported
            "-Wcast-align",
            "-Wcast-qual",
            "-Winline",
            "-Wno-multichar",
            "-Wno-pmf-conversions",
            "-Woverloaded-virtual",
            "-Wpointer-arith",
            "-Wredundant-decls",
            "-Wsign-promo",
            "-Wwrite-strings",
        ] )

#
# Release settings
#
if RELEASE or PROFILE:
    if WINDOWS:
        if env["CC"]=="cl":
            # multithreaded DLL runtime, reasonable opt.
            env.AppendUnique( CCFLAGS=["/MD", "/Ox"] )
    else:
        env.Append( CPPDEFINES="NDEBUG",
                    CXXFLAGS= ["-O2",
 
            # RELEASEFLAGS from orig. Makefile (for 'release' and 'profile' targets)
            #
            "-Wuninitialized",
        ] )


#
# Profile settings
#
if PROFILE:
    if WINDOWS:
        { }     # TBD
    else: 
        env.Append( CXXFLAGS="-g -pg" )


#---
# Exceptions to the regular compilation rules (from orig. Makefile)
#
# Note: Samples show 'env.Replace( OBJDIR=... )' to be able to use separate
#       object dir, but this did not work.    -- AKa 15-Sep-2008
#
#env.Replace( OBJDIR=OBJDIR )
#env.Library( "smartmet_trajectory", Glob("source/*.cpp") )

objs= []

env.Append( LIBS= [ "smartmet_newbase", "smartmet_smarttools" ] )
if not WINDOWS:
    env.Append( CPPDEFINES= "_REENTRANT" )


for fn in Glob("source/*.cpp"): 
	s= os.path.basename( str(fn) )
	obj_s= OBJDIR+"/"+ s.replace(".cpp","")
	
	objs += env.Object( obj_s, fn )

# Make just the static lib (at least it should be default for just 'scons')

env.Library( "smartmet_trajectory", objs )

# Make main programs

env.Program( "qdtrajectory", [ "qdtrajectory.cpp", "libsmartmet_trajectory.a" ] );
