#!/bin/bash

export N_SEEDS=1
export N_LEECHES_BEFORE=5
export N_LEECHES_AFTER=5

. ./test_utils/make-test-environment.sh

make_seed_dir 1
run_test_client seed 1 &
export SEED_PID=$!

for i in $(seq 1 $N_LEECHES_BEFORE);
do
    make_leech_dir $i
    run_test_client leech $i &
done

sleep 1
kill $SEED_PID


for i in $(seq 1 $N_LEECHES_AFTER);
do
    my_index=$(($i + $N_LEECHES_BEFORE))
    make_leech_dir $my_index
    run_test_client leech $my_index &
done

watch_pieces
