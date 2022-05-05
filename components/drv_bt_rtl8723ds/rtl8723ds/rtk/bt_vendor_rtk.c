/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Realtek Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bt_vendor_rtk.c
 *
 *  Description:   Realtek vendor specific library implementation
 *
 ******************************************************************************/

#undef NDEBUG
#define LOG_TAG "bt_vendor_uart"

#include <stdio.h>

#include <ulog/ulog.h>
#include <aos/kernel.h>

#include "userial_vendor.h"

#define TAG "hci_vendor"

void change_2_h4()
{
    int size;
    uint8_t buffer[40];
    uint8_t snyc_req[] = {0xc0, 0x00, 0x2f, 0x00, 0xd0, 0x01, 0x7e, 0xc0};
    uint8_t snyc_rsp[] = {0xc0, 0x00, 0x2f, 0x00, 0xd0, 0x02, 0x7D, 0xc0};
    // uint8_t conf_req[] = {0xc0, 0x00, 0x2f, 0x00, 0xd0, 0x03, 0xFC, 0x14, 0xc0};
    uint8_t cong_rsq[] = {0xc0, 0x00, 0x2f, 0x00, 0xd0, 0x04, 0x7B, 0xc0};

    uint8_t h5_2_h4[] = {0xC0, 0xDB, 0xDC, 0x01, 0x01, 0x3D, 0x56, 0xFC, 0x0D, 0x01, 0x50, 0xC5, 0xEA, 0x19, 0xE0, 0x1B, 0xF1, 0xAF, 0x58, 0x01, 0xA4, 0x0B, 0x7E, 0x3E, 0xC0};
    uint8_t h5_2_h4_conf[] = {0xc0, 0x08, 0x00, 0x00, 0xf7, 0xc0};


    userial_vendor_send_data(snyc_req, sizeof(snyc_req));
sync:
    size = userial_vendor_recv_data(buffer, 8, 1000);

    printf("recv: %d\n", size);
    for(int i = 0; i < size; i++) {
        printf("0x%02x ", buffer[i]);
    }
    printf("\n");

    if (size != 8) {
        goto sync;
    }

    size = userial_vendor_recv_data(buffer, 8, 1000);

    if (size != 8) {
        goto sync;
    }
    printf("recv: %d\n", size);
    for(int i = 0; i < size; i++) {
        printf("0x%02x ", buffer[i]);
    }
    printf("\n");

    userial_vendor_send_data(snyc_rsp, sizeof(snyc_rsp));

    // userial_vendor_send_data(conf_req, sizeof(conf_req));

    size = userial_vendor_recv_data(buffer, 8, 1000);

    if (size != 8) {
        goto sync;
    }
    printf("recv: %d\n", size);
    for(int i = 0; i < size; i++) {
        printf("0x%02x ", buffer[i]);
    }
    printf("\n");

    userial_vendor_send_data(cong_rsq, sizeof(cong_rsq));

    userial_vendor_send_data(h5_2_h4, sizeof(h5_2_h4));

    size = userial_vendor_recv_data(buffer, 35, 1000);

    if (size != 35) {
        goto sync;
    }
    printf("recv: %d\n", size);
    for(int i = 0; i < size; i++) {
        printf("0x%02x ", buffer[i]);
    }
    printf("\n");

    userial_vendor_send_data(h5_2_h4_conf, sizeof(h5_2_h4_conf));

    userial_vendor_set_parity(0);
}