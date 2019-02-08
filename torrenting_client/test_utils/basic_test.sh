#!/bin/bash

export N_SEEDS=1
export N_LEECHES=3

. ./test_utils/make-test-environment.sh

for i in $(seq 1 $N_SEEDS);
do
    make_seed_dir $i
    run_test_client seed $i &
done

for i in $(seq 1 $N_LEECHES);
do
    make_leech_dir $i
    run_test_client leech $i &
done

watch_pieces
