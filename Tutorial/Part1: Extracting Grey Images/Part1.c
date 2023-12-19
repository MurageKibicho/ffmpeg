#include <assert.h>
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//gcc Part1.c -o Part1.o -lm -lavcodec -lavutil -lavformat -lswscale && ./Part1.o

void SaveGreyFramePPM(uint8_t *pixels, int wrap, int imageHeight, int imageWidth, char *fileName)
{
  FILE *fp = fopen(fileName, "w"); 
  printf("\n\nWrap: %d\n\n", wrap);
  fprintf(fp,"P5\n%d %d\n%d\n",imageWidth,imageHeight,255);
  for(int i = 0; i < imageHeight; i++)
  {
    unsigned char *ch = (pixels+i*wrap);
    fwrite(ch,1,imageWidth,fp);
  }
  fclose(fp);
}

int DecodeVideoPacket_GreyFrame(AVPacket *packet, AVCodecContext *codecContext, AVFrame *frame)
{
  int returnValue = 0;
  //Send compressed packet for decompression
  returnValue = avcodec_send_packet(codecContext, packet);
  if(returnValue != 0){av_log(NULL, AV_LOG_ERROR, "Error decompressing packet");return returnValue;}
  while(returnValue >= 0)
  {
      returnValue = avcodec_receive_frame(codecContext, frame);
      if(returnValue == AVERROR(EAGAIN))
      {
        //Not data memory for frame, have to free and get more data
        printf("Not enough data\n");
        av_frame_unref(frame);
        av_freep(frame);
        break;
      }
      else if(returnValue == AVERROR_EOF){av_log(NULL, AV_LOG_ERROR, "End Of File Reached");av_frame_unref(frame);av_freep(frame);return returnValue;}
      else if(returnValue < 0){av_log(NULL, AV_LOG_ERROR, "Error in receiving frame");av_frame_unref(frame);av_freep(frame);return returnValue;}
      else
      {
        // We got a picture!
        printf( "Frame number %d (type=%c frame, size = %d bytes, width = %d, height = %d) pts %ld key_frame %d [DTS %d]\n",codecContext->frame_number,av_get_picture_type_char(frame->pict_type),frame->pkt_size,frame->width,frame->height,frame->pts,frame->key_frame,frame->coded_picture_number);
        SaveGreyFramePPM(frame->data[0],frame->linesize[0],frame->height,frame->width, "Test.ppm");
        
      }
  }

  return returnValue;
}

int main(void) {
  char *fileName = "out.mp4";

  AVFormatContext *formatContext = NULL;
  
  AVPacket *packet = NULL;
  AVFrame *videoFrame = NULL;

  AVCodecParameters *videoCodecParameters = NULL;
  AVCodec *videoCodec = NULL; 
  AVCodecContext *videoCodecContext = NULL;
  int videoStreamIndex = -1;

  AVCodecParameters *audioCodecParameters = NULL;
  AVCodec *audioCodec = NULL; 
  AVCodecContext *audioCodecContext = NULL;
  int audioStreamIndex = -1;

  int returnValue = 0;

  /*Check File Exists and allocate format context*/
  returnValue = avformat_open_input(&formatContext, fileName, NULL, NULL);
  if(returnValue != 0){av_log(NULL, AV_LOG_ERROR, "Error opening file");return -1;}

  /*Allocate Packet*/
  packet = av_packet_alloc(); 
  if(!packet){av_log(NULL, AV_LOG_ERROR, "Error allocating packet");return -1;}

  /*Allocate Frame*/
  videoFrame = av_frame_alloc();
  if(!videoFrame){av_log(NULL, AV_LOG_ERROR, "Error allocating packet");return -1;}


  /*Ensure stream information exists*/
  if(avformat_find_stream_info(formatContext, NULL) < 0){av_log(NULL, AV_LOG_ERROR, "Stream information not found");return -1;}


  for(int i = 0; i < formatContext->nb_streams; i++)
  {
    AVCodecParameters *currentCodecParameters = NULL;
    AVCodec *currentCodec = NULL;
    AVStream *currentStream = NULL;

    currentStream = formatContext->streams[i]; 
    currentCodecParameters = currentStream->codecpar;
    currentCodec = avcodec_find_decoder(currentCodecParameters->codec_id);
    if(currentCodec == NULL){av_log(NULL, AV_LOG_ERROR, "Codec not supported");continue;}

  
    if(currentCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      double frameRate = av_q2d(currentStream->r_frame_rate);
      videoStreamIndex = i;
      videoCodec = currentCodec;
      videoCodecParameters = currentCodecParameters;

      printf("\nFound video stream\n");
      printf("ID: %d\n Codec: %s\n BitRate: %ld\n Width :%d, Height %d\n Framerate: %f fps\n", currentCodecParameters->codec_id, currentCodec->name, currentCodecParameters->bit_rate, currentCodecParameters->width, currentCodecParameters->height, frameRate);
    }
    else if(currentCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      audioStreamIndex = i;
      audioCodec = currentCodec;
      audioCodecParameters = currentCodecParameters;
      printf("\nFound audio stream\n");
      printf("ID: %d\n Codec: %s\n BitRate: %ld\nChannels :%d, Sample Rate %d\n\n",currentCodecParameters->codec_id, currentCodec->name, currentCodecParameters->bit_rate, currentCodecParameters->channels, currentCodecParameters->sample_rate);
    }
    else if(currentCodecParameters->codec_type == AVMEDIA_TYPE_SUBTITLE)
    {
      printf("\nFound subtitle stream\n");
    }
  }

  if(videoStreamIndex == -1){av_log(NULL, AV_LOG_ERROR, "Error Not found video stream");return -1;}

  /*Allocate codec context*/
  videoCodecContext = avcodec_alloc_context3(videoCodec);
  if(!videoCodecContext){av_log(NULL, AV_LOG_ERROR, "Error allocating codec context");return -1;}

  //https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga8a4998c9d1695abb01d379539d313227
  returnValue = avcodec_parameters_to_context(videoCodecContext, videoCodecParameters);
  if(returnValue != 0){av_log(NULL, AV_LOG_ERROR, "Error copying codec parameters to context");return -1;}

  returnValue = avcodec_open2(videoCodecContext, videoCodec, NULL);
  if(returnValue != 0){av_log(NULL, AV_LOG_ERROR, "Error opening avcodec");return -1;}

  int packetCount = 0;
  while(av_read_frame(formatContext,packet) >= 0)
  {
    if(packet->stream_index == videoStreamIndex)
    {
      int64_t duration = packet->pts;
      printf("Video packet : ");
      returnValue = DecodeVideoPacket_GreyFrame(packet, videoCodecContext, videoFrame);
      //break;
    }
    else if(packet->stream_index == audioStreamIndex)
    {
      int64_t duration = packet->pts;
      printf("Audio packet : ");
    }
    packetCount += 1;
    if(packetCount == 10){break;}
  }
  /*Free memory*/
  avformat_close_input(&formatContext);
  av_packet_free(&packet); 
  av_free(videoFrame);
  avcodec_free_context(&videoCodecContext);
  return 0;
}
