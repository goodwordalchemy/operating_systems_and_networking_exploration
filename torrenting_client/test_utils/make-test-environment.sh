#!/bin/bash
# set -e

export N_SEEDS=1
export N_LEECHES=3

export SEED_PORT=8000
export LEECH_PORT=9000

export FILE_TO_TORRENT=lovely_photo.jpg
export TRACKER_URL=http://localhost:6969/announce

export TESTDIR=test_test_clients/

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

_make_test_dir(){
    CURDIR=$1
    rm -rf $CURDIR
    mkdir -p $CURDIR
    pushd $CURDIR
    ln -s ../../urtorrent
    ln -s ../../$FILE_TO_TORRENT.torrent
    popd
}

get_dir_name(){
    LEECH_OR_SEED=$1
    INDEX=$2
    echo "$TESTDIR/$LEECH_OR_SEED-$INDEX"
}

make_seed_dir(){
    INDEX=$1
    CURDIR=$(get_dir_name seed $INDEX)
    _make_test_dir $CURDIR
    pushd $CURDIR
    ln -s ../../$FILE_TO_TORRENT 
    popd
}

make_leech_dir(){
    INDEX=$1
    CURDIR=$(get_dir_name leech $INDEX)
    _make_test_dir $CURDIR
}

run_test_client(){
    local LEECH_OR_SEED=$1
    local INDEX=$2
    local CURDIR=$(get_dir_name $LEECH_OR_SEED $INDEX)

    if [ "$LEECH_OR_SEED" = "seed" ]; then
        local PORT=$SEED_PORT
    else
        local PORT=$LEECH_PORT
    fi

    echo leech or seed : $LEECH_OR_SEED :: port : $(($PORT + $INDEX))

    pushd $CURDIR
    ./urtorrent $(($PORT + $INDEX)) $FILE_TO_TORRENT.torrent > out.log 2>&1 
    popd
}

# main section
rm -rf $TESTDIR
make

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

wait
