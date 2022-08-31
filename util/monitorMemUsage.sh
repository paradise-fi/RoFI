#!/usr/bin/env bash

# Monitors peak memory usage. Measures only memory resident in the physical
# memory, ignores swapped pages

LOGFILE=$1
shift

echo '{}' > $LOGFILE

TFILE="$(mktemp /tmp/memUsage.XXXXXXXXX)" || exit 1

/usr/bin/time -f '%M' -o ${TFILE} $@

cat $LOGFILE | jq ". |= . + { \"peakMemoryUsage\": $(cat ${TFILE}) }" | sponge $LOGFILE

rm ${TFILE}

