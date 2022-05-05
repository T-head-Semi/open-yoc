
/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

extern int aui_nlp_init(void);
extern void cli_reg_cmd_aui(void);

int app_aui_nlp_init(void)
{
    cli_reg_cmd_aui();
    return aui_nlp_init();
}
