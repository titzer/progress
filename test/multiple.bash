#!/bin/bash

NUM=${NUM:=4}
T=/tmp/progress-test/multiple
CMD="/Users/titzer/Code/progress/bin/multi-progress "

mkdir -p $T

function exec4() {
   $1 3<$T/fifo.0 4<$T/fifo.1 5<$T/fifo.2 6<$T/fifo.3
}

i=0
while [ $i -lt $NUM ]; do
    FIFO=$T/fifo.$i
    rm -f $FIFO
    mkfifo $FIFO

    echo FooBarMcDuck $i > $FIFO &
    i=$(($i + 1))
done

exec4 $CMD
