# FFMPEG C API Memory Leak Cheatsheet
**Every malloc has an equal and opposite free** ~ Newton's fourth law of Physics.

Hello guys,
My name is Murage Kibicho and this is a quick guide to freeing memory when using the FFmpeg C api.
I list the data structure, how to allocate memory and how to free memory.


## AVFormatContext
 - Allocate `avformat_open_input(AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options)`
 - Free `avformat_close_input(AVFormatContext **s)`
 
## AVFrame
 - Allocate `av_frame_alloc()`
 - Dereference buffers `av_frame_unref (AVFrame *frame)`
 - Free and leave pointer null `av_freep(void *ptr)`
 - Regular Free `av_free(void *ptr)`
 
## AVPacket
 - Allocate `av_packet_alloc()`
 - Dereference buffers `av_packet_unref(AVPacket *pkt)`
 - Free `av_packet_free(AVPacket **pkt)`
 - NOTE `av_free_packet` is deprecated. **NEVER** use.
 
## AVCodecContext
 - Allocate `avcodec_alloc_context3(const AVCodec *codec)`
 - Free `avcodec_free_context (AVCodecContext **avctx)`

## struct SwsContext
 - Allocate `sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat, int dstW, int dstH, enum AVPixelFormat dstFormat, int flags, SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param)`
 - Free `sws_freeContext(struct SwsContext *swsContext)`

 
