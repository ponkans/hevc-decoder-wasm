echo "Beginning Build:"
rm -r ffmpeg
mkdir -p ffmpeg
cd ../ffmpeg
rm -rf ffbuild/.config ffbuild/config.fate ffbuild/config.log ffbuild/config.mak ffbuild/config.sh
make clean
./configure --cc="emcc" --cxx="em++" --ar="emar" --ranlib="emranlib" --prefix=$(pwd)/../hevc-decoder-wasm/ffmpeg --target-os=none --arch=x86_32 --cpu=generic \
    --disable-avdevice \
    --disable-avformat \
    --disable-swresample \
    --disable-postproc \
    --disable-avfilter \
    --disable-programs \
    --disable-logging \
    --disable-everything \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-asm \
    --disable-doc \
    --disable-devices \
    --disable-network \
    --disable-hwaccels \
    --disable-parsers \
    --disable-bsfs \
    --disable-debug \
    --disable-protocols \
    --disable-indevs \
    --disable-outdevs \
    --disable-decoders \
    --disable-encoders \
    --disable-demuxers \
    --disable-muxers \
    --disable-filters \
    --disable-swscale \
    --enable-cross-compile \
    --enable-gpl \
    --enable-version3 \
    --enable-decoder=hevc \
    --enable-parser=hevc
make
make install

cd ../hevc-decoder-wasm
./build_hevc_decoder_wasm.sh