/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "stream/stream.h"
#include "avformat/avformat_utils.h"

/**
 * @brief  xxx_sync read bytes from stream
 * @param  [in] opaque
 * @param  [in] bytes
 * @param  [in] size
 * @return -1 on error
 */
int sync_read_stream(void *opaque, uint8_t *bytes, size_t size)
{
    int rc;
    stream_cls_t *s = opaque;

    rc = stream_read(s, bytes, size);

    return rc;
}

/**
 * @brief  xxx_sync read bytes from bio_t, may be used for avformat probe
 * @param  [in] opaque
 * @param  [in] bytes
 * @param  [in] size
 * @return -1 on error
 */
int sync_read_bio(void *opaque, uint8_t *bytes, size_t size)
{
    bio_t *bio = opaque;
    return bio_read(bio, bytes, size);
}

/**
 * @brief  get size of the id2v2
 * @param  [in] buf
 * @param  [in] buf_size
 * @return 0 on no id3 info
 */
size_t get_id3v2_size(const uint8_t *buf, size_t buf_size)
{
    CHECK_PARAM(buf && (buf_size >= 10), 0);
    if (!strncmp((char *)buf, "ID3", 3)) {
        return (((buf[6] & 0x7f) << 21) | ((buf[7] & 0x7f) << 14) |
                ((buf[8] & 0x7f) << 7) | (buf[9] & 0x7f)) + 10;
    }

    return 0;
}

/**
 * @brief  new a track info
 * @return NULL on error
 */
track_info_t* track_info_new()
{
    return av_zalloc(sizeof(track_info_t));
}

/**
 * @brief  free the track info
 * @param  [in] track
 * @return 0/-1
 */
int track_info_free(track_info_t *track)
{
    CHECK_PARAM(track, -1);
    if (track->type == AVMEDIA_TYPE_AUDIO) {
        av_freep(&track->t.a.artist);
        av_freep(&track->t.a.album);
        av_freep(&track->t.a.title);
    }
    av_freep(&track->codec_name);
    av_free(track);

    return 0;
}

/**
 * @brief  free the track list info
 * @param  [in] ptracks
 * @return 0/-1
 */
int tracks_info_freep(track_info_t **ptracks)
{
    track_info_t *tracks, *ntrack;

    CHECK_PARAM(ptracks, -1);
    tracks = *ptracks;
    while (tracks) {
        ntrack = tracks->next;
        track_info_free(tracks);
        tracks = ntrack;
    }
    *ptracks = NULL;

    return 0;
}

/**
 * @brief  add track to the track list info
 * @param  [in] ptracks
 * @param  [in] track
 * @return 0/-1
 */
int tracks_info_add(track_info_t **ptracks, const track_info_t *track)
{
    track_info_t *tracks;

    CHECK_PARAM(ptracks && track, -1);
    tracks = *ptracks;
    if (tracks) {
        while ((*ptracks)->next) *ptracks = (*ptracks)->next;
        (*ptracks)->next = (track_info_t*)track;
        *ptracks         = tracks;
    } else {
        *ptracks = (track_info_t*)track;
    }

    return 0;
}

/**
 * @brief  duplicate the media-track
 * @param  [in] track
 * @return NULL on error
 */
track_info_t* track_info_dup(const track_info_t *track)
{
    track_info_t* tck;

    CHECK_PARAM(track && track->type != AVMEDIA_TYPE_UNKNOWN, NULL);
    tck = track_info_new();
    switch (track->type) {
    case AVMEDIA_TYPE_AUDIO:
        tck->t.a.sf     = track->t.a.sf;
        tck->t.a.artist = track->t.a.artist ? strdup(track->t.a.artist) : NULL;
        tck->t.a.album  = track->t.a.album ? strdup(track->t.a.album) : NULL;
        tck->t.a.title  = track->t.a.title ? strdup(track->t.a.title) : NULL;
        break;
    case AVMEDIA_TYPE_VIDEO:
        tck->t.v.width  = track->t.v.width;
        tck->t.v.height = track->t.v.height;
        break;
    default:
        break;
    }
    tck->type       = track->type;
    tck->codec_id   = track->codec_id;
    tck->bps        = track->bps;
    tck->duration   = track->duration;
    tck->codec_name = track->codec_name ? strdup(track->codec_name) : NULL;

    return tck;
}

/**
 * @brief  duplicate the media-track list
 * @param  [in] tracks : media track list
 * @return NULL on error
 */
track_info_t* tracks_info_dup(const track_info_t *tracks)
{
    track_info_t* tck;
    track_info_t* tcks;
    track_info_t* itcks = (track_info_t*)tracks;

    CHECK_PARAM(tracks && tracks->type != AVMEDIA_TYPE_UNKNOWN, NULL);
    tcks = track_info_dup(tracks);
    while (itcks->next) {
        tck = track_info_dup(itcks->next);
        tracks_info_add(&tcks, tck);
        itcks = itcks->next;
    }

    return tcks;
}

/**
 * @brief  init the media-info
 * @param  [in] info
 * @return 0/-1
 */
int media_info_init(media_info_t *info)
{
    CHECK_PARAM(info, -1);
    memset(info, 0, sizeof(media_info_t));
    return 0;
}

/**
 * @brief  uninit the media-info
 * @param  [in] info
 * @return
 */
void media_info_uninit(media_info_t *info)
{
    if (info) {
        /* release track list info */
        tracks_info_freep(&info->tracks);
        memset(info, 0, sizeof(media_info_t));
    }
}


