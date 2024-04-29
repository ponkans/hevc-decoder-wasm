let webglPlayer;
let preTiem,
  totalTime = 0,
  count = 0;
let fps = 30,
  totalSeconds = 10;

async function chooseH265File() {
  const [fileHandle] = await window.showOpenFilePicker();
  const file = await fileHandle.getFile();
  const AnnexBBuffer = await file.arrayBuffer();
  if (!AnnexBBuffer) return;

  init();

  let typedArray = new Uint8Array(AnnexBBuffer);
  let size = typedArray.length;

  // 大致计算每帧数据大小
  let chunkSize = Math.ceil(size / (totalSeconds * fps));

  let cacheBuffers = [];
  for (let i = 0; i < totalSeconds * fps; i++) {
    let start = i * chunkSize;
    let end = Math.min(start + chunkSize, size);
    let chunk = typedArray.slice(start, end);
    let cacheBuffer = Module._malloc(chunk.length);
    cacheBuffers.push(cacheBuffer);
    Module.HEAPU8.set(chunk, cacheBuffer);
  }

  for (let i = 0; i < totalSeconds * fps; i++) {
    (function (index) {
      setTimeout(() => {
        let cacheBuffer = cacheBuffers[index];
        Module._decode_AnnexB_buffer(cacheBuffer, chunkSize);
        Module._free(cacheBuffer);
      }, 0);
    })(i);
  }
}

function init() {
  const videoCallback = Module.addFunction(function (
    start,
    size,
    width,
    height
  ) {
    const cur = Date.now();
    if (!preTiem) {
      preTiem = cur;
    }
    const temp = cur - preTiem;
    totalTime += temp;
    preTiem = cur;
    count += 1;
    console.log("解码一帧平均时间:", totalTime / count);

    const u8s = Module.HEAPU8.subarray(start, start + size);
    const data = new Uint8Array(u8s);
    let obj = {
      data,
      width,
      height,
    };

    displayVideoFrame(obj);
  },
  "viiii");
  Module._init_decoder(videoCallback);
}

function displayVideoFrame(obj) {
  let data = new Uint8Array(obj.data);
  let width = obj.width;
  let height = obj.height;
  let yLength = width * height;
  let uvLength = (width / 2) * (height / 2);
  if (!webglPlayer) {
    const canvasId = "playCanvas";
    canvas = document.getElementById(canvasId);
    webglPlayer = new WebGLPlayer(canvas, {
      preserveDrawingBuffer: false,
    });
  }
  webglPlayer.renderFrame(data, width, height, yLength, uvLength);
}
