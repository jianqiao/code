#!/bin/sh
# makedef.sh
# Creates an appropriate xsb.def file, given the object
# files in XSB/config/<configname>/saved.o

SRCDIR=/afs/cs.wisc.edu/u/j/i/jianqiao/Desktop/code/XSB
CONFIGURATION=x86_64-unknown-linux-gnu
CONFIG_TAG=-mt
SRC_CONFIG_PREFIX=/afs/cs.wisc.edu/u/j/i/jianqiao/Desktop/code/XSB/config/x86_64-unknown-linux-gnu-mt
OBJS=$@
DEFFILE=out.def
TMPFILE=out.def.tmp

echo "Creating .def file for ${CONFIGURATION} in ${SRC_CONFIG_PREFIX}/saved.o"
echo "OBJS=${OBJS}" 
echo EXPORTS > ${DEFFILE}
nm ${OBJS} | grep " [TC] " | sed '/ _/s//  /' | \
    awk '{print $3}' | grep @ > ${TMPFILE}
for i in `cat ${TMPFILE}`; do
    echo $i >> ${DEFFILE}
    echo _$i = $i >> ${DEFFILE}
done
rm -f out.def.tmp >/dev/null 2>&1
