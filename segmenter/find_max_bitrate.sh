#!/bin/bash

# Simple script to find max bit rate of the segments to set the throttle speed
# at the sender appropriately.
# Assumes outdir has a playlist of the segments with the timing info and
# the actual segments

outdir=segm_out

    i=0
    for fn in $(awk '/^[^#]/' ${outdir}/list.m3u8) ; do
	filesize=$(stat -c %s "${outdir}/${fn}")
	sizearr[$i]=$filesize
	i=$((i+1))
    done

    echo "${#sizearr[@]} filesizes"
    
    i=0
    for fn in $(awk '/^#EXTINF/' ${outdir}/list.m3u8) ; do	
	fn=${fn:8:-1}
	timearr[$i]=$fn
	i=$((i+1))
    done
    echo "${#timearr[@]} playout durations"

    n=${#sizearr[@]}

    i=0
    for size in "${sizearr[@]}" ; do
	t=${timearr[$i]}
	rate=$(bc -l <<<"$size*8/$t/1000/1000")
	ratearr[$i]=$rate
	i=$((i+1))
    done

    printf '%s\n' "${ratearr[@]}" | awk '$1 > m || NR == 1 { m = $1} END { print m " Mbps is the max segment rate"}'
    


		
