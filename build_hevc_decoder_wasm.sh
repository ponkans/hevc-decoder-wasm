rm -rf dist/libffmpeg_265.wasm dist/libffmpeg_265.js
export TOTAL_MEMORY=67108864
export EXPORTED_FUNCTIONS="[ \
		'_init_decoder', \
		'_flush_decoder', \
		'_close_decoder', \
    '_decode_AnnexB_buffer', \
    '_main', \
    '_malloc', \
    '_free'
]"

echo "Running Emscripten..."
emcc decode_hevc.c ffmpeg/lib/libavcodec.a ffmpeg/lib/libavutil.a \
    -O2 \
    -I "ffmpeg/include" \
    -s WASM=1 \
    -s TOTAL_MEMORY=${TOTAL_MEMORY} \
   	-s EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS}" \
   	-s EXTRA_EXPORTED_RUNTIME_METHODS="['addFunction']" \
		-s RESERVED_FUNCTION_POINTERS=14 \
		-s FORCE_FILESYSTEM=1 \
    -o dist/libffmpeg_265.js

echo "Finished Build"
