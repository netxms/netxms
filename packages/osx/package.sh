#!/bin/sh

set -e

if [ ! -r zulu11.33.15-ca-jre11.0.4-macosx_x64.zip ]; then
   echo zulu11.33.15-ca-jre11.0.4-macosx_x64.zip is missing, download from 'https://www.azul.com/downloads/zulu-community/?&version=java-11-lts&os=macos&os-details=macOS&architecture=x86-64-bit&package=jre'
   exit 1
fi

if [ -z $1 ]; then
   version=`grep AC_INIT ../../configure.ac|cut -d'[' -f3|cut -d']' -f1`
else
   version=$1
fi

mkdir -p dist
rm -rf dist/* nxmc-${version}.dmg
cp -R ../../src/java/netxms-eclipse/Product/target/products/org.netxms.ui.eclipse.console.product/macosx/cocoa/x86_64/Eclipse.app dist/NetXMS\ Console.app

cat Info.plist | sed "s,@version@,${version},g" > "dist/NetXMS Console.app/Contents/Info.plist"
cd dist/NetXMS\ Console.app/Contents/
unzip -x ../../../zulu11.33.15-ca-jre11.0.4-macosx_x64.zip >/dev/null
mv zulu11.33.15-ca-jre11.0.4-macosx_x64 jre
cd - >/dev/null
codesign --deep --force -s 'Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)' -v 'dist/NetXMS Console.app'
create-dmg --volname "NetXMS ${version}" --volicon ~/Development/netxms/netxms/packages/osx/dmg-icon.icns --window-size 500 320 --background ~/Development/netxms/netxms/packages/osx/dmg-background.png --icon "NetXMS Console.app" 114 132 --app-drop-link 399 132 nxmc-${version}.dmg dist
codesign -s 'Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)' nxmc-${version}.dmg
