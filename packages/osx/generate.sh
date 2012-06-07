#!/bin/sh

cp Info.plist "dist/NetXMS Console.app/Contents/"
/Users/alk/Development/yoursway-create-dmg/create-dmg --volname NetXMS --volicon /Users/alk/Development/netxms/packages/osx/dmg-icon.icns --window-size 500 320 --background /Users/alk/Development/netxms/packages/osx/dmg-background.png --icon "NetXMS Console.app" 114 132 --app-drop-link 399 132 nxmc-1.2.1.dmg dist
