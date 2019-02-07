#!/bin/bash
# set -e

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
    local CURDIR=$1
    rm -rf $CURDIR
    mkdir -p $CURDIR
    pushd $CURDIR
    ln -s ../../urtorrent
    ln -s ../../$FILE_TO_TORRENT.torrent
    popd
}

get_dir_name(){
    local LEECH_OR_SEED=$1
    local INDEX=$2
    echo "$TESTDIR/$LEECH_OR_SEED-$INDEX"
}

make_seed_dir(){
    local INDEX=$1
    local CURDIR=$(get_dir_name seed $INDEX)
    _make_test_dir $CURDIR
    pushd $CURDIR
    ln -s ../../$FILE_TO_TORRENT 
    popd
}

make_leech_dir(){
    local INDEX=$1
    local CURDIR=$(get_dir_name leech $INDEX)
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
