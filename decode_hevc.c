#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*YUV420PBuffer)(uint8_t* yuv_buffer, int size, int width,
                              int height);

typedef enum ErrorCode {
  kErrorCode_Success = 0,
  kErrorCode_FFmpeg_Error = 1
} ErrorCode;

YUV420PBuffer YUV420P_buffer_callback = NULL;

const AVCodec* codec;
AVCodecParserContext* parser_context;
AVCodecContext* codec_context = NULL;
AVPacket* pkt;
AVFrame* frame;

ErrorCode init_decoder(YUV420PBuffer callback) {
  ErrorCode ret = kErrorCode_Success;

  do {
    codec = avcodec_find_decoder(AV_CODEC_ID_H265);
    if (!codec) {
      ret = kErrorCode_FFmpeg_Error;
      break;
    }

    parser_context = av_parser_init(codec->id);
    if (!parser_context) {
      ret = kErrorCode_FFmpeg_Error;
      break;
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
      ret = kErrorCode_FFmpeg_Error;
      break;
    }

    /* 多线程开启配置: 配置 4 个线程进行解码 */
    // codec_context->thread_count = 4;

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
      ret = kErrorCode_FFmpeg_Error;
      break;
    }

    frame = av_frame_alloc();

    /* 指定格式为 YUV420 */
    frame->format = AV_PIX_FMT_YUV420P;

    if (!frame) {
      ret = kErrorCode_FFmpeg_Error;
      break;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
      ret = kErrorCode_FFmpeg_Error;
      break;
    }

    YUV420P_buffer_callback = callback;
  } while (0);

  return ret;
}

void AVFrame_to_yuv_buffer(AVFrame* frame) {
  int frame_size =
      av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
  if (frame_size < 0) {
    return;
  }

  uint8_t* yuv_buffer = (uint8_t*)av_malloc(frame_size);
  if (!yuv_buffer) {
    return;
  }

  /* frame 数据按照 yuv420 格式填充到 yuv_buffer */
  int ret = av_image_copy_to_buffer(
      yuv_buffer, frame_size, (const uint8_t**)frame->data, frame->linesize,
      frame->format, frame->width, frame->height, 1);
  if (ret < 0) {
    av_free(yuv_buffer);
    return;
  }


  YUV420P_buffer_callback(yuv_buffer, frame_size, frame->width, frame->height);
  av_free(yuv_buffer);
}

static ErrorCode decode_AVPacket(AVCodecContext* dec_ctx, AVFrame* frame,
                                 AVPacket* pkt) {
  ErrorCode res = kErrorCode_Success;
  int ret;

  ret = avcodec_send_packet(dec_ctx, pkt);

  if (ret < 0) {
    res = kErrorCode_FFmpeg_Error;
  } else {
    while (ret >= 0) {
      ret = avcodec_receive_frame(dec_ctx, frame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      } else if (ret < 0) {
        res = kErrorCode_FFmpeg_Error;
        break;
      }
      AVFrame_to_yuv_buffer(frame);
    }
  }

  return res;
}

ErrorCode decode_AnnexB_buffer(const uint8_t* buffer, size_t buffer_size) {
  ErrorCode ret = kErrorCode_Success;

  while (buffer_size > 0) {
    int size = av_parser_parse2(parser_context, codec_context, &pkt->data,
                                &pkt->size, buffer, buffer_size, AV_NOPTS_VALUE,
                                AV_NOPTS_VALUE, 0);

    if (size < 0) {
      ret = kErrorCode_FFmpeg_Error;
      break;
    }
    
    buffer += size;
    buffer_size -= size;

    if (pkt->size) {
      ret = decode_AVPacket(codec_context, frame, pkt);
      if (ret != kErrorCode_Success) {
        break;
      }
    }
  }

  return ret;
}

ErrorCode flush_decoder() { return decode_AVPacket(codec_context, frame, NULL); }

ErrorCode close_decoder() {
  ErrorCode ret = kErrorCode_Success;

  if (parser_context != NULL) {
    av_parser_close(parser_context);
  }
  if (codec_context != NULL) {
    avcodec_free_context(&codec_context);
  }
  if (frame != NULL) {
    av_frame_free(&frame);
  }
  if (pkt != NULL) {
    av_packet_free(&pkt);
  }

  return ret;
}

int main(int argc, char** argv) { return 0; }