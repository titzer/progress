#!/bin/bash

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

cd $DIR
if [ $# -gt 0 ]; then
    TESTS=$*
else
    TESTS=*.in
fi

T=/tmp/$USER/progress-test/

mkdir -p $T

PROGRESS=${PROGRESS:=$DIR/../bin/progress.host}

for f in $TESTS; do
    echo "##+$f"
    OUT=$T/$(basename $f)
    OUT=${OUT/in/out}
    $PROGRESS < $f > $OUT
    diff ${f/in/out} $OUT > $OUT.diff
    if [ "$?" = 0 ]; then
	echo "##-ok"
    else
	echo "##-fail: $OUT.diff"
    fi
done
