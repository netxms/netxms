#!/bin/sh
# vim: ts=3 sw=3 expandtab

set -e

if [ `uname -p` = 'arm' ]; then
   arch=aarch64
   jre_basename=zulu17.42.19-ca-jdk17.0.7-macosx_aarch64
else
   arch=x86_64
   jre_basename=zulu17.42.19-ca-jdk17.0.7-macosx_x64
fi

if [ ! -r "${jre_basename}.zip" ]; then
   curl -o "${jre_basename}.zip" "https://cdn.azul.com/zulu/bin/${jre_basename}.zip"
fi

if [ -z $1 ]; then
   version=`git describe|cut -d- -f1,2|tr - .`
else
   version=$1
fi

echo Version: $version

mkdir -p dist
rm -rf dist/* nxmc-${version}-${arch}.dmg
cp -R ../../src/java/netxms-eclipse/Product/target/products/org.netxms.ui.eclipse.console.product/macosx/cocoa/${arch}/Eclipse.app dist/NetXMS\ Console.app
rm -rf dist/NetXMS\ Console.app/Contents/Eclipse/p2/org.eclipse.equinox.p2.*

cat Info.plist | sed "s,@version@,${version},g" > "dist/NetXMS Console.app/Contents/Info.plist"
cd dist/NetXMS\ Console.app/Contents/
unzip -x ../../../${jre_basename}.zip >/dev/null
mv "${jre_basename}" jre
cd - >/dev/null

find dist/NetXMS\ Console.app -name \*.so -print0 | xargs -0 codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist
find dist/NetXMS\ Console.app -name \*.jnilib -print0 | xargs -0 codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist
find dist/NetXMS\ Console.app -name \*.dylib -print0 | xargs -0 codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist

cd dist/NetXMS\ Console.app/Contents/jre
codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements ../../../../entitlements.plist `find . -type f | xargs -n 1 file | grep Mach-O | cut -d: -f1`
cd -

7zz x dist/NetXMS\ Console.app/Contents/Eclipse/plugins/org.eclipse.swt.cocoa.macosx.${arch}_*.jar \*.jnilib
codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist ./libswt-*
7zz a dist/NetXMS\ Console.app/Contents/Eclipse/plugins/org.eclipse.swt.cocoa.macosx.${arch}_*.jar \*.jnilib
rm -f *.jnilib
codesign --options=runtime --deep --force -s "Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)" -v --entitlements entitlements.plist dist/NetXMS\ Console.app

mv dist/NetXMS\ Console.app dist/NetXMS\ Console\ \(${version}\).app

create-dmg --no-internet-enable --volname "NetXMS ${version}" --volicon dmg-icon.icns --window-size 500 320 --background dmg-background.png --icon "NetXMS Console (${version}).app" 114 132 --app-drop-link 399 132 nxmc-${version}-${arch}.dmg dist
codesign -s 'Developer ID Application: Raden Solutions, SIA (KFFD69X4L6)' nxmc-${version}-${arch}.dmg

#xcrun altool --notarize-app --primary-bundle-id "org.netxms.ui.eclipse.console.product" --username "alk@alk.lv" --password "@keychain:AC_PASSWORD" --asc-provider KFFD69X4L6 --file nxmc-${version}-${arch}.dmg
