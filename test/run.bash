#!/bin/bash

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

PROGRESS=${PROGRESS:=$DIR/../bin/progress.host}

echo "##+check PROGRESS=$PROGRESS"
if [ -x $PROGRESS ]; then
    echo "##-ok"
else
    echo "##-fail: not executable"
    exit 1
fi

cd $DIR
if [ $# -gt 0 ]; then
    TESTS=$*
else
    TESTS=*.in
fi

T=/tmp/$USER/progress-test/

mkdir -p $T

DEBUG=${DEBUG:=0}

function debug() {
    if [ $DEBUG != 0 ]; then
	echo "$@"
    fi
}

function check_diff() {
    EXPECT=$1
    OUT=$2
    diff $EXPECT $OUT > $OUT.diff
    if [ "$?" = 0 ]; then
	echo "##-ok"
    else
	echo "##-fail: $OUT.diff"
    fi
}

# Serial tests in multiple modes, determined by the .out files
for f in $TESTS; do
    test=$(basename $f)
    test=${test/.in/}
    base=${f/.in/}
    debug "==> $f"

    regex=".*-(.*).out"
    for expfile in ${base}*.out; do
	debug "  ==> expf: $expfile"
	args=""
	if [[ $expfile =~ $regex ]]; then
	    args="${BASH_REMATCH[1]}"
	fi
	debug "  ==> args: $args"

	echo "##+progress $args < $f"
	OUT=$T/$B.$args.out
	DIFF=$T/$B.$args.diff
	$PROGRESS $args < $f > $OUT
	check_diff $expfile $OUT
    done
done



# Parallel tests
echo "##+progress p 3<one_ok.in 4<one_fail.in"
expfile=p2.out
OUT=$T/p2.out
$PROGRESS p 3<one_ok.in 4<one_fail.in > $T/p2.out
check_diff $expfile $OUT

echo "##+progress p 3<p51.in 4<one_fail.in 5<m15.in"
expfile=p3.out
OUT=$T/p3.out
$PROGRESS p 3<p51.in 4<one_fail.in 5<m15.in > $T/p3.out
check_diff $expfile $OUT

