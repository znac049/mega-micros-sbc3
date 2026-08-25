#!/usr/bin/bash

if [[ "$#" -ne 1 ]]; then
    echo usage: $0 '<docname>'
    exit 1
fi

MD=$1
FNNAME=`basename $MD .md`
SRCFILE=$FNNAME.c

NUMSRC=`find . -name $SRCFILE | wc -l`
if [[ "$NUMSRC" -eq 0 ]]; then
    echo No source file found: $SRCFILE
    exit 1
fi

if [[ "$NUMSRC" -ne 1 ]]; then
    echo Too many source files found: $SRCFILE
    exit 1
fi

SRCPATH=`find . -name $SRCFILE`
TARGET=`dirname $SRCPATH`/$MD

if [[ -f $TARGET ]]; then
    echo $TARGET already exists
    exit 1
fi

INCLUDE=`dirname $SRCPATH | sed -e 's+./++'`
PROTO=`egrep "$FNNAME.* {" $SRCPATH | sed -e 's/ {/;/'`

echo NAME=$FNNAME
echo INCLUDE=$INCLUDE
echo PROTO=$PROTO

TEMPLATE=$MEGA_MICROS_DIR/src/templates/function.md

cp $TEMPLATE $TARGET
sed -i -e "s/==NAME==/$FNNAME/" -e "s/==INCLUDE==/$INCLUDE/" -e "s/==PROTO==/$PROTO/" $TARGET