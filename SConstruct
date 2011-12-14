# -*- python -*-
#
# Setup our environment
#
# Do not change these
import glob, os, re, sys

# Note: This uses the build-variant feature in Scons, which is known to be 
# troublesome in Scons 0.98.5.  Scons 1.2.0 and 2.0.1 are known to work.

env = Environment()

xrd_dir = "/scratch/xrd/xrootd";
if os.environ.has_key('XRD_DIR'):
    xrd_dir = os.environ['XRD_DIR']
if not os.path.exists(xrd_dir):
    xrd_dir = "/home/ktl/xrootd"
if not os.path.exists(xrd_dir):
    print >> sys.stderr, "Could not locate xrootd base directory"
    Exit(1)
xrd_platform = None
platforms = ["x86_64_linux_26", "x86_64_linux_26_dbg", 
             "i386_linux26","i386_linux26_dbg"]
if os.environ.has_key('XRD_PLATFORM'):
    platforms.insert(0, os.environ['XRD_PLATFORM'])
for p in platforms:
    if os.path.exists(os.path.join(xrd_dir, "lib", p)):
        print "Using platform=", p
        xrd_platform = p
        break
if not xrd_platform:
    print >> sys.stderr, "Could not locate xrootd libraries"
    Exit(1)
env.Append(CPPPATH = [os.path.join(xrd_dir, "src")])
env.Append(LIBPATH = [os.path.join(xrd_dir, "lib", xrd_platform)])
print "xrd lib path is", os.path.join(xrd_dir,"lib",xrd_platform)

if os.environ.has_key('BOOST_DIR'):
    boost_dir = os.environ['BOOST_DIR']
    if not os.path.exists(boost_dir):
        print >> sys.stderr, "Could not locate Boost base directory (BOOST_DIR)"
        Exit(1)
    env.Append(CPPPATH = [os.path.join(boost_dir, "include")])
    env.Append(LIBPATH = [os.path.join(boost_dir, "lib")])

if os.environ.has_key('SSL_DIR'):
    ssl_dir = os.environ['SSL_DIR']
    if os.path.exists(ssl_dir):
        env.Append(LIBPATH=[ssl_dir])

if os.environ.has_key('MYSQL_DIR'):
    mysql_dir = os.environ['MYSQL_DIR']
    if os.path.exists(mysql_dir):
        env.Prepend(LIBPATH=[os.path.join(mysql_dir,"lib","mysql")])
        env.Prepend(CPPPATH=[os.path.join(mysql_dir,"include")])
        path64 = os.path.join(mysql_dir,"lib64","mysql")
        if os.path.exists(path64): env.Prepend(LIBPATH=[path64])
else:
    path64 = os.path.join("/usr","lib64","mysql")
    if os.path.exists(path64): env.Prepend(LIBPATH=[path64])

# Add lib64 for Redhat/Fedora
if os.path.exists("/usr/lib64"): env.Append(LIBPATH=["/usr/lib64"])
conf = Configure(env)

if not conf.CheckLib("ssl"):
    print >> sys.stderr, "Could not locate ssl"
    Exit(1)
if not conf.CheckLib("crypto"):
    print >> sys.stderr, "Could not locate crypto"
    Exit(1)

if not conf.CheckLibWithHeader("mysqlclient_r", "mysql/mysql.h", "c"):
    print >> sys.stderr, "Could not locate mysqlclient"
    Exit(1)
if not conf.CheckDeclaration("mysql_next_result", "#include <mysql/mysql.h>","c++" ):
    print >> sys.stderr, "mysqlclient too old. (check MYSQL_DIR)."
    Exit(1)
if not conf.CheckLibWithHeader("XrdSys", "XrdSfs/XrdSfsInterface.hh", "C++"):
    print >> sys.stderr, "Could not locate XrdSys"
    Exit(1)

if not conf.CheckLib("boost_regex-gcc34-mt", language="C++") \
        and not conf.CheckCXXHeader("boost/regex.hpp"):
    print >> sys.stderr, "Could not locate Boost headers"
    Exit(1)
if not (conf.CheckLib("boost_regex-gcc43-mt", language="C++") 
        or conf.CheckLib("boost_regex-gcc41-mt", language="C++") 
        or conf.CheckLib("boost_regex-gcc34-mt", language="C++") 
        ) and not conf.CheckLib("boost_regex", language="C++"):
    print >> sys.stderr, "Could not locate boost_regex library"
    Exit(1)
if not conf.CheckLib("boost_thread-gcc34-mt", language="C++") \
        and not conf.CheckLib("boost_thread-gcc41-mt", language="C++") \
        and not conf.CheckLib("boost_thread-mt", language="C++") \
        and not conf.CheckLib("boost_thread", language="C++"):
    print >> sys.stderr, "Could not locate boost_thread library"
    Exit(1)
if not conf.CheckLib("boost_signals-gcc34-mt", language="C++") \
        and not conf.CheckLib("boost_signals-gcc41-mt", language="C++") \
        and not conf.CheckLib("boost_signals", language="C++"):
    print >> sys.stderr, "Could not locate boost_signals library"
    Exit(1)
env = conf.Finish()

checkSinCosSource = """
    #include <math.h>
    int main(int argc, char **argv)
    {
        double s, c;
        sincos(0.5, &s, &c);
        return 0;
    }
    """

def CheckSinCos(context):
    context.Message('Checking for sincos()...')
    result = context.TryLink(checkSinCosSource, '.c')
    context.Result(result)
    return result

## Check for protobufs support
if (os.environ["PROTOC"] 
    and os.environ["PROTOC_INC"] 
    and os.environ["PROTOC_LIB"]):
    print "Protocol buffers using protoc=%s with lib=%s and include=%s" %(
        os.environ["PROTOC"], os.environ["PROTOC_INC"], 
        os.environ["PROTOC_LIB"])
else:
    print """Can't continue without Google Protocol Buffers.
Make sure PROTOC, PROTOC_INC, and PROTOC_LIB env vars are set.
e.g., PROTOC=/usr/local/bin/protoc 
      PROTOC_INC=/usr/local/include 
      PROTOC_LIB=/usr/local/lib"""
        

udfEnv = Environment()
if os.environ.has_key('MYSQL_SERVER_DIR'):
    mysql_server_dir = os.environ['MYSQL_SERVER_DIR']
    if os.path.isdir(mysql_server_dir):
        udfEnv.Prepend(CPPPATH=[os.path.join(mysql_server_dir,"include")])
udfEnv.Append(CPPDEFINES=["-D_GNU_SOURCE"])
conf = Configure(udfEnv, custom_tests = {'CheckSinCos' : CheckSinCos})
if not conf.CheckLibWithHeader("m", "math.h", "c"):
    print >> sys.stderr, "Could not locate libm"
    Exit(1)
if conf.CheckSinCos():
    conf.env.Append(CPPDEFINES=["-DQSERV_HAVE_SINCOS=1"])
if not conf.CheckHeader("mysql/mysql.h"):
    print >> sys.stderr, "Could not locate mysql"
    Exit(1)
udfEnv = conf.Finish()

# Describe what your package contains here.
env.Help("""
LSST Query Services worker package
""")

#
# Build/install things
#

## Build worker protocol
SConscript("proto/SConscript", exports={'env' : env})

## Build lib twice, with and without xrd
## Must invoke w/ variant_dir at top-level SConstruct.
envNoXrd = env.Clone(CCFLAGS=["-g","-DNO_XROOTD_FS"])
env.Append(CCFLAGS=['-g','-pedantic','-Wno-long-long'])
# Hack to workaround missing XrdSfs library in xrootd.
sfsObjs = map(lambda f: env.SharedObject(os.path.join(xrd_dir, "src", 
                                                      "XrdSfs", f)),
              ["XrdSfsCallBack.cc","XrdSfsNative.cc"])
env.Append(sfsObjs=sfsObjs)
envNoXrd.Append(sfsObjs=sfsObjs)
for bldDir, expEnv in [['bld',env], ['bldNoXrd',envNoXrd]]:
#for bldDir, expEnv in [['bld',env]]:
    try:
        VariantDir(bldDir, 'src')       
        SConscript("src/SConscript.lib", variant_dir=bldDir,
                   exports={'env' : expEnv, 'xrd_dir' : xrd_dir})
    except Exception, e:
        print >> sys.stderr, "%s: %s" % (os.path.join("src", "SConscript.lib"), e)


######################################################################
# Build UDFs
try:
    #raise Warning("Udf building DISABLED. Comment-out line in SConstruct to enable")
    SConscript("SConscript.udf", variant_dir='bldUdf', exports={'env': udfEnv})
except Exception, e:
    print >> sys.stderr, "%s: %s" % (os.path.join("udf", "SConscript"), e)


######################################################################
for d in Split("tests doc"): 
    if os.path.isdir(d):
        try:
            SConscript(os.path.join(d, "SConscript"), 
                       exports={'env' : env, 'envNoXrd' : envNoXrd})
        except Exception, e:
            print >> sys.stderr, "%s: %s" % (os.path.join(d, "SConscript"), e)
