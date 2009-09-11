pushd ..
gcc -O2 squeeze.c -o squeeze
./squeeze demo.asc demosqz >& /dev/null
popd
./compile.sh
