@echo off
set version=1.1.1

cd win32.win32.x86
zip -r nxmc-%version%-win32-x86.zip nxmc
mv nxmc-%version%-win32-x86.zip ..
cd ..

cd win32.win32.x86_64
zip -r nxmc-%version%-win32-x64.zip nxmc
mv nxmc-%version%-win32-x64.zip ..
cd ..

cd linux.gtk.x86
tar cvf nxmc-%version%-linux-x86.tar nxmc
gzip nxmc-%version%-linux-x86.tar
mv nxmc-%version%-linux-x86.tar.gz ..
cd ..

cd linux.gtk.x86_64
tar cvf nxmc-%version%-linux-x64.tar nxmc
gzip nxmc-%version%-linux-x64.tar
mv nxmc-%version%-linux-x64.tar.gz ..
cd ..

cd solaris.gtk.x86
tar cvf nxmc-%version%-solaris-x86.tar nxmc
gzip nxmc-%version%-solaris-x86.tar
mv nxmc-%version%-solaris-x86.tar.gz ..
cd ..

cd solaris.gtk.sparc 
tar cvf nxmc-%version%-solaris-sparc.tar nxmc
gzip nxmc-%version%-solaris-sparc.tar
mv nxmc-%version%-solaris-sparc.tar.gz ..
cd ..
