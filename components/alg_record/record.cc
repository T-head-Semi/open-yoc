/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <memory>
#include <time.h>
#include <stdint.h>
#include <aos/kernel.h>
#include <aos/cli.h>
#include <ulog/ulog.h>
#include <aos/ringbuffer.h>
#include <yoc/record.h>
#include <yoc/mic.h>
#include <cxvision/cxvision.h>

#define TAG "record"

#define HELP_USAGE                                                                         \
    "record start/stop ws://192.168.50.216:8090 micdata.pcm [200000(rbsize) 1(reccount)] " \
    "[1(3->5)]"

#define MAX_MIC_REC_COUNT 3
#define CHUNK_SIZE        (5120) // 5K

using RecordMessageT = posto::Message<thead::voice::proto::RecordMsg>;

typedef struct _rec_mgr {
    dev_ringbuf_t ringbuffer;
    char *        rbuffer;
    aos_event_t   wait_evt;
    rec_hdl_t     hdl;
    /* debug info */
    uint32_t  total_rec;
    uint32_t  rb_min_left;
    long long last_print_ms;
    uint32_t  total_drop;
} rec_mgr_t;

static rec_mgr_t *                                    g_rec_mgrs[MAX_MIC_REC_COUNT];
static std::shared_ptr<posto::Participant>            participant = NULL;
static std::shared_ptr<posto::Writer<RecordMessageT>> writer      = NULL;

static void data_ready(void *arg)
{
    unsigned   flags   = 0;
    rec_mgr_t *rec_mgr = (rec_mgr_t *)arg;

    // LOGD(TAG, "read wait...");
    aos_event_get(&rec_mgr->wait_evt, 0x01, AOS_EVENT_OR_CLEAR, &flags, AOS_WAIT_FOREVER);
}

void data_release(void *arg)
{
    rec_mgr_t *rec_mgr = (rec_mgr_t *)arg;
    aos_event_set(&rec_mgr->wait_evt, 0x01, AOS_EVENT_OR);
}

#ifdef __cplusplus
extern "C" {
#endif

void rec_copy_data(int index, uint8_t *data, uint32_t size)
{
    if (index < MAX_MIC_REC_COUNT) {
        rec_mgr_t *rec_mgr = g_rec_mgrs[index];
        if (rec_mgr && rec_mgr->hdl && size > 0) {

            uint32_t write_space = ringbuffer_available_write_space(&rec_mgr->ringbuffer);
            if (write_space > size) {
                ringbuffer_write(&rec_mgr->ringbuffer, data, size);

                /* Debug info */
                rec_mgr->total_rec += size;
                if (write_space < rec_mgr->rb_min_left) {
                    rec_mgr->rb_min_left = write_space;
                } else if (rec_mgr->rb_min_left == 0) {
                    rec_mgr->rb_min_left = write_space;
                }

            } else {
                // LOGW(TAG, "buffer full, drop %d", size);
                rec_mgr->total_drop += size;
            }

            // if (ringbuffer_available_read_space(&rec_mgr->ringbuffer) >= CHUNK_SIZE) {
            aos_event_set(&rec_mgr->wait_evt, 0x01, AOS_EVENT_OR);
            // }

            /* Debug info print */
            if (aos_now_ms() - rec_mgr->last_print_ms > 5000) {
                LOGW(TAG, "total rec=%u rb min left=%u drop=%u\r\n", rec_mgr->total_rec,
                     rec_mgr->rb_min_left, rec_mgr->total_drop);
                rec_mgr->last_print_ms = aos_now_ms();
            }
        }
    }
}

void rec_start(const char *url, const char *save_name, int rb_size, int rec_cnt)
{
    for (int i = 0; i < rec_cnt; i++) {
        rec_mgr_t *rec_mgr = (rec_mgr_t *)aos_zalloc_check(sizeof(rec_mgr_t));

        rec_mgr->rbuffer = (char *)aos_malloc_check(rb_size + 1);
        ringbuffer_create(&rec_mgr->ringbuffer, rec_mgr->rbuffer, rb_size + 1);
        aos_event_new(&rec_mgr->wait_evt, 0);
        char buf[64];
        snprintf(buf, sizeof(buf), "ringbuffer://handle=%lu", (size_t)(&rec_mgr->ringbuffer));
        char   buf2[128];
        time_t tt = time(NULL);
        if (i == 0) {
            snprintf(buf2, sizeof(buf2), "%s/%06ld_%s_%s", url, tt, "mic", save_name);
        } else if (i == 1) {
            snprintf(buf2, sizeof(buf2), "%s/%06ld_%s_%s", url, tt, "ssp", save_name);
        } else if (i == 2) {
            snprintf(buf2, sizeof(buf2), "%s/%06ld_%s_%s", url, tt, "proc", save_name);
        }

        rec_hdl_t hdl = record_register(buf, buf2);
        record_set_data_ready_cb(hdl, data_ready, (void *)rec_mgr);
        record_set_data_release_cb(hdl, data_release, (void *)rec_mgr);
        record_set_chunk_size(hdl, rb_size);
        record_start(hdl);
        rec_mgr->hdl  = hdl;
        g_rec_mgrs[i] = rec_mgr;
        LOGD(TAG, "start mic rec [%d]\n", i);
    }

    if (participant == NULL) {
        participant = posto::Domain::CreateParticipant("cmd_producer");
        writer      = participant->CreateWriter<RecordMessageT>("RecordMsg");
    }

    auto msg = std::make_shared<RecordMessageT>();

    msg->body().set_cmd(thead::voice::proto::START);

    writer->Write(msg);
}

void rec_stop(void)
{
    if (participant == NULL) {
        return;
    }

    auto msg = std::make_shared<RecordMessageT>();

    msg->body().set_cmd(thead::voice::proto::STOP);

    writer->Write(msg);
    for (int i = 0; i < MAX_MIC_REC_COUNT; i++) {
        rec_mgr_t *rec_mgr = g_rec_mgrs[i];
        if (rec_mgr == NULL) {
            continue;
        }

        record_stop(rec_mgr->hdl);
        aos_event_free(&rec_mgr->wait_evt);
        aos_free(rec_mgr->rbuffer);
        ringbuffer_destroy(&rec_mgr->ringbuffer);
        record_unregister(rec_mgr->hdl);
        rec_mgr->hdl = NULL;
        aos_free(rec_mgr);
        g_rec_mgrs[i] = NULL;
        LOGD(TAG, "mic rec stop over.");
    }
}

static void cmd_record_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc < 2) {
        return;
    }

    /* record start ws://192.168.50.216:8090 micdata.pcm [200000(rbsize) 1(reccount)] [1(3->5)] */
    if (strcmp(argv[1], "start") == 0) {
        if (argc >= 4) {
            int rec_cnt = 0;
            if (argc == 5) {
                rec_cnt = atoi(argv[4]);
            } else {
                rec_cnt = 1;
            }
            rec_start(argv[2], argv[3], 400 * 1024, rec_cnt);

            printf("mic rec start.\n");
            return;
        }
    } else if (strcmp(argv[1], "stop") == 0) {

        rec_stop();

        printf("mic rec stop\n");
        return;
    } else {
        printf("%s\n", HELP_USAGE);
    }
    return;
}

void cli_reg_cmd_record(void)
{
    static const struct cli_command cmd_info = {"record", "record pcm", cmd_record_func};

    aos_cli_register_command(&cmd_info);
}

#ifdef __cplusplus
}
#endif