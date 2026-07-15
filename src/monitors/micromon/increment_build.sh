#!/bin/bash

build=`grep 'static int MAGIC_BUILD_NUMBER' main.c | sed -e 's/.*= *//' -e 's/ *;//'`
new_build=`expr $build + 1`
sed -i.bak -e "s/\(static int MAGIC_BUILD_NUMBER *=\).*;\(.*\)$/\1 $new_build;\2/" main.c
