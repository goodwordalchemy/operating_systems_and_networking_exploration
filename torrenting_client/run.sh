export WDIR=$1
export PORT=$2

make
pushd $WDIR
./urtorrent $PORT lovely_photo.jpg.torrent
popd
