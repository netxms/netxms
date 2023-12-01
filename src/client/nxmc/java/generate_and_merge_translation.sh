#!/bin/sh
directory="src/main/resources/po"

xgettext --language=Java --from-code=UTF-8 --no-wrap -ktrc -ktr -kmarktr -ktrn:1,2 -o messages.pot $(find . -name '*.java')
if [ -d "$directory" ]; then
    for file in "$directory"/*.po; do
        if [ -f "$file" ]; then
            # Extract file name without extension
            echo "$file translation merge: "
            filename=$(basename -- "$file")
            msgmerge --no-wrap --no-fuzzy-matching -U $file messages.pot 
        fi
    done
else
    echo "Directory not found!"
fi

rm messages.pot