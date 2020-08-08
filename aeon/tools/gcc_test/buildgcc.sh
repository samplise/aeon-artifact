
if [ $# -eq 0 ]; then
  echo "Need the version of gcc to build as the parameter e.g. 4.7.2"
  exit
fi

mirror="http://gcc.petsads.us/releases/"
gcc_ver=$1
base_name=gcc-$1
build_dir=build-$gcc_ver
install_dir=/scratch/chuangw/opt/$base_name

echo "Downloading from mirror" $mirror$1 "..."
wget $mirror$base_name/$base_name.tar.bz2
if [ $? -ne 0 ]; then
  echo "Can't download the source tarball"
  exit
fi
echo "Decompressing..."
bunzip2 $base_name.tar.bz2
tar xf $base_name.tar

echo "Building" $gcc_ver "..."
mkdir $build_dir
cd $build_dir
../$base_name/configure --prefix=$install_dir --build=x86_64-pc-linux-gnu --host=x86_64-pc-linux-gnu --target=x86_64-pc-linux-gnu
RETVAL=$?
if [ $RETVAL -eq 0 ]; then
  make -j8 all
  RETVAL2=$?
  if [ $RETVAL -eq 0 ]; then
    make install
  fi
fi
