#!/bin/sh

set -e

version=`grep AC_INIT ../../configure.ac|cut -d'[' -f3|cut -d']' -f1`

rm -rf dist/* nxmc-${version}.dmg
~/Development/yoursway-eclipse-osx-repackager/EclipseOSXRepackager out/eclipse dist/NetXMS\ Console.app

cat Info.plist | sed "s,@version@,${version},g" > "dist/NetXMS Console.app/Contents/Info.plist"
~/Development/yoursway-create-dmg/create-dmg --volname NetXMS --volicon ~/Development/netxms/packages/osx/dmg-icon.icns --window-size 500 320 --background ~/Development/netxms/packages/osx/dmg-background.png --icon "NetXMS Console.app" 114 132 --app-drop-link 399 132 nxmc-${version}.dmg dist
