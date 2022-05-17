#pragma once

#include <string>
#include <mutex>
#include <memory>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/hwcontext.h"
#include "libavutil/pixdesc.h"

}
#include "libavutil/hwcontext.h"
#include "libavutil/hwcontext_d3d11va.h"

class AVDecoder
{
public:
	AVDecoder& operator=(const AVDecoder&) = delete;
	AVDecoder(const AVDecoder&) = delete;
	AVDecoder();
	virtual ~AVDecoder();

	virtual bool Init(AVStream* stream, void* d3d11_device);
	virtual void Destroy();

	virtual int  Send(AVPacket* packet);
	virtual int  Recv(AVFrame* frame);

private:
	std::mutex mutex_;

	AVStream* stream_ = nullptr;
	AVCodecContext* codec_context_ = nullptr;
	AVDictionary* options_ = nullptr;
	AVBufferRef* device_buffer_ = nullptr;

	int decoder_reorder_pts_ = -1;

	int64_t next_pts_ = AV_NOPTS_VALUE;
	int64_t start_pts_ = AV_NOPTS_VALUE;
	int finished_ = -1;
	AVRational start_pts_tb_;
	AVRational next_pts_tb_;

public:
    ID3D11Device* decoder_device_ = nullptr;
    ID3D11DeviceContext* decoder_device_context_ = nullptr;
    ID3D11VideoDevice* decoder_video_device_ = nullptr;
    ID3D11VideoContext* decoder_video_context_ = nullptr;
};

