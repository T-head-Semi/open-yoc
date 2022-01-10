/*
 * =====================================================================================
 *
 *       Filename:  vbasetype.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020年08月31日 15时28分21秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:
 *
 * =====================================================================================
 */
#ifndef __V_BASE_TYPE_H__
#define __V_BASE_TYPE_H__
#include "typedef.h"
enum EVIDEOCODECFORMAT
{
    VIDEO_CODEC_FORMAT_UNKNOWN          = 0,
    VIDEO_CODEC_FORMAT_MJPEG            = 0x101,
    VIDEO_CODEC_FORMAT_MPEG1            = 0x102,
    VIDEO_CODEC_FORMAT_MPEG2            = 0x103,
    VIDEO_CODEC_FORMAT_MPEG4            = 0x104,
    VIDEO_CODEC_FORMAT_MSMPEG4V1        = 0x105,
    VIDEO_CODEC_FORMAT_MSMPEG4V2        = 0x106,
    VIDEO_CODEC_FORMAT_DIVX3            = 0x107, //* not support
    VIDEO_CODEC_FORMAT_DIVX4            = 0x108, //* not support
    VIDEO_CODEC_FORMAT_DIVX5            = 0x109, //* not support
    VIDEO_CODEC_FORMAT_XVID             = 0x10a,
    VIDEO_CODEC_FORMAT_H263             = 0x10b,
    VIDEO_CODEC_FORMAT_SORENSSON_H263   = 0x10c,
    VIDEO_CODEC_FORMAT_RXG2             = 0x10d,
    VIDEO_CODEC_FORMAT_WMV1             = 0x10e,
    VIDEO_CODEC_FORMAT_WMV2             = 0x10f,
    VIDEO_CODEC_FORMAT_WMV3             = 0x110,
    VIDEO_CODEC_FORMAT_VP6              = 0x111,
    VIDEO_CODEC_FORMAT_VP8              = 0x112,
    VIDEO_CODEC_FORMAT_VP9              = 0x113,
    VIDEO_CODEC_FORMAT_RX               = 0x114,
    VIDEO_CODEC_FORMAT_H264             = 0x115,
    VIDEO_CODEC_FORMAT_H265             = 0x116,
    VIDEO_CODEC_FORMAT_AVS              = 0x117,
    VIDEO_CODEC_FORMAT_AVS2             = 0x118,

    VIDEO_CODEC_FORMAT_MAX              = VIDEO_CODEC_FORMAT_AVS2,
    VIDEO_CODEC_FORMAT_MIN              = VIDEO_CODEC_FORMAT_MJPEG,
};

enum EPIXELFORMAT
{
    PIXEL_FORMAT_DEFAULT            = 0,

    PIXEL_FORMAT_YUV_PLANER_420     = 1,
    PIXEL_FORMAT_YUV_PLANER_422     = 2,
    PIXEL_FORMAT_YUV_PLANER_444     = 3,

    PIXEL_FORMAT_YV12               = 4,
    PIXEL_FORMAT_NV21               = 5,
    PIXEL_FORMAT_NV12               = 6,
    PIXEL_FORMAT_YUV_MB32_420       = 7,
    PIXEL_FORMAT_YUV_MB32_422       = 8,
    PIXEL_FORMAT_YUV_MB32_444       = 9,

    PIXEL_FORMAT_RGBA                = 10,
    PIXEL_FORMAT_ARGB                = 11,
    PIXEL_FORMAT_ABGR                = 12,
    PIXEL_FORMAT_BGRA                = 13,

    PIXEL_FORMAT_YUYV                = 14,
    PIXEL_FORMAT_YVYU                = 15,
    PIXEL_FORMAT_UYVY                = 16,
    PIXEL_FORMAT_VYUY                = 17,

    PIXEL_FORMAT_PLANARUV_422        = 18,
    PIXEL_FORMAT_PLANARVU_422        = 19,
    PIXEL_FORMAT_PLANARUV_444        = 20,
    PIXEL_FORMAT_PLANARVU_444        = 21,
    PIXEL_FORMAT_P010_UV             = 22,
    PIXEL_FORMAT_P010_VU             = 23,

    PIXEL_FORMAT_MIN = PIXEL_FORMAT_DEFAULT,
    PIXEL_FORMAT_MAX = PIXEL_FORMAT_PLANARVU_444,
};

typedef struct VIDEO_FRM_MV_INFO
{
    s16              nMaxMv_x;
    s16              nMinMv_x;
    s16              nAvgMv_x;
    s16              nMaxMv_y;
    s16              nMinMv_y;
    s16              nAvgMv_y;
    s16              nMaxMv;
    s16              nMinMv;
    s16              nAvgMv;
    s16              SkipRatio;
}VIDEO_FRM_MV_INFO;

typedef enum VID_FRAME_TYPE
{
    VIDEO_FORMAT_TYPE_UNKONWN = 0,
    VIDEO_FORMAT_TYPE_I,
    VIDEO_FORMAT_TYPE_P,
    VIDEO_FORMAT_TYPE_B,
    VIDEO_FORMAT_TYPE_IDR,
    VIDEO_FORMAT_TYPE_BUTT,
}VID_FRAME_TYPE;

typedef struct VIDEO_FRM_STATUS_INFO
{
    VID_FRAME_TYPE   enVidFrmType;
    int              nVidFrmSize;
    int              nVidFrmDisW;
    int              nVidFrmDisH;
    int              nVidFrmQP;
    double           nAverBitRate;
    double           nFrameRate;
    int64_t          nVidFrmPTS;
    VIDEO_FRM_MV_INFO nMvInfo;
    int              bDropPreFrame;
}VIDEO_FRM_STATUS_INFO;

typedef enum VIDEO_TRANSFER
{
    VIDEO_TRANSFER_RESERVED_0      = 0,
    VIDEO_TRANSFER_BT1361          = 1,
    VIDEO_TRANSFER_UNSPECIFIED     = 2,
    VIDEO_TRANSFER_RESERVED_1      = 3,
    VIDEO_TRANSFER_GAMMA2_2        = 4,
    VIDEO_TRANSFER_GAMMA2_8        = 5,
    VIDEO_TRANSFER_SMPTE_170M      = 6,
    VIDEO_TRANSFER_SMPTE_240M      = 7,
    VIDEO_TRANSFER_LINEAR          = 8,
    VIDEO_TRANSFER_LOGARITHMIC_0   = 9,
    VIDEO_TRANSFER_LOGARITHMIC_1   = 10,
    VIDEO_TRANSFER_IEC61966        = 11,
    VIDEO_TRANSFER_BT1361_EXTENDED = 12,
    VIDEO_TRANSFER_SRGB     = 13,
    VIDEO_TRANSFER_BT2020_0 = 14,
    VIDEO_TRANSFER_BT2020_1 = 15,
    VIDEO_TRANSFER_ST2084   = 16,
    VIDEO_TRANSFER_ST428_1  = 17,
    VIDEO_TRANSFER_HLG      = 18,
    VIDEO_TRANSFER_RESERVED = 19, //* 19~255
}VIDEO_TRANSFER;

typedef enum VIDEO_MATRIX_COEFFS
{
    VIDEO_MATRIX_COEFFS_IDENTITY       = 0,
    VIDEO_MATRIX_COEFFS_BT709          = 1,
    VIDEO_MATRIX_COEFFS_UNSPECIFIED_0  = 2,
    VIDEO_MATRIX_COEFFS_RESERVED_0     = 3,
    VIDEO_MATRIX_COEFFS_BT470M         = 4,
    VIDEO_MATRIX_COEFFS_BT601_625_0    = 5,
    VIDEO_MATRIX_COEFFS_BT601_625_1    = 6,
    VIDEO_MATRIX_COEFFS_SMPTE_240M     = 7,
    VIDEO_MATRIX_COEFFS_YCGCO          = 8,
    VIDEO_MATRIX_COEFFS_BT2020         = 9,
    VIDEO_MATRIX_COEFFS_BT2020_CONSTANT_LUMINANCE = 10,
    VIDEO_MATRIX_COEFFS_SOMPATE                   = 11,
    VIDEO_MATRIX_COEFFS_CD_NON_CONSTANT_LUMINANCE = 12,
    VIDEO_MATRIX_COEFFS_CD_CONSTANT_LUMINANCE     = 13,
    VIDEO_MATRIX_COEFFS_BTICC                     = 14,
    VIDEO_MATRIX_COEFFS_RESERVED                  = 15, //* 15~255
}VIDEO_MATRIX_COEFFS;

typedef enum VIDEO_FULL_RANGE_FLAG
{
    VIDEO_FULL_RANGE_LIMITED = 0,
    VIDEO_FULL_RANGE_FULL    = 1,
}VIDEO_FULL_RANGE_FLAG;

typedef struct VIDEOPICTURE
{
    int     nID;
    int     nStreamIndex;
    int     ePixelFormat;
    int     nWidth;
    int     nHeight;
    int     nLineStride;
    int     nTopOffset;
    int     nLeftOffset;
    int     nBottomOffset;
    int     nRightOffset;
    int     nFrameRate;
    int     nAspectRatio;
    int     bIsProgressive;
    int     bTopFieldFirst;
    int     bRepeatTopField;
    int64_t nPts;
    int64_t nPcr;
    char*   pData0;
    char*   pData1;
    char*   pData2;
    char*   pData3;
    int     bMafValid;
    char*   pMafData;
    int     nMafFlagStride;
    int     bPreFrmValid;
    int     nBufId;
    size_addr phyYBufAddr;
    size_addr phyCBufAddr;
    void*   pPrivate;
    int     nBufFd;
    int     nBufStatus;
    int     bTopFieldError;
    int        bBottomFieldError;
    int     nColorPrimary;  // default value is 0xffffffff, valid value id 0x0000xxyy
                            // xx: is video full range code
                            // yy: is matrix coefficient
    int     bFrameErrorFlag;

    //* to save hdr info and afbc header info
    void*   pMetaData;

    //*display related parameter
    VIDEO_FULL_RANGE_FLAG   video_full_range_flag;
    VIDEO_TRANSFER          transfer_characteristics;
    VIDEO_MATRIX_COEFFS     matrix_coeffs;
    u8      colour_primaries;
    //*end of display related parameter defined
    //size_addr    nLower2BitPhyAddr;
    int          nLower2BitBufSize;
    int          nLower2BitBufOffset;
    int          nLower2BitBufStride;
    int          b10BitPicFlag;
    int          bEnableAfbcFlag;
    int          nLbcLossyComMod;//1:1.5x; 2:2x; 3:2.5x;
    unsigned int bIsLossy; //lossy compression or not
    unsigned int bRcEn;    //compact storage or not

    int     nBufSize;
    int     nAfbcSize;
    int     nLbcSize;
    int     nDebugCount;
    VIDEO_FRM_STATUS_INFO nCurFrameInfo;
}VideoPicture;

#endif
