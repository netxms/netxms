#include "..\..\..\build\netxms-build-tag.iss"

AppName=NetXMS Agent
AppVerName=NetXMS Agent {#VersionString}
AppVersion={#VersionString}
VersionInfoVersion={#BaseVersion}.{#BuildNumber}
VersionInfoTextVersion={#VersionString}
AppPublisher=Raden Solutions
AppPublisherURL=http://www.radensolutions.com
AppCopyright=© 2003-2019 Raden Solutions
AppSupportURL=http://www.netxms.org
AppUpdatesURL=http://www.netxms.org
DefaultDirName=C:\NetXMS
DefaultGroupName=NetXMS Agent
DisableDirPage=auto
DisableProgramGroupPage=auto
WizardStyle = modern
AllowNoIcons=yes
Compression=lzma2
LZMANumBlockThreads=4
SolidCompression=yes
LanguageDetectionMethod=none
LicenseFile=..\..\..\copying
CloseApplications=no
SignTool=signtool
