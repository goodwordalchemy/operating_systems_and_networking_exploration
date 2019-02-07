#!/bin/bash

export N_SEEDS=1
export N_LEECHES=5

. ./test_utils/make-test-environment.sh

make_seed_dir 1
run_test_client seed 1 &
export SEED_PID=$!

for i in $(seq 1 $N_LEECHES);
do
    make_leech_dir $i
    run_test_client leech $i &
done

sleep 1

kill $SEED_PID

wait
