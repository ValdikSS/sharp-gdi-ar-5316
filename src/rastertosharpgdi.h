#ifndef _RASTERTOSHARPGDI_H_
#define _RASTERTOSHARPGDI_H_

#include <stdint.h>

#define SHARP_MANUAL_FEED               261
#define SHARP_TRAY1                     257
#define SHARP_TRAY_AUTO                 7

#define SHARP_PB_CHUNK                  32764

#define SHARP_BEGIN_JOB                 0x0800
#define SHARP_BEGIN_JOB_DESCRIPTION     0x0200
#define SHARP_BEGIN_PAGE_DESCRIPTION    0x0500
#define SHARP_BEGIN_RASTER              0x1000
#define SHARP_RASTER_DATA               0x0100
#define SHARP_END_RASTER_DATA           0x0600
#define SHARP_END_JOB                   0x0300
#define SHARP_FINISHED                  0x0900

typedef struct __attribute__ ((__packed__)) sharp_cmd_s {
    uint16_t magic;
    uint16_t length;
    uint16_t cmd;
} sharp_cmd_t;

typedef struct sharp_paper_code_s {
    char *paper_name;
    int paper_code;
} sharp_paper_code_t;

sharp_paper_code_t sharp_paper_codes[] = {
    {"Letter", 1},
    {"Ledger", 2},
    {"Legal", 5},
    {"Statement", 6},
    {"Executive", 7},
    {"A3", 8},
    {"A4", 9},
    {"A5", 11},
    {"B4", 12},
    {"B5", 13},
    {"Folio", 14},
    {"Env10", 20},
    {"EnvDL", 27},
    {"EnvC5", 28},
    {"A6", 70},
    {"EnvChou3", 73},
    {"B6", 88},
    {"PRC16K", 93},
    {"PRC32K", 280}
};

#endif