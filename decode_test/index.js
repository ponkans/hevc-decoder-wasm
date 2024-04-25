var webglPlayer, canvas, videoWidth, videoHeight, yLength, uvLength;
var LOG_LEVEL_JS = 0;
var LOG_LEVEL_WASM = 1;
var LOG_LEVEL_FFMPEG = 2;
var DECODER_H265 = 1;

var decoder_type = DECODER_H265;

function handleVideoFiles(files) {
    var file_list = files;
    var file_idx = 0;
    decode_seq(file_list, file_idx);
}

function setDecoder(type) {
    decoder_type = type;
}

function decode_seq(file_list, file_idx) {
    if (file_idx >= file_list.length)
        return;
    var file = file_list[file_idx];
    var start_time = new Date();

    var videoSize = 0;
    var videoCallback = Module.addFunction(function (start, size, width, height) {
      videoSize += size;
      console.log(`${width} * ${height}`, `pts: ${pts}`);
      const u8s = Module.HEAPU8.subarray(start, start + size);
      const data = new Uint8Array(u8s);
      var obj = {
        data,
        width,
        height
      };
      displayVideoFrame(obj);
    }, "viiii");

    var ret = Module._init_decoder(videoCallback);
    if(ret == 0) {
        console.log("openDecoder success");
    } else {
        console.error("openDecoder failed with error", ret);
        return;
    }

    var readerIndex = 0
    // var CHUNK_SIZE = 4096;
    var CHUNK_SIZE = 3007;
    var i_stream_size = 0;
    var filePos = 0;
    var totalSize = 0
    var pts = 0
    function run() {
      setTimeout(() => {
        var reader = new FileReader();
        reader.onload = function () {
          var typedArray = new Uint8Array(this.result);
          var size = typedArray.length;
          var cacheBuffer = Module._malloc(size);
          Module.HEAPU8.set(typedArray, cacheBuffer);
          totalSize += size;
          // console.log("[" + (++readerIndex) + "] Read len = ", size + ", Total size = " + totalSize)
  
          Module._decode_AnnexB_buffer(cacheBuffer, size);
          Module._free(cacheBuffer);
        };
        i_stream_size = read_file_slice(reader, file, filePos, CHUNK_SIZE);
        filePos += i_stream_size;
        if (i_stream_size > 0) {
          run();
        }
      }, 33);
    }
    run();

}

//从地址 start_addr 开始读取 size 大小的数据
function read_file_slice(reader, file, start_addr, size) {
    var file_size = file.size;
    var file_slice;

    if (start_addr > file_size - 1) {
        return 0;
    }
    else if (start_addr + size > file_size - 1) {
        file_slice = blob_slice(file, start_addr, file_size);
        reader.readAsArrayBuffer(file_slice);
        return file_size - start_addr;
    }
    else {
        file_slice = blob_slice(file, start_addr, start_addr + size);
        reader.readAsArrayBuffer(file_slice);
        return size;
    }
}

function blob_slice(blob, start_addr, end_addr) {
    if (blob.slice) {
        return blob.slice(start_addr, end_addr);
    }
    // compatible firefox
    if (blob.mozSlice) {
        return blob.mozSlice(start_addr, end_addr);
    }
    // compatible webkit
    if (blob.webkitSlice) {
        return blob.webkitSlice(start_addr, end_addr);
    }
    return null;
}

function displayVideoFrame(obj) {
    var data = new Uint8Array(obj.data);
    // 问题就在这里，没有返回宽高，导致 yuv 分量找不到位置，所以绿屏了
    var width = obj.width;
    var height = obj.height;
    var yLength = width * height;
    var uvLength = (width / 2) * (height / 2);
    if(!webglPlayer) {
        const canvasId = "playCanvas";
        canvas = document.getElementById(canvasId);
        webglPlayer = new WebGLPlayer(canvas, {
            preserveDrawingBuffer: false
        });
    }
    webglPlayer.renderFrame(data, width, height, yLength, uvLength);
}


// ipc.265 不要放上去
// debug 调试
// 读取文件，调用 decode 方法自己写。读取文件使用最新的 chrome 最新的 api，直接读取
// flush close 方法测试