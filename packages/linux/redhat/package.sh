#!/bin/bash

source ./vars.sh

script_dir="$(pwd)"

function cleanup {
    cd "$script_dir"
    [ "$dont_cleanup" ] || ./clean.sh
}

trap "cleanup" EXIT

function usage {
cat <<USAGE
    Usage:
        ${0#*/} options
        
    Available options:
        -ver ver
            Version of the artifact to build
            
        -a artifact's name
            Build the specified artifact. By default, 
        artifacts will be created based on the artifact
        directories in the current path
        
        -dc
            Don't cleanup
            
        -h, --help
            Print this help and exit
            
USAGE
    exit 1
}

function check_ver {
    [ $# -ne 2 ] && return 1
    
    "$1" &> /dev/null
    [ "$?" -eq 127 ] && \
        echo "You don't seem to have $1 installed!" && \
        return 1
        
    [[ "$($1 --version)" =~ ([0-9].[0-9]*.[0-9]*) ]] && \
        ver="${BASH_REMATCH[1]}" || return 1
        
    compare_ver "$ver" "$2"
    return $?
}

function compare_ver {
    (IFS='.'; ver=($1); req_ver=($2)
    ver_length="${#ver[@]}"
    for (( i=0; i < ver_length; i++ ))
    do
        [ "${ver[$i]}" -gt "${req_ver[$i]}" ] && return 0
        [ "${ver[$i]}" -eq "${req_ver[$i]}" ] || return 1
    done)
}

[ $# -lt 1 ] && usage

check_ver rpmbuild 4.10.1 || exit 1

while [ $# -gt 0 ]
do
    case $1 in
        -h | --help)
            usage;;
            
        -ver)
            [ ! -z "$2" ] && ver="$2" || shift
            shift 2;;
    
        -a)
            [ ! -z "$2" ] && artifacts=("${artifacts[@]}" "$2") || shift
            shift 2;;
        
        -dc)
            dont_cleanup=true
            shift;;
            
        *)
            echo "Unknown option: $1"
            exit 1;;
    esac
done

[ -z "$ver" ] && echo "No version provided!" && exit 1

if [ -z "$artifacts" ]
then
    for file in `ls`
    do
        [ -d "$file" ] && \
        if [ -e "$file/$file.conf" ]
        then
            echo "* Artifact $file found, adding to the artifact list."
            artifacts=("${artifacts[@]}" "$file")
        else
            echo "$file - not an artifact, skipping."
        fi
    done
    echo
fi

[ -z "$artifacts" ] && echo "No artifacts to build." && exit 1

$(./init.sh)

source="netxms-$ver"
source_archive="$source.tar.gz"
web_archive="http://www.netxms.org/download/archive/$source_archive"
top_dir="$script_dir/$DIR_ROOT"
source_dir="$top_dir/$DIR_SOURCE"
spec_dir="$top_dir/$DIR_SPEC"
rpm_dir="$top_dir/$DIR_RPMS"
build_root_dir="$top_dir/$DIR_BUILDROOT"
build_root="$build_root_dir/$source"
build_dir="$top_dir/$DIR_BUILD"

if [ -e "$source_archive" ]
then
    echo "$source_archive found in the current directory."
    cp -f "./$source_archive" "$source_dir"
else
    echo "$source_archive isn't found in the current directory."    
    wget &> /dev/null
    wget_exit="$?"
    if [ "$wget_exit" -ne 127 ]
    then
        echo -e "Downloading from the web.\n"
        wget -N -P "$source_dir" "$web_archive"
        [ "$?" -ne 0 ] && exit 1
    else    
        $(curl &> /dev/null);
        if [ "$?" -ne 127 ] 
        then
            echo -e "Downloading from the web.\n"
            cd "$source_dir" && curl -R -O "$web_archive"
            [ "$?" -ne 0 ] && exit 1
        else
            echo "You don't seem to have a download manager installed."
            echo "Supported managers are wget and curl."
            exit 1
        fi
    fi
    
    unset wget_exit
fi

echo -n "Extracting sources..."
tar -xf "$source_dir/$source_archive" -p -C "$build_dir"
if [ $? -ne 0 ]
then
    echo -e \
        "\nExtraction failed!"
    exit 1
else
    echo "OK."
fi

cd "$build_dir/$source"

if [ !$(compare_ver "$ver" 1.2.10) ]
then
    #Macro hacks
    sed -i \
        -e 's;-DPREFIX=\(L\?\)["\${}a-zA-Z_]*;-DPREFIX=\1\\\\\\"/usr\\\\\\";' \
        -e 's;-DDATADIR=\(L\?\)["\${}a-zA-Z_]*;-DDATADIR=\1\\\\\\"/usr/share/netxms\\\\\\";' \
        -e 's;-DBINDIR=\(L\?\)["\${}a-zA-Z_]*;-DBINDIR=\1\\\\\\"/usr/bin\\\\\\";' \
        -e 's;-DLIBDIR=\(L\?\)["\${}a-zA-Z_]*;-DLIBDIR=\1\\\\\\"/usr/lib\\\\\\";' \
        -e 's;-DPKGLIBDIR=\(L\?\)["\${}a-zA-Z_]*;-DPKGLIBDIR=\1\\\\\\"/usr/lib/netxms\\\\\\"";' \
        configure.ac
fi

echo -n "Configuring sources..."
./reconf &> /dev/null
./configure --with-runtime-prefix=/usr --prefix="$build_root/usr" --enable-unicode --with-server \
    --with-odbc --with-sqlite --with-pgsql --with-mysql --with-agent &> /dev/null
if [ $? -ne 0 ]
then
    echo -e \
        "\nConfiguration failed, make sure you have all the"\
        "required dev packages installed!"
    exit 1
else
    echo "OK."
fi

echo -n "Building sources..."
make &> /dev/null 
if [ $? -ne 0 ]
then
    echo -e \
        "\nBuild process failed, make sure you have all the"\
        "required dev packages installed!"
    exit 1
else
    echo "OK."
fi

echo -n "Installing binaries..."
make install &> /dev/null
if [ $? -ne 0 ]
then
    echo -e \
        "\nInstallation failed!"
    exit 1
else
    echo "OK."
fi

echo

mkdir -p "$build_root/etc/init.d"
cp "$build_dir/$source/contrib/startup/redhat/netxmsd" "$build_root/etc/init.d"
cp "$build_dir/$source/contrib/startup/redhat/nxagentd" "$build_root/etc/init.d"

find "$build_root" -name *.la -exec rm -f {} \;

cd "$build_root/usr/lib"
for nsm in netxms/*.nsm
do
    libname="${nsm##*/}"
    libname="libnsm_${libname%.*}.so"
    ln -sf "$nsm" "$libname"
    unset nsm libname
done
for ddr in netxms/dbdrv/*.ddr
do
    libname="${ddr##*/}"
    libname="libnxddr_${libname%.*}.so"
    ln -sf "$ddr" "$libname"
    unset ddr libname
done
cd "$script_dir"

for art in ${artifacts[@]}
do
    echo "Building artifact $art:"
    specfile="$spec_dir/$art.spec"
    yes | ./gen_spec.sh -n "$art" -ver "$ver" -o "$spec_dir" &> /dev/null
    [ $? -ne 0 ] && exit 1
    rpmbuild --noclean --define "_topdir `pwd`/$DIR_ROOT" \
        --buildroot="$build_root" -bb "$specfile" &> /dev/null
    if [ $? -eq 0 ]
    then
        mkdir -p rpms
        art_rpm="$art-$ver.`uname -m`.rpm"
        install -m 755 -t "$DIR_OUT" "$rpm_dir/$art_rpm" &> /dev/null
        echo "$art built successfully."
        echo -e "$art_rpm saved to ./$DIR_OUT/\n"
    else
        echo "$art: build failed!"
        exit 1
    fi
    unset specfile art art_rpm
done
