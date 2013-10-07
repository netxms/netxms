#!/bin/bash

source ./vars.sh

script_name="${0#*/}"

if [ $# -eq 0 ]
then
cat <<USAGE
    Usage:
        $script_name <artifact-name>
        
USAGE
    exit 1
fi

[ -e "./default.conf" ] && . "./default.conf" && \
    echo "default.conf found. Reading."

for artifact in $@
do
    mkdir -p "$artifact"
    cd "$artifact"
    descfile="$artifact.dsc"
    [ -e "$descfile" ] || echo -e \
        "Description goes in here." > "$descfile"
    prepfile="$artifact.prep"
    [ -e "$prepfile" ] || echo -e \
        "# Preparation phase\n%setup -q" > "$prepfile"
    buildfile="$artifact.bld"
    [ -e "$buildfile" ] || echo -e \
        "# Build phase\n%configure\nmake" > "$buildfile"
    checkfile="$artifact.chk"
    [ -e "$checkfile" ] || echo -e \
        "# Checks\nmake check" > "$checkfile"
    insfile="$artifact.ins"
    [ -e "$insfile" ] || echo -e \
        "# Install phase\nmake install" > "$insfile"
    prefile="$artifact.pre"
    [ -e "$prefile" ] || echo -e \
        "# Pre-install phase" > "$prefile"
    preunfile="$artifact.upre"
    [ -e "$preunfile" ] || echo -e \
        "# Pre-uninstall phase" > "$preunfile"
    postfile="$artifact.pst"
    [ -e "$postfile" ] || echo -e \
        "# Post-install phase" > "$postfile"
    postunfile="$artifact.upst"
    [ -e "$postunfile" ] || echo -e \
        "# Post-uninstall phase" > "$postunfile"
    listfile="$artifact.lst"
    [ -e "$listfile" ] || echo -e \
        "# List of files to include\n%{_bindir}/*\n%{_libdir}/*" > "$listfile"
    reqfile="$artifact.req"
    [ -e "$reqfile" ] || echo -e \
        "# List packages required to run the artifact\n# Ex: zlib >= 1.0" > "$reqfile"

    RPM_NAME="$artifact"
    [ -z "$RPM_VER" ]       && RPM_VER='1.0'
    [ -z "$RPM_SUMMARY" ]   && RPM_SUMMARY='Short summary'
    [ -z "$RPM_LICENSE" ]   && RPM_LICENSE='GPL'
    [ -z "$RPM_SRC" ]       && RPM_SRC='http://www.example.com/src/%{name}-%{version}.tar.gz'
    [ -z "$RPM_URL" ]       && RPM_URL='http://www.example.com/john'
    [ -z "$RPM_VENDOR" ]    && RPM_VENDOR='John Himself'
    [ -z "$RPM_PACKAGER" ]  && RPM_PACKAGER='John Doe'
    [ -z "$RPM_MAIL" ]      && RPM_MAIL='john.doe@example.com'
    
    conffile="$artifact.conf"
    [ -e "$conffile" ] || \
cat <<PROPERTIES > "$conffile"
RPM_NAME='$RPM_NAME'
RPM_VER='$RPM_VER'
RPM_SUMMARY='$RPM_SUMMARY'
RPM_REL='$RPM_REL'
RPM_LICENSE='$RPM_LICENSE'
RPM_SRC='$RPM_SRC'
RPM_URL='$RPM_URL'
RPM_VENDOR='$RPM_VENDOR'
RPM_PACKAGER='$RPM_PACKAGER'
RPM_MAIL='$RPM_MAIL'
PROPERTIES

    logfile="$artifact.log"
    [ -e "$logfile" ] || \
cat <<CHANGELOG > "$logfile"
* `date '+%a %b %d %Y'` $RPM_PACKAGER <$RPM_MAIL> $RPM_VER`[ -z "$RPM_REL" ] || echo -n "-$RPM_REL"`
- Initial RPM build
CHANGELOG

    echo "Artifact $artifact created."
    cd ..
done
