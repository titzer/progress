#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: parallel-progress <cmd> <inputs>"
    echo "Environment variables: "
    echo "  PROGRESS_ARGS=<args> arguments to progress"
    echo "  BATCHING=<num> processes <inputs> in batches"
    echo "  PARALLEL=<num> sets the amount of parallelism"
    exit 1
fi

T=/tmp/$USER/parallel-progress
mkdir -p $T

PROCESSORS=$(getconf _NPROCESSORS_ONLN)
BATCHING=${BATCHING:=1}
PARALLEL=${PARALLEL:=$PROCESSORS}

function run_batched() {
    local cmd=$1
    shift

    local i=1
    while [ $i -le $# ]; do
	local args=${@:$i:$BATCHING}
        $cmd $args
	i=$(($i + $BATCHING))
    done
}

function run_progress() {
    local cmd=$1
    shift
    
    if [[ $PARALLEL -gt 1 ]]; then
	# run in parallel mode
	local chunk=$(expr $# / $PARALLEL)
	if [ $chunk = 0 ]; then
	    chunk=1
	fi
	
	local i=1
	local f=3
	local progress_cmd="progress p$PROGRESS_ARGS "
	while [ $i -le $# ]; do
	    # divide the input up into chunks of 1 / p size
	    files=${@:$i:$chunk}
	    fifo=$T/fifo.$f
	    rm -f $fifo
	    mkfifo $fifo
	    # run command on chunk and pipe to fifo
	    if [ $chunk = 1 ]; then
		$cmd $files > $fifo &
	    else
		run_batched "$cmd" $files > $fifo &
	    fi
	    progress_cmd="$progress_cmd $f<$fifo"
	    i=$(($i + $chunk))
	    f=$(($f + 1))
	done
	# combine all fifos using the progress command
	echo $progress_cmd > $T/progress.sh
	chmod 755 $T/progress.sh
	bash $T/progress.sh
    else
	# run in serial, possibly batched, mode
	run_batched "$cmd" "$@" | progress $PROGRESS_ARGS
    fi
}

cmd=$1
shift
run_progress "$cmd" "$@"
pkill -P $$
