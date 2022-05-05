#ifndef CONFIG_KERNEL_NONE
#include <stdio.h>
#include <string.h>
#include <aos/kernel.h>
#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/control.h>
#include <aw-alsa-lib/common.h>
#include <aos_pcm.h>
#include <ulog/ulog.h>

#define TAG "alsa_port"
static void xrun(snd_pcm_t *handle)
{
    LOGD(TAG, "Xrun...\n");
    int ret = snd_pcm_prepare(handle);
    if (ret < 0) {
        LOGD(TAG, "prepare failed in xrun. return %d\n", ret);
    }
}

static int set_param(snd_pcm_t *handle, snd_pcm_format_t format,
                     unsigned int rate, unsigned int channels,
                     snd_pcm_uframes_t period_size,
                     snd_pcm_uframes_t buffer_size)
{
    int ret = 0;
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_uframes_t period_size_tmp = period_size;
    snd_pcm_uframes_t buffer_size_tmp = buffer_size;

    /* HW params */
    snd_pcm_hw_params_alloca(&params);
    ret = snd_pcm_hw_params_any(handle, params);
    if (ret < 0) {
        LOGD(TAG, "no configurations available\n");
        return ret;
    }
    ret = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        LOGD(TAG, "failed to set access\n");
        return ret;
    }
    ret = snd_pcm_hw_params_set_format(handle, params, format);
    if (ret < 0) {
        LOGD(TAG, "failed to set format\n");
        return ret;
    }
    ret = snd_pcm_hw_params_set_channels(handle, params, channels);
    if (ret < 0) {
        LOGD(TAG, "failed to set channels\n");
        return ret;
    }
    ret = snd_pcm_hw_params_set_rate(handle, params, rate, 0);
    if (ret < 0) {
        LOGD(TAG, "failed to set rate\n");
        return ret;
    }

    ret = snd_pcm_hw_params_set_period_size_near(handle, params, &period_size_tmp, NULL);
    if (ret < 0) {
        LOGD(TAG, "failed to set period size\n");
        return ret;
    }
    if (period_size_tmp != period_size) {
        LOGD(TAG, "period size changed (request: %lu, get: %lu)\n",
               period_size, period_size_tmp);
        period_size = period_size_tmp;
    }
    ret = snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer_size_tmp);
    if (ret < 0) {
        LOGD(TAG, "failed to set buffer size\n");
        return ret;
    }
    if (buffer_size_tmp != buffer_size) {
        LOGD(TAG, "buffer size changed (request: %lu, get: %lu)\n",
               buffer_size, buffer_size_tmp);
        buffer_size = buffer_size_tmp;
    }
    ret = snd_pcm_hw_params(handle, params);
    if (ret < 0) {
        LOGD(TAG, "Unable to install hw prams! (return: %d)\n", ret);
        return ret;
    }

    /* SW params */
    snd_pcm_sw_params_alloca(&sw_params);
    snd_pcm_sw_params_current(handle, sw_params);
    if (snd_pcm_stream(handle) == SND_PCM_STREAM_CAPTURE) {
        snd_pcm_sw_params_set_start_threshold(handle, sw_params, 1);
    } else {
        snd_pcm_uframes_t boundary = 0;
        snd_pcm_sw_params_get_boundary(sw_params, &boundary);
        snd_pcm_sw_params_set_start_threshold(handle, sw_params, buffer_size);
        /* set silence size, in order to fill silence data into ringbuffer */
        snd_pcm_sw_params_set_silence_size(handle, sw_params, boundary);
    }
    snd_pcm_sw_params_set_stop_threshold(handle, sw_params, buffer_size);
    snd_pcm_sw_params_set_avail_min(handle, sw_params, period_size);
    ret = snd_pcm_sw_params(handle, sw_params);
    if (ret < 0) {
        LOGD(TAG, "Unable to install sw prams!\n");
        return ret;
    }

    // if (g_verbose)
    //     snd_pcm_dump(handle);

    return ret;
}

static int pcm_read(snd_pcm_t *handle, char *data, snd_pcm_uframes_t frames_total, unsigned int frame_bytes)
{
	int ret = 0;
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_loop = 160;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0;
	unsigned int offset = 0;

	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (frames == 0)
			frames = frames_loop;
		/*LOGD(TAG, "snd_pcm_readi %ld frames\n", frames);*/
		size = snd_pcm_readi(handle, (void *)(data + offset), frames);
		if (size < 0)
			LOGD(TAG, "snd_pcm_readi return %ld", size);

		if (size == -EAGAIN) {
			/* retry */
			aos_msleep(10);
			continue;
		} else if (size == -EPIPE) {
			xrun(handle);
			continue;
		} else if (size == -ESTRPIPE) {

			continue;
		} else if (size < 0) {
			LOGD(TAG, "-----snd_pcm_readi failed!!, return %ld\n", size);
			ret = (int)size;
			goto err;
		}
		offset += (size * frame_bytes);
		frames_count += size;
		frames -= size;
		if (frames_total == frames_count)
			break;
		/*LOGD(TAG, "frames_count = %ld, frames_total = %ld\n", frames_count, frames_total);*/
	}
err:
	return frames_count > 0 ? frames_count : ret;
}

static int pcm_write(snd_pcm_t *handle, char *data, snd_pcm_uframes_t frames_total, unsigned int frame_bytes)
{
    // int ret = 0;
    snd_pcm_sframes_t size;
    snd_pcm_uframes_t frames_loop = 400;
    snd_pcm_uframes_t frames_count = 0;
    snd_pcm_uframes_t frames = 0;
    // int cnt = 0;

    while (1) {
        if ((frames_total - frames_count) < frames_loop)
            frames = frames_total - frames_count;
        if (frames == 0)
            frames = frames_loop;
        /*hal_usleep(500000);*/
        // LOGD(TAG, "###################write (%d)(%ld)\r\n", cnt++, frames);
        size = snd_pcm_writei(handle, data, frames);
        if (size != frames) {
            LOGD(TAG, "snd_pcm_writei return %ld\n", size);
        }
        if (size == -EAGAIN) {
            aos_msleep(10);
            continue;
        }
        else if (size == -EPIPE) {
            xrun(handle);
            continue;
        }
        else if (size == -ESTRPIPE) {
            continue;
        }
        else if (size < 0) {
            LOGD(TAG, "-----snd_pcm_writei failed!!, return %ld\n", size);
            return size;
        }
        data += (size * frame_bytes);
        frames_count += size;
        frames -= size;
        if (frames_total == frames_count)
            break;
        /*LOGD(TAG, "frames_count = %ld, frames_total = %ld\n", frames_count, frames_total);*/
    }

    return frames_count * frame_bytes;
}

typedef struct {
    snd_pcm_t *hdl;
} playback_t;

typedef struct {
    snd_pcm_t *hdl;
} capture_t;


int aos_dev_mute(aos_audio_pcm_device_t *pcm, aos_audio_snd_device_t device, int mute)
{
    return -1;
}

int aos_audio_pcm_setup(aos_audio_pcm_device_t *pcm)
{
    playback_t *p = (playback_t *)pcm->private_data;
    snd_pcm_format_t format; 

    int format_byte = 2;
    if(pcm->config.format == AOS_PCM_FORMAT_S16_LE) {
        format = SND_PCM_FORMAT_S16_LE;
        format_byte = 2;
    } else if(pcm->config.format == AOS_PCM_FORMAT_S32_LE) {
        format = SND_PCM_FORMAT_S32_LE;
        format_byte = 4;
    } else if(pcm->config.format == AOS_PCM_FORMAT_S8) {
        format = SND_PCM_FORMAT_S8;
        format_byte = 1;
    } else {
        format = SND_PCM_FORMAT_S16_LE;
        format_byte = 2;
    }

    snd_pcm_uframes_t frame_period_size = pcm->config.period_size / pcm->config.channels / format_byte;
    snd_pcm_uframes_t frame_buffer_size = frame_period_size  * pcm->config.period_count;

    int ret = set_param(p->hdl, format, pcm->config.rate, pcm->config.channels,
                        frame_period_size, frame_buffer_size);

    return ret;
}

int aos_audio_pcm_open(aos_audio_pcm_device_t *pcm)
{
    int ret = -1;

    /*pcm->card define @yoc_d1\components\minialsa\port\pcm_aos.c:pcm_open*/
    if (pcm->card == 3) {
        playback_t *p = aos_malloc(sizeof(playback_t));
        if (!p) {
            return ret;
        }
        ret = snd_pcm_open(&p->hdl, "default", SND_PCM_STREAM_PLAYBACK, 0);
        pcm->private_data = p;
    } else if(pcm->card == 0) {
        capture_t *p = aos_malloc(sizeof(capture_t));
        if (!p) {
            return ret;
        }
        ret = snd_pcm_open(&p->hdl, "snddmic", SND_PCM_STREAM_CAPTURE, 0);
        pcm->private_data = p;
    } else if(pcm->card == 1){
        snd_ctl_set_bynum("audiocodec", 22, 1); /* (LineINL)/MICIN1/FMINL -> ADC1*/
        snd_ctl_set_bynum("audiocodec", 23, 1); /* (LineINR)/MICIN2/FMINR -> ADC2*/
        snd_ctl_set_bynum("audiocodec", 19, 1); /* MIC3 */
        snd_ctl_set_bynum("audiocodec", 7, 31); /* MIC3 Gain */
    
        capture_t *p = aos_malloc(sizeof(capture_t));
        if (!p) {
            return ret;
        }
        ret = snd_pcm_open(&p->hdl, "audiocodec", SND_PCM_STREAM_CAPTURE, 0);
        pcm->private_data = p;
    }

    return ret;
}

int aos_audio_pcm_read(aos_audio_pcm_device_t *pcm, unsigned char *buffer, int nbytes)
{
    capture_t *p = (capture_t *)pcm->private_data;

	int ret = pcm_read(p->hdl, (char *)buffer,
			           snd_pcm_bytes_to_frames(p->hdl, nbytes),
			           snd_pcm_frames_to_bytes(p->hdl, 1));

    return snd_pcm_frames_to_bytes(p->hdl, ret);
}

int aos_audio_pcm_write(aos_audio_pcm_device_t *pcm, unsigned char *buffer, int nbytes)
{
    playback_t *p = (playback_t *)pcm->private_data;

    int ret = pcm_write(p->hdl, (char *)buffer,
                    snd_pcm_bytes_to_frames(p->hdl, nbytes),
                    snd_pcm_frames_to_bytes(p->hdl, 1));

    return ret;//snd_pcm_frames_to_bytes(p->hdl, ret);
}

int aos_audio_pcm_flush(aos_audio_pcm_device_t *pcm)
{
    snd_pcm_t *hdl;

    if (!pcm->dir) {
        playback_t *p = (playback_t *)pcm->private_data;
        hdl = p->hdl;
    } else {
        capture_t *p = (capture_t *)pcm->private_data;
        hdl = p->hdl;
    }

    snd_pcm_drain(hdl);

    return 0;
}

int aos_audio_pcm_close(aos_audio_pcm_device_t *pcm)
{
    snd_pcm_t *hdl;

    if (!pcm->dir) {
        playback_t *p = (playback_t *)pcm->private_data;
        hdl = p->hdl;
    } else {
        capture_t *p = (capture_t *)pcm->private_data;
        hdl = p->hdl;
    }

    int ret = snd_pcm_close(hdl);
    aos_free(pcm->private_data);

    return ret;
}

int aos_audio_pcm_wait(aos_audio_pcm_device_t *pcm, int timeout)
{
    // snd_pcm_t *hdl;

    if (!pcm->dir) {
        // playback_t *p = (playback_t *)pcm->private_data;
        // hdl = p->hdl;
        /* TODO */
    } else {
        // capture_t *p = (capture_t *)pcm->private_data;
        // hdl = p->hdl;
        /* aos_audio_pcm_read will block, 
        ignore the function implementation 
        */
    }

    return 0;
}

int aos_audio_pcm_pause(aos_audio_pcm_device_t *pcm, int enable)
{
    snd_pcm_t *hdl;

    if (!pcm->dir) {
        playback_t *p = (playback_t *)pcm->private_data;
        hdl = p->hdl;
    } else {
        capture_t *p = (capture_t *)pcm->private_data;
        hdl = p->hdl;
    }
    
    return snd_pcm_pause(hdl, enable);
}

static void amixer_ctl_info_print(snd_ctl_info_t *info)
{
	if (info->type == SND_CTL_ELEM_TYPE_INTEGER) {
		LOGD(TAG, "numid=%u, name=\'%s\'\n", info->id, info->name);
		LOGD(TAG, "\t value=%lu, min=%u, max=%u\n",
			info->value, info->min, info->max);
	} else if (info->type == SND_CTL_ELEM_TYPE_ENUMERATED) {
		LOGD(TAG, "numid=%u, name=\'%s\'\n", info->id, info->name);
		LOGD(TAG, "\t value=%s, enum=", info->texts[info->value]);
		int i = 0;
		for (i = 0; i < info->items; i++) {
			LOGD(TAG, "%s ", info->texts[i]);
		}
		LOGD(TAG, "\n");
	} else {
		LOGD(TAG, "numid=%u, name=\'%s\' type error:%d\n",
			info->id, info->name, info->type);
	}
}

int aos_audio_set_volume(aos_audio_snd_device_t device, int volume)
{
    int numid = 1;
    snd_ctl_info_t info;

    if (volume >= 0) {
        if (snd_ctl_get_bynum("audiocodec", numid, &info) != 0) {
            LOGD(TAG, "snd_ctl_get failed\n");
            return -1;
        }
        int value = info.max - volume*(info.max-info.min)/100;
        snd_ctl_set_bynum("audiocodec", numid, value);

        if (!snd_ctl_get_bynum("audiocodec", numid, &info)) {
            amixer_ctl_info_print(&info);
        }
        return 0;
    }

    return -1;  
}

int aos_audio_dev_mute(aos_audio_pcm_device_t *pcm, aos_audio_snd_device_t device, int mute)
{
    return -1;
}

int aos_audio_pcm_init(void)
{
    return 0;
}

#endif


