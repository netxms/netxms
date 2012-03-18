#!/bin/sh

usage() {
	echo "usage `basename $0` <dir> <lang> <verbose>";
	echo "dir: netxms source directory"
	echo "lang: two-letter abbreviation";
	echo "verbose: on|off "
}

compare() {
	local _dir="$1"
	local _lang="$2"
	local _verb="$3"
	local _name="$4" 
	
	for MFILE in `find "$_dir" -name "$_name" -print`; do
		BASENAME=`basename $MFILE`		
		PREFIX="${BASENAME%.properties}"
		DIR=`dirname $MFILE`
		LFILE=$DIR/$PREFIX"_$_lang.properties"
		if [ -f $LFILE ]; then
			LINES_M=`wc -l < $MFILE | tr -d '\n'`
			LINES_L=`wc -l < $LFILE | tr -d '\n'`
			if [ "$LINES_M" != "$LINES_L" ]; then
				echo "$DIR [$LINES_M:$LINES_L] ! $LFILE"
			else
				if [ "$_verb" == "on" ]; then
					echo "$DIR [$LINES_M:$LINES_L]"
				fi
			fi
		else
			echo "can't find $LFILE"
		fi
	done
}

if [ $# -lt 2 ]; then
	usage
	exit
fi

if [  $# -eq 3 ]; then
	case "$3" in
	on)
		VERB="$3"
		;;
	off)
		VERB="$3"
		;;
	*)
		usage
		exit
		;;
	esac
else
	usage
	exit
fi 

for F in messages bundle compare ; do
	compare "$1" "$2" "$VERB" "$F.properties"
done
