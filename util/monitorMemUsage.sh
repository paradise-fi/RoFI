#!/usr/bin/env bash

# Monitors peak memory usage. Measures only memory resident in the physical
# memory, ignores swapped pages

LOGFILE=$1
shift

echo '{}' > $LOGFILE

TFILE="$(mktemp /tmp/memUsage.XXXXXXXXX)" || exit 1

/usr/bin/time -f '%M\n%e\n%S\n%U' -o ${TFILE} $@

cat $LOGFILE | jq ". |= . + {
    \"peakMemoryUsage\": $(sed '1q;d' ${TFILE}),
    \"wallClockTime\": $(sed '2q;d' ${TFILE}),
    \"kernelTime\": $(sed '3q;d' ${TFILE}),
    \"userTime\": $(sed '4q;d' ${TFILE}),
}" | sponge $LOGFILE

rm ${TFILE}

