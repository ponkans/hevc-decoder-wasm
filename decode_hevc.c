#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>

typedef void(*VideoCallback)(unsigned char* data_y, unsigned char* data_u, unsigned char* data_v, int line1, int line2, int line3, int width, int height, long pts);

typedef enum ErrorCode {
	kErrorCode_Success = 0,
	kErrorCode_FFmpeg_Error = 1
}ErrorCode;

VideoCallback videoCallback = NULL;

const AVCodec* codec;
AVCodecParserContext* parser;
AVCodecContext* c = NULL;
AVPacket* pkt;
AVFrame* frame;
AVFrame* outFrame;
long ptslist[10];

ErrorCode copyFrameData(AVFrame* src, AVFrame* dst, long ptslist[]) {
	ErrorCode ret = kErrorCode_Success;
	memcpy(dst->data, src->data, sizeof(src->data));
	dst->linesize[0] = src->linesize[0];
	dst->linesize[1] = src->linesize[1];
	dst->linesize[2] = src->linesize[2];
	dst->width = src->width;
	dst->height = src->height;
	long pts = LONG_MAX;
	int index = -1;
	for (int i = 0; i < 10; i++) {
		if (ptslist[i] < pts) {
			pts = ptslist[i];
			index = i;
		}
	}
	if (index > -1) {
		ptslist[index] = LONG_MAX;
	}
	dst->pts = pts;
	return ret;
}

static ErrorCode decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, AVFrame* outFrame, long ptslist[])
{
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
			}
			else if (ret < 0) {
				res = kErrorCode_FFmpeg_Error;
				break;
			}

			res = copyFrameData(frame, outFrame, ptslist);
			if (res != kErrorCode_Success) {
				break;
			}

			// 替换成 yuv420 的姿势

			videoCallback(outFrame->data[0], outFrame->data[1], outFrame->data[2], outFrame->linesize[0], outFrame->linesize[1], outFrame->linesize[2], outFrame->width, outFrame->height, outFrame->pts);
		}
	}

	return res;
}

ErrorCode openDecoder(int codecType, long callback, int logLv) {
	ErrorCode ret = kErrorCode_Success;
	do {
		codec = avcodec_find_decoder(AV_CODEC_ID_H265);
		if (!codec) {
			ret = kErrorCode_FFmpeg_Error;
			break;
		}

		parser = av_parser_init(codec->id);
		if (!parser) {
			ret = kErrorCode_FFmpeg_Error;
			break;
		}

		c = avcodec_alloc_context3(codec);
		if (!c) {
			ret = kErrorCode_FFmpeg_Error;
			break;
		}

		if (avcodec_open2(c, codec, NULL) < 0) {
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

		outFrame = av_frame_alloc();
		if (!outFrame) {
			ret = kErrorCode_FFmpeg_Error;
			break;
		}

		pkt = av_packet_alloc();
		if (!pkt) {
			ret = kErrorCode_FFmpeg_Error;
			break;
		}

		for (int i = 0; i < 10; i++) {
			ptslist[i] = LONG_MAX;
		}

		videoCallback = (VideoCallback)callback;

	} while (0);
	return ret;
}

ErrorCode decodeData(unsigned char* data, size_t data_size, long pts) {
	ErrorCode ret = kErrorCode_Success;

	for (int i = 0; i < 10; i++) {
		if (ptslist[i] == LONG_MAX) {
			ptslist[i] = pts;
			break;
		}
	}

	while (data_size > 0) {
		int size = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
			data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (size < 0) {
			ret = kErrorCode_FFmpeg_Error;
			break;
		}
		data += size;
		data_size -= size;

		if (pkt->size) {
			ret = decode(c, frame, pkt, outFrame, ptslist);
			if (ret != kErrorCode_Success) {
        break;
      }
		}
	}
	return ret;
}

ErrorCode flushDecoder() {
	return decode(c, frame, NULL, outFrame, ptslist);
}

ErrorCode closeDecoder() {
	ErrorCode ret = kErrorCode_Success;

	if (parser != NULL) {
		av_parser_close(parser);
	}
	if (c != NULL) {
		avcodec_free_context(&c);
	}
	if (frame != NULL) {
		av_frame_free(&frame);
	}
	if (pkt != NULL) {
		av_packet_free(&pkt);
	}
	if (outFrame != NULL) {
		av_frame_free(&outFrame);
	}
	
	return ret;
}

int main(int argc, char** argv)
{
	return 0;
}
