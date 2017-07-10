#!/bin/bash

SUBDIRS="include src example example/performance"
FILETYPES="*.cc *.h"
ASTYLE_BIN="astyle"
ASTYLE="${ASTYLE_BIN} -A2 -HtUwpj -M80 -c -s4 --pad-header --align-pointer=type "

if test -f ${ASTYLE_BIN}
then
    echo "use astyle"
else
    echo "astyle not found!!!"
fi

for d in ${SUBDIRS}
do
    echo "astyle format subdir: $d "
    for t in ${FILETYPES}
    do
        for file in $d/$t
        do
            echo ">>>>>>>>>> format file: $file "
            if test -f $file
            then
               ${ASTYLE} $file
               rm -f ${file}.orig
            fi
        done
    done
done

