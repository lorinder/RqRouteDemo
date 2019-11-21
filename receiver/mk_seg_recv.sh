#!/bin/sh

#
# Script to generate the seg_recv/ folder which contains a copy of the
# m3u8 playlist as well as all the segment files as fifos.
#

set -e

origin=segments/
seg_recv=seg_recv/

if ! [ -d "${origin}" ] ; then
	echo "Error:  Origin directory \"${origin}\" not found." 1>&2
	exit 1
fi

rm -Rf "${seg_recv}"
mkdir "${seg_recv}"

cp "${origin}/dummy_playlist.m3u8" "${seg_recv}"
if true ; then
	for fn in $(awk '/^[^#]/' ${seg_recv}/dummy_playlist.m3u8) ; do
		mkfifo "${seg_recv}/${fn}"
	done
fi
