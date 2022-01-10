
/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : vdecoder.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef VDECODER_H
#define VDECODER_H

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "typedef.h"
#include <allwinner/sc_interface.h>
#include "allwinner/veInterface.h"
#include "allwinner/vbasetype.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AFBC_VERSION   4
#define AFBC_FILEHEADER_SIZE 32

#define AFBC_ENABLE_SIZE_WIDTH (3840)  //* enable afbc when (pic_size) >= (the_value)
#define AFBC_ENABLE_SIZE_HEIGHT (2160)

#define ENABLE_HIGH_CHANNAL_SIZE_WIDTH (3840)
#define ENABLE_HIGH_CHANNAL_SIZE_HEIGHT (2160)
#define ENABLE_SBM_FRAME_INTERFACE (1)
#define ENABLE_NEW_MEMORY_OPTIMIZATION_PROGRAM (0)

typedef struct VeProcDecInfo {
    unsigned int     nChannelNum;
    char             nProfile[10];

    unsigned int     nWidth;
    unsigned int     nHeight;
    s32              nCurFrameRef;
    int              nFrameRate;
}VeProcDecInfo;


typedef enum CONTROL_AFBC_MODE {
    DISABLE_AFBC_ALL_SIZE     = 0,
    ENABLE_AFBC_JUST_BIG_SIZE = 1, //* >= 4k
    ENABLE_AFBC_ALL_SIZE      = 2,
}eControlAfbcMode;

typedef enum CONTROL_IPTV_MODE {
    DISABLE_IPTV_ALL_SIZE     = 0,
    ENABLE_IPTV_JUST_SMALL_SIZE = 1, //* < 4k
    ENABLE_IPTV_ALL_SIZE      = 2,
}eControlIptvMode;

typedef enum COMMON_CONFIG_FLAG
{
   IS_MIRACAST_STREAM = 1,

}eCommonConfigFlag;

enum EVDECODERESULT
{
    VDECODE_RESULT_UNSUPPORTED       = -1,
    VDECODE_RESULT_OK                = 0,
    VDECODE_RESULT_FRAME_DECODED     = 1,
    VDECODE_RESULT_CONTINUE          = 2,
    VDECODE_RESULT_KEYFRAME_DECODED  = 3,
    VDECODE_RESULT_NO_FRAME_BUFFER   = 4,
    VDECODE_RESULT_NO_BITSTREAM      = 5,
    VDECODE_RESULT_RESOLUTION_CHANGE = 6,

    VDECODE_RESULT_MIN = VDECODE_RESULT_UNSUPPORTED,
    VDECODE_RESULT_MAX = VDECODE_RESULT_RESOLUTION_CHANGE,
};

//*for new display
typedef struct FBMBUFINFO
{
    int nBufNum;
    int nBufWidth;
    int nBufHeight;
    int ePixelFormat;
    int nAlignValue;
    int bProgressiveFlag;
    int bIsSoftDecoderFlag;

    int bHdrVideoFlag;
    int b10bitVideoFlag;
    int bAfbcModeFlag;
    int nLbcLossyComMod;   //1:1.5x; 2:2x; 3:2.5x;
    unsigned int bIsLossy; //lossy compression or not
    unsigned int bRcEn;    //compact storage or not

    int nTopOffset;
    int nBottomOffset;
    int nLeftOffset;
    int nRightOffset;
}FbmBufInfo;

typedef struct VIDEOSTREAMINFO
{
    int   eCodecFormat;
    int   nWidth;
    int   nHeight;
    int   nFrameRate;
    int   nFrameDuration;
    int   nAspectRatio;
    int   bIs3DStream;
    int   nCodecSpecificDataLen;
    char* pCodecSpecificData;
    int   bSecureStreamFlag;
    int   bSecureStreamFlagLevel1;
    int   bIsFramePackage; /* 1: frame package;  0: stream package */
    int   h265ReferencePictureNum;
    int   bReOpenEngine;
    int   bIsFrameCtsTestFlag;
}VideoStreamInfo;

typedef struct VCONFIG
{
    int bScaleDownEn;
    int bRotationEn;
    int bSecOutputEn;
    int nHorizonScaleDownRatio;
    int nVerticalScaleDownRatio;
    int nSDWidth;
    int nSDHeight;
    int bAnySizeSD;
    int nSecHorizonScaleDownRatio;
    int nSecVerticalScaleDownRatio;
    int nRotateDegree;
    int bThumbnailMode;
    int eOutputPixelFormat;
    int eSecOutputPixelFormat;
    int bNoBFrames;
    int bDisable3D;
    int bSupportMaf;    //not use
    int bDispErrorFrame;
    int nVbvBufferSize;
    int nFrameBufferNum;
    int bSecureosEn;
    int  bGpuBufValid;
    int  nAlignStride;
    int  bIsSoftDecoderFlag;
    int  bVirMallocSbm;
    int  bSupportPallocBufBeforeDecode;
    //only used for xuqi, set this flag to 1 meaning palloc the fbm buffer before
    // decode the sequence, to short the first frame decoing time
    int nDeInterlaceHoldingFrameBufferNum;
    int nDisplayHoldingFrameBufferNum;
    int nRotateHoldingFrameBufferNum;
    int nDecodeSmoothFrameBufferNum;
    int bIsTvStream;
    int nLbcLossyComMod;   //1:1.5x; 2:2x; 3:2.5x;
    unsigned int bIsLossy; //lossy compression or not
    unsigned int bRcEn;    //compact storage or not

    struct ScMemOpsS *memops;
    eControlAfbcMode  eCtlAfbcMode;
    eControlIptvMode  eCtlIptvMode;

    VeOpsS*           veOpsS;
    void*             pVeOpsSelf;
    int               bConvertVp910bitTo8bit;
    unsigned int      nVeFreq;

    int               bCalledByOmxFlag;

    int               bSetProcInfoEnable; //* for check the decoder info by cat devices-note
    int               nSetProcInfoFreq;
    int               nChannelNum;
    int               nSupportMaxWidth;  //the max width of mjpeg continue decode
    int               nSupportMaxHeight; //the max height of mjpeg continue decode
    eCommonConfigFlag  commonConfigFlag;
    int               bATMFlag;
}VConfig;

typedef struct VIDEOSTREAMDATAINFO
{
    char*   pData;
    int     nLength;
    int64_t nPts;
    int64_t nPcr;
    int     bIsFirstPart;
    int     bIsLastPart;
    int     nID;
    int     nStreamIndex;
    int     bValid;
    unsigned int     bVideoInfoFlag;
    void*   pVideoInfo;
}VideoStreamDataInfo;




typedef struct VIDEOFBMINFO
{
    unsigned int         nValidBufNum;
    void*                pFbmFirst;
    void*                pFbmSecond;
    FbmBufInfo           pFbmBufInfo;
    unsigned int         bIs3DStream;
    unsigned int         bTwoStreamShareOneFbm;
    VideoPicture*        pMajorDispFrame;
    VideoPicture*        pMajorDecoderFrame;
    unsigned int         nMinorYBufOffset;
    unsigned int         nMinorCBufOffset;
    int                  bIsFrameCtsTestFlag;
    int                  nExtraFbmBufferNum;
    int                  nDecoderNeededMiniFbmNum;
    int                  nDecoderNeededMiniFbmNumSD;
    int                  bIsSoftDecoderFlag;
}VideoFbmInfo;

typedef struct JPEGSKIACONFIG
{
    int mode_selection;
    int filed_alpha;
    int imcu_int_minus1;
    int region_top;
    int region_bot;
    int region_left;
    int region_right;
    int nScaleDownRatio;
    void* pFrameBuffer;
    void* pInputIndexBuffer;
    int   nInputIndexSize;
    void* pTileVbvBuffer;
    int nTileVbvVBufferSize;
}JpegSkiaConfig;


//added by xyliu for set and get the decoder debug command
enum EVDECODERSETPERFORMCMD
{
    VDECODE_SETCMD_DEFAULT                  = 0,
    VDECODE_SETCMD_START_CALDROPFRAME       = 1,
    VDECODE_SETCMD_STOP_CALDROPFRAME        = 2,
};

enum EVDECODERGETPERFORMCMD
{
    VDECODE_GETCMD_DEFAULT                  = 0,
    VDECODE_GETCMD_DROPFRAME_INFO           = 1,
};

typedef struct VID_PERFORMANCE
{
    unsigned int nDropFrameNum;
    // this variable is valid for VDECODE_GETCMD_DROPFRAME_INFO command
    int nFrameDuration;
}VDecodePerformaceInfo;

typedef void* VideoDecoder;

//* NO.1
extern void AddVDPlugin(void);

//* NO.2
VideoDecoder* CreateVideoDecoder(void);

//* NO.3
void DestroyVideoDecoder(VideoDecoder* pDecoder);

//* NO.4
int InitializeVideoDecoder(VideoDecoder* pDecoder,
                     VideoStreamInfo* pVideoInfo,
                     VConfig* pVconfig);

//* NO.5
void ResetVideoDecoder(VideoDecoder* pDecoder);

int ConfigRotateInfo(VideoDecoder* pDecoder, int nDegree);

//* NO.6
int ReopenVideoEngine(VideoDecoder* pDecoder,
                    VConfig* pVConfig,
                    VideoStreamInfo* pStreamInfo);

//* NO.7
int DecodeVideoStream(VideoDecoder* pDecoder,
                      int           bEndOfStream,
                      int           bDecodeKeyFrameOnly,
                      int           bDropBFrameIfDelay,
                      int64_t       nCurrentTimeUs);

//* NO.8 about sbm
int RequestVideoStreamBuffer(VideoDecoder* pDecoder,
                             int           nRequireSize,
                             char**        ppBuf,
                             int*          pBufSize,
                             char**        ppRingBuf,
                             int*          pRingBufSize,
                             int           nStreamBufIndex);
//* NO.9 about sbm
int SubmitVideoStreamData(VideoDecoder*        pDecoder,
                          VideoStreamDataInfo* pDataInfo,
                          int                  nStreamBufIndex);

//* NO.10 about fbm
VideoPicture* NextPictureInfo(VideoDecoder* pDecoder, int nStreamIndex);

//* NO.11 about fbm
VideoPicture* RequestPicture(VideoDecoder* pDecoder, int nStreamIndex);

//* NO.12 about fbm
int ReturnPicture(VideoDecoder* pDecoder, VideoPicture* pPicture);

//* NO.13
int RotatePicture(struct ScMemOpsS* memOps,
                  VideoPicture* pPictureIn,
                  VideoPicture* pPictureOut,
                  int           nRotateDegree,
                  int           nGpuYAlign,
                  int           nGpuCAlign);

//* NO.14
int RotatePictureHw(VideoDecoder* pDecoder,
                    VideoPicture* pPictureIn,
                    VideoPicture* pPictureOut,
                    int           nRotateDegree);

//* NO.15
int GetVideoStreamInfo(VideoDecoder* pDecoder,
                    VideoStreamInfo* pVideoInfo);

//* NO.16 about sbm
void* VideoStreamBufferAddress(VideoDecoder* pDecoder, int nStreamBufIndex);
int VideoStreamBufferSize(VideoDecoder* pDecoder, int nStreamBufIndex);

//* NO.17 about sbm
int VideoStreamDataSize(VideoDecoder* pDecoder, int nStreamBufIndex);

//* NO.18 about sbm
int VideoStreamFrameNum(VideoDecoder* pDecoder, int nStreamBufIndex);

//* NO.19 about sbm
void* VideoStreamDataInfoPointer(VideoDecoder* pDecoder, int nStreamBufIndex);

//* NO.20 about fbm
int TotalPictureBufferNum(VideoDecoder* pDecoder, int nStreamIndex);

//* NO.21 about fbm
int EmptyPictureBufferNum(VideoDecoder* pDecoder, int nStreamIndex);

//* NO.22 about fbm
int ValidPictureNum(VideoDecoder* pDecoder, int nStreamIndex);

//* NO.25 about malloc buffer
void *VideoDecoderPallocIonBuf(VideoDecoder* pDecoder, int nSize);

//* NO.26 about malloc buffer
void VideoDecoderFreeIonBuf(VideoDecoder* pDecoder, void *pIonBuf);


//* NO.27 about new display
FbmBufInfo* GetVideoFbmBufInfo(VideoDecoder* pDecoder);

//* NO.28 about new display
VideoPicture* SetVideoFbmBufAddress(VideoDecoder* pDecoder,
                                 VideoPicture* pVideoPicture,
                                 int bForbidUseFlag);

//* NO.29 about new display
int SetVideoFbmBufRelease(VideoDecoder* pDecoder);


//* NO.30 about new display
VideoPicture* RequestReleasePicture(VideoDecoder* pDecoder);


//* NO.31 about new display
VideoPicture*  ReturnRelasePicture(VideoDecoder* pDecoder,
                              VideoPicture* pVpicture,
                              int bForbidUseFlag);

//* NO.32
int ConfigExtraScaleInfo(VideoDecoder* pDecoder,
                     int nWidthTh,
                     int nHeightTh,
                     int nHorizonScaleRatio,
                     int nVerticalScaleRatio);

//* NO.33
int DecoderSetSpecialData(VideoDecoder* pDecoder, void *pArg);

//* NO.34
int SetDecodePerformCmd(VideoDecoder* pDecoder,
                      enum EVDECODERSETPERFORMCMD performCmd);

//* NO.35
int GetDecodePerformInfo(VideoDecoder* pDecoder,
                      enum EVDECODERGETPERFORMCMD performCmd,
                      VDecodePerformaceInfo** performInfo);

int VideoDecoderSetFreq(VideoDecoder* pDecoder, int nVeFreq);
#ifdef __cplusplus
}
#endif

#endif

