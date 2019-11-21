#!/bin/sh

infile=original/tears_of_steel_1080p.mov

outdir=segm_out
outtempl=${outdir}/seg-%03d.mkv

[ -d "${outdir}" ] || mkdir "${outdir}"
ffmpeg -i "${infile}" \
	-vcodec copy \
	-an \
	-f segment \
	-segment_list_flags live \
	-segment_list ${outdir}/list.m3u8 \
	"${outtempl}"

awk '!/#EXTINF/' ${outdir}/list.m3u8 > ${outdir}/dummy_playlist.m3u8
