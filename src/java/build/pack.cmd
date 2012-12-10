@echo off
set version=1.2.5-rc1

cd win32.win32.x86
zip -r nxmc-%version%-win32-x86.zip nxmc
mv nxmc-%version%-win32-x86.zip ..
cd ..

cd win32.win32.x86_64
zip -r nxmc-%version%-win32-x64.zip nxmc
mv nxmc-%version%-win32-x64.zip ..
cd ..

cd linux.gtk.x86
tar cvf nxmc-%version%-linux-gtk-x86.tar nxmc
gzip nxmc-%version%-linux-gtk-x86.tar
mv nxmc-%version%-linux-gtk-x86.tar.gz ..
cd ..

cd linux.gtk.x86_64
tar cvf nxmc-%version%-linux-gtk-x64.tar nxmc
gzip nxmc-%version%-linux-gtk-x64.tar
mv nxmc-%version%-linux-gtk-x64.tar.gz ..
cd ..

cd solaris.gtk.x86
tar cvf nxmc-%version%-solaris-gtk-x86.tar nxmc
gzip nxmc-%version%-solaris-gtk-x86.tar
mv nxmc-%version%-solaris-gtk-x86.tar.gz ..
cd ..

cd solaris.gtk.sparc 
tar cvf nxmc-%version%-solaris-gtk-sparc.tar nxmc
gzip nxmc-%version%-solaris-gtk-sparc.tar
mv nxmc-%version%-solaris-gtk-sparc.tar.gz ..
cd ..

cd macosx.cocoa.x86
tar cvf nxmc-%version%-macosx-cocoa-x86.tar nxmc
gzip nxmc-%version%-macosx-cocoa-x86.tar
mv nxmc-%version%-macosx-cocoa-x86.tar.gz ..
cd ..

cd macosx.cocoa.x86_64
tar cvf nxmc-%version%-macosx-cocoa-x64.tar nxmc
gzip nxmc-%version%-macosx-cocoa-x64.tar
mv nxmc-%version%-macosx-cocoa-x64.tar.gz ..
cd ..
