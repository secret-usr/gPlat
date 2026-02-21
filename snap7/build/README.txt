for older versions (prior 1.4.3) please refer to the repository at
https://sourceforge.net/projects/snap7/files/

Folders

bin          - Library output directory divided by os
temp         - Intermediate objects/temp files, can be safety emptied...

linux        - linux makefile directory
mips-linux   - linux MIPS makefile directory
osx          - OSX makefile directory
bsd          - BSD makefile directory
solaris      - Solaris makefiles directory
windows      - Windows Visual Studio solutions, directory are divided by compilers

BUILD UNIX (Linux, BSD, OSX, MIPS, SOLARIS)

Enter in the OS folder then

Linux, MIPS: 
make [clean|all|install]

OSX, BSD:
gmake [clean|all|install]

Solaris 32 bit using Oracle Solaris Studio
gmake -f i386_solaris_cc.mk [clean|all|install]

Solaris 64 bit using Oracle Solaris Studio
gmake -f x86_64_solaris_cc.mk [clean|all|install]

Solaris 32 bit using GNU
gmake -f i386_solaris_gcc.mk [clean|all|install]

Solaris 64 bit using GNU 
gmake -f x86_64_solaris_gcc.mk [clean|all|install]


BUILD WINDOWS

Open the Solution file (windows/VSXXXX/VSXXXX.sln) with Visual Studio (Community Edition is fine) and build.
