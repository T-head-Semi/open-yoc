#include <stdio.h>
#include <string.h>
#include <aos_pcm.h>
#include <alsa/pcm.h>
#include <alsa/mixer.h>
#include <stdlib.h>

static int hw_params_set(aos_pcm_t *pcm, aos_pcm_hw_params_t *hw_params)
{
    aos_audio_pcm_device_t *p = (aos_audio_pcm_device_t *)(pcm->hdl);

    p->config.rate = hw_params->rate;
    p->config.channels = hw_params->channels;
    p->config.period_size = hw_params->period_bytes;
    p->config.period_count = hw_params->buffer_bytes / hw_params->period_bytes;
    if (hw_params->format == 16) {
        p->config.format = AOS_PCM_FORMAT_S16_LE;
    } else if(hw_params->format == 32){
        p->config.format = AOS_PCM_FORMAT_S32_LE;
    }

    p->dir = (pcm->stream == AOS_PCM_STREAM_PLAYBACK)? AOS_AUDIO_PCM_DIR_OUT : AOS_AUDIO_PCM_DIR_IN;
    aos_audio_pcm_open(pcm->hdl);
    
    return aos_audio_pcm_setup(p);
}

static int pcm_open(aos_pcm_t *pcm)
{
    pcm->hdl = calloc(sizeof(aos_audio_pcm_device_t), 1);
    if(!pcm->hdl) {
        return -1;
    }

    if(strcmp(pcm->name, "pcmP0") == 0) {
        ((aos_audio_pcm_device_t *)pcm->hdl)->card = 3;
    } else if(strcmp(pcm->name, "pcmC0") == 0) {
        ((aos_audio_pcm_device_t *)pcm->hdl)->card = 0;
    }  else if(strcmp(pcm->name, "pcmC1") == 0) {
        ((aos_audio_pcm_device_t *)pcm->hdl)->card = 1;
    } else {
        free(pcm->hdl);
        return -1;
    }

    return 0;
}

static int pcm_read(aos_pcm_t *pcm, void *buffer, int nbytes)
{
    aos_audio_pcm_device_t *p = (aos_audio_pcm_device_t *)(pcm->hdl);

    return aos_audio_pcm_read(p, (unsigned char*)buffer, nbytes);
}   

static int pcm_write(aos_pcm_t *pcm, void *buffer, int nbytes)
{
    aos_audio_pcm_device_t *p = (aos_audio_pcm_device_t *)(pcm->hdl);

    return aos_audio_pcm_write(p, (unsigned char*)buffer, nbytes);
}

static int pcm_flush(aos_pcm_t *pcm)
{
    aos_audio_pcm_device_t *p = (aos_audio_pcm_device_t *)(pcm->hdl);
    aos_audio_pcm_flush(p);
    return 0;
}

static int pcm_close(aos_pcm_t *pcm)
{
    aos_audio_pcm_device_t *p = (aos_audio_pcm_device_t *)(pcm->hdl);
    
    aos_audio_pcm_close(p);
    free(p);
    return 0;
}

static int pcm_wait(aos_pcm_t *pcm, int timeout)
{
    aos_audio_pcm_device_t *p = (aos_audio_pcm_device_t *)(pcm->hdl);
    
    return aos_audio_pcm_wait(p, timeout);
}

static int pcm_pause(aos_pcm_t *pcm, int enable)
{
    aos_audio_pcm_device_t *p = (aos_audio_pcm_device_t *)(pcm->hdl);

    aos_audio_pcm_pause(p, enable);
    return 0;
}

aos_pcm_ops_t g_pcm_ops = {
    .open = pcm_open,
    .hw_params_set = hw_params_set,
    .pause = pcm_pause,
    .write = pcm_write,
    .read = pcm_read,
    .close = pcm_close,
    .wait = pcm_wait,
    .flush = pcm_flush,
};

static int set_volume(aos_mixer_elem_t *elem, int l_value, int r_value)
{
    aos_audio_set_volume(AOS_SND_DEVICE_OUT_SPEAKER, l_value);
    return 0;
}

sm_elem_ops_t g_mixer_ops = {
    .set_volume = set_volume,
};

