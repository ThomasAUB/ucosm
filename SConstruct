#!python
import os

############################################################################################""
commonflags = ['-std=c++11']

debugcflags = commonflags + [] #['-W1', '-GX', '-EHsc', '-D_DEBUG', '/MDd']   #extra compile flags for debug
releasecflags = commonflags + [] #['-O2', '-EHsc', '-DNDEBUG', '/MD']         #extra compile flags for release

cpppaths = [os.path.join(os.getcwd(), 'src')] # maybe change the lib path to: include/uCoSM ?
libraries = ['cppunit']

buildroot = os.path.join(os.getcwd(), 'build')
############################################################################################""


#get the profile flag from the command line
#default to 'release' if the user didn't specify
profile = ARGUMENTS.get('profile', 'release')   #holds current profile

#check if the user has been naughty: only 'debug' or 'release' allowed
if not (profile in ['debug', 'release']):
   print ("Error: expected 'debug' or 'release', found: " + profile)
   Exit(1)

#tell the user what we're doing
print ('**** Compiling in ' + profile + ' profile...')

env = Environment()

env.Append( CPPPATH=cpppaths)

#make sure the sconscripts can get to the variables
Export('env', 'buildroot', 'profile', 'debugcflags', 'releasecflags', 'libraries')

#put all .sconsign files in one place
env.SConsignFile()

project = 'test'
SConscript('test/SConscript', exports=['project'])
