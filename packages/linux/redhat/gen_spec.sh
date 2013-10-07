#!/bin/bash

script_name="${0#*/}"

function print_usage {
cat <<USAGE
    Usage:
        $script_name options
    
    Try \`$script_name -h\` for help
    
USAGE
    exit 1
}

function print_help {
cat <<HELP
        $script_name is a script for creating specification files 
    needed to build RPM packages.

    Available options:        
        -n name
            Set the artifacts's name
            
        -ver version
            Set the artifact's version
                    
        -sum summary
            Provide the artifact's short description
            
        -rel release
            Set artifact's release number (defaults to 1)        
                    
        -lic license
            Provide the artifact's license
                    
        -src url
            Provide the path to the artifact's source location
            
        -url url
            Provide the path to the artifact's documentation location
            
        -vend vendor's name
            Set the artifact's vendor's name
            
        -pkr packager's name
            Set the artifact's packager's name
            
        -conf path to the conf file
            Provide the path to the artifact's configuration
        file (note that all other provided artifact options will override
        the ones specified in the configuration file)        
                    
        -dsc path to dsc file
            Provide the path to the artifact's description file
            
        -prep path to prep file
            Provide the path to the artifact's preparation phase file
                    
        -bld path to bld file
            Provide the path to the artifact's build phase file
                    
        -chk path to chk file
            Provide the path to the artifact's check phase file
                    
        -ins path to ins file
            Provide the path to the artifact's installation phase file
                                
        -pre path to pre file
            Provide the path to the artifact's pre-installation phase file
                                
        -upre path to upre file
            Provide the path to the artifact's pre-uninstallation phase file
            
        -pst path to pst file
            Provide the path to the artifact's post-installation phase file
                                
        -upst path to upst file
            Provide the path to the artifact's post-uninstallation phase file
                    
        -lst path to lst file
            Provide the path to the artifact's file list file
                    
        -log path to log file
            Provide the path to the artifact's changelog file
        
        -req path to req file
            Provide the path to the artifact's requirements file
            
        -o dir
            Set the output path for the spec file
        
        -h, --help
            Print this help and exit
            
HELP
    exit 1
}

[ $# -eq 0 ] && print_usage

while [ $# -gt 0 ]
do
    case "$1" in
        -h | --help)
            print_help;;
    
        -conf)
            [ ! -z "$2" ] && PROPFILE="$2" || shift
            shift 2;;
    
        -n)
            [ ! -z "$2" ] && IN_RPM_NAME="$2" || shift
            if [ -e "$IN_RPM_NAME/$IN_RPM_NAME.conf" ]
            then
                while :
                do
                    read -p "Property file found in $IN_RPM_NAME directory. Read it? [y/n] " yn
                    case $yn in
                        Y | y) PROPFILE="$IN_RPM_NAME/$IN_RPM_NAME.conf"; break;;
                        N | n) break;;
                        *) echo "Please answer with \"y\" for yes or \"n\" for no"
                    esac
                done
                [ -z "$PROPFILE" ] && break
            fi
            shift 2;;
            
        -ver)
            [ ! -z "$2" ] && IN_RPM_VER="$2" || shift
            shift 2;;
                    
        -sum)
            [ ! -z "$2" ] && IN_RPM_SUMM="$2" || shift
            shift 2;;
            
        -rel)
            [ ! -z "$2" ] && IN_RPM_REL="$2" || shift
            shift 2;;
            
        -lic)
            [ ! -z "$2" ] && IN_RPM_LICENSE="$2" || shift
            shift 2;;
         
        -src)
            [ ! -z "$2" ] && IN_RPM_SRC="$2" || shift
            shift 2;;
            
        -url)
            [ ! -z "$2" ] && IN_RPM_URL="$2" || shift
            shift 2;;
        
        -vend)
            [ ! -z "$2" ] && IN_RPM_VENDOR="$2" || shift
            shift 2;;
                
        -pkr)
            [ ! -z "$2" ] && IN_RPM_PACKAGER="$2" || shift
            shift 2;;
            
        -dsc)
            [ ! -z "$2" ] && DESCFILE="$2" || shift
            shift 2;;
          
        -prep)
            [ ! -z "$2" ] && PREPFILE="$2" || shift
            shift 2;;
            
        -bld)
            [ ! -z "$2" ] && BUILDFILE="$2" || shift
            shift 2;;
            
        -chk)
            [ ! -z "$2" ] && CHECKFILE="$2" || shift
            shift 2;;
            
        -ins)
            [ ! -z "$2" ] && INSFILE="$2" || shift
            shift 2;;
                        
        -pre)
            [ ! -z "$2" ] && PREFILE="$2" || shift
            shift 2;;        
                    
        -upre)
            [ ! -z "$2" ] && PREUNFILE="$2" || shift
            shift 2;;
            
        -pst)
            [ ! -z "$2" ] && POSTFILE="$2" || shift
            shift 2;;        
                    
        -upst)
            [ ! -z "$2" ] && POSTUNFILE="$2" || shift
            shift 2;;
            
        -lst)
            [ ! -z "$2" ] && LISTFILE="$2" || shift
            shift 2;;
            
        -log)
            [ ! -z "$2" ] && LOGFILE="$2" || shift
            shift 2;;
            
        -req)
            [ ! -z "$2" ] && REQFILE="$2" || shift
            shift 2;;
            
        -o)
            if [ -d "$2" ] 
            then
                [ ! -z "$2" ] && OUTDIR="$2"
            else
                "Not a directory: $2!"
                exit 1
            fi
            shift 2;;
            
        *)
            echo "Unknown option: $1"
            exit 1
    esac
done

[ -z "$PROPFILE" ] || source "$PROPFILE"

[ -z "$IN_RPM_NAME" ]     || RPM_NAME="$IN_RPM_NAME"
[ -z "$IN_RPM_VER" ]      || RPM_VER="$IN_RPM_VER"
[ -z "$IN_RPM_SUMM" ]     || RPM_SUMM="$IN_RPM_SUMM"
[ -z "$IN_RPM_REL" ]      || RPM_REL="$IN_RPM_REL"
[ -z "$IN_RPM_LICENSE" ]  || RPM_LICENSE="$IN_RPM_LICENSE"
[ -z "$IN_RPM_SRC" ]      || RPM_SRC="$IN_RPM_SRC"
[ -z "$IN_PRM_URL" ]      || PRM_URL="$IN_PRM_URL"
[ -z "$IN_PRM_VENDOR" ]   || PRM_VENDOR="$IN_PRM_VENDOR"
[ -z "$IN_PRM_PACKAGER" ] || PRM_PACKAGER="$IN_PRM_PACKAGER"

[ -z "$RPM_NAME" ]    && echo "No name provided! Exiting." && exit 1
[ -z "$RPM_VER" ]     && echo "No version provided! Exiting." && exit 1
[ -z "$RPM_SUMMARY" ] && echo "No summary provided. Exiting." && exit 1
[ -z "$RPM_SRC" ]     && echo "No source provided. Exiting." && exit 1
[ -z "$RPM_LICENSE" ] && echo "No license info provided, setting to GPL." \
    && RPM_LICENSE='GPL'

path="$RPM_NAME/$RPM_NAME"
[ -z "$DESCFILE" ]   && DESCFILE="$path.dsc"
[ -z "$PREPFILE" ]   && PREPFILE="$path.prep"
[ -z "$BUILDFILE" ]  && BUILDFILE="$path.bld"
[ -z "$CHECKFILE" ]  && CHECKFILE="$path.chk"
[ -z "$INSFILE" ]    && INSFILE="$path.ins"
[ -z "$PREFILE" ]    && PREFILE="$path.pre"
[ -z "$PREUNFILE" ]  && PREUNFILE="$path.upre"
[ -z "$POSTFILE" ]   && POSTFILE="$path.pst"
[ -z "$POSTUNFILE" ] && POSTUNFILE="$path.upst"
[ -z "$LISTFILE" ]   && LISTFILE="$path.lst"
[ -z "$LOGFILE" ]    && LOGFILE="$path.log"
[ -z "$REQFILE" ]    && REQFILE="$path.req"
unset path

[ -z "$OUTDIR" ] && OUTDIR="."
if [ ! -d "$OUTDIR" ]
then
    echo "No such directory: $OUTDIR!"
    exit 1
fi

SPECFILE="$OUTDIR/$RPM_NAME.spec"

cat <<SPEC > "$SPECFILE"
Name:       $RPM_NAME
Version:    $RPM_VER
Release:    $RPM_REL%{?dist}
Summary:    $RPM_SUMMARY

License:    $RPM_LICENSE
Source:     $RPM_SRC
$([ -z "$RPM_URL" ] || echo "\
URL:        $RPM_URL";
[ -z "$RPM_VENDOR" ] || echo "\
Vendor:     $RPM_VENDOR";
[ -z "$RPM_PACKAGER" ] || echo "\
Packager:   $RPM_PACKAGER";
[ -e "$REQFILE" ] && while read line; do
[ "${line::1}" != '#' ] && echo "\
Requires:   $line" 
done < "$REQFILE")

$([ -e "$DESCFILE" ] && echo -e "%description\n$(< "$DESCFILE")\n";
[ -e "$PREPFILE" ]   && echo -e "%prep\n$(< "$PREPFILE")\n";
[ -e "$BUILDFILE" ]  && echo -e "%build\n$(< "$BUILDFILE")\n";
[ -e "$CHECKFILE" ]  && echo -e "%check\n$(< "$CHECKFILE")\n";
[ -e "$INSFILE" ]    && echo -e "%install\n$(< "$INSFILE")\n";
[ -e "$PREFILE" ]    && echo -e "%pre\n$(< "$PREFILE")\n";
[ -e "$PREUNFILE" ]  && echo -e "%preun\n$(< "$PREUNFILE")\n";
[ -e "$POSTFILE" ]   && echo -e "%post\n$(< "$POSTFILE")\n";
[ -e "$POSTUNFILE" ] && echo -e "%postun\n$(< "$POSTUNFILE")\n";
[ -e "$LISTFILE" ]   && echo -e "%files\n$(< "$LISTFILE")\n";
[ -e "$LOGFILE" ]    && echo -e "%changelog\n$(< "$LOGFILE")\n")
SPEC

echo "Specfile written to $SPECFILE" 
