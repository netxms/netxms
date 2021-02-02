#!/bin/sh

set -e

if [ ! -r zulu15.29.15-ca-jre15.0.2-macosx_x64.zip ]; then
   echo zulu15.29.15-ca-jre15.0.2-macosx_x64.zip is missing, download from 'https://www.azul.com/downloads/zulu-community/?&version=java-11-lts&os=macos&os-details=macOS&architecture=x86-64-bit&package=jre'
   exit 1
fi

if [ -z $1 ]; then
   version=`git describe|cut -d- -f1,2|tr - .`
else
   version=$1
fi

echo Version: $version

mkdir -p dist
rm -rf dist/* nxmc-${version}.dmg
cp -R ../../src/java/netxms-eclipse/Product/target/products/org.netxms.ui.eclipse.console.product/macosx/cocoa/x86_64/Eclipse.app dist/NetXMS\ Console.app
rm -rf dist/NetXMS\ Console.app/Contents/Eclipse/p2/org.eclipse.equinox.p2.*

cat Info.plist | sed "s,@version@,${version},g" > "dist/NetXMS Console.app/Contents/Info.plist"
cd dist/NetXMS\ Console.app/Contents/
unzip -x ../../../zulu15.29.15-ca-jre15.0.2-macosx_x64.zip >/dev/null
mv zulu15.29.15-ca-jre15.0.2-macosx_x64 jre
cd - >/dev/null

find dist/NetXMS\ Console.app -name \*.so -print0 | xargs -0 codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist
find dist/NetXMS\ Console.app -name \*.jnilib -print0 | xargs -0 codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist
find dist/NetXMS\ Console.app -name \*.dylib -print0 | xargs -0 codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist

cd dist/NetXMS\ Console.app/Contents/jre
codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements ../../../../entitlements.plist `find . -type f | xargs -n 1 file | grep Mach-O | cut -d: -f1`
cd -

7z x dist/NetXMS\ Console.app/Contents/Eclipse/plugins/org.eclipse.swt.cocoa.macosx.x86_64_*.jar \*.jnilib
codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist ./libswt-*
7z a dist/NetXMS\ Console.app/Contents/Eclipse/plugins/org.eclipse.swt.cocoa.macosx.x86_64_*.jar \*.jnilib
rm -f *.jnilib
codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist dist/NetXMS\ Console.app

mv dist/NetXMS\ Console.app dist/NetXMS\ Console\ \($version\).app

create-dmg --no-internet-enable --volname "NetXMS ${version}" --volicon dmg-icon.icns --window-size 500 320 --background dmg-background.png --icon "NetXMS Console ($version).app" 114 132 --app-drop-link 399 132 nxmc-${version}.dmg dist
codesign -s 'Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)' nxmc-${version}.dmg

#xcrun altool --notarize-app --primary-bundle-id "org.netxms.ui.eclipse.console.product" --username "alk@alk.lv" --password "@keychain:AC_PASSWORD" --asc-provider KFFD69X4L6 --file nxmc-${version}.dmg
