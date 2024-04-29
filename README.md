## 简介
H265 FFmpeg wasm 软解

### 参考文章
[Web 端 H265 wasm 软解入门](https://juejin.cn/spost/7362547971060367398)

### 运行流程
+ mkdir hevc-box
+ cd hevc-box
+ git clone https://git.ffmpeg.org/ffmpeg.git
+ cd ffmpeg
+ git checkout -b 4.1 origin/release/4.1
+ cd ../
+ git clone https://github.com/ponkans/hevc-decoder-wasm.git
+ cd hevc-decoder-wasm 
+ ./build_hevc_decoder.sh
+ http-server 
+ http://127.0.0.1:8080/decode_test