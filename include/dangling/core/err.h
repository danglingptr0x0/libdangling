#ifndef LDG_CORE_ERR_H
#define LDG_CORE_ERR_H

// generic (0-99)
#define LDG_ERR_AOK              0
#define LDG_ERR_FUNC_ARG_NULL    1
#define LDG_ERR_FUNC_ARG_INVALID 2
#define LDG_ERR_ALLOC_NULL       3
#define LDG_ERR_NOT_INIT         4
#define LDG_ERR_FULL             5
#define LDG_ERR_OVERFLOW         6
#define LDG_ERR_EMPTY            7
#define LDG_ERR_BUSY             8
#define LDG_ERR_INVALID          9
#define LDG_ERR_TIMEOUT          10

// mem (100-199)
#define LDG_ERR_MEM_BAD            100
#define LDG_ERR_MEM_STR_TRUNC      101
#define LDG_ERR_MEM_MEMMOVE_ALLOCD 102
#define LDG_ERR_MEM_SENTINEL       103
#define LDG_ERR_MEM_POOL_FULL      104
#define LDG_ERR_MEM_POOL_INVALID   105

// io (200-299)
#define LDG_ERR_IO_NOT_FOUND 200
#define LDG_ERR_IO_READ      201
#define LDG_ERR_IO_WRITE     202
#define LDG_ERR_IO_FORMAT    203

// time (300-399)
#define LDG_ERR_TIME_CORE_MIGRATED  300
#define LDG_ERR_TIME_NOT_CALIBRATED 301

// net (400-499)
#define LDG_ERR_NET_INIT    400
#define LDG_ERR_NET_PERFORM 401
#define LDG_ERR_NET_TIMEOUT 402
#define LDG_ERR_NET_CONN    403

// str (500-599)
#define LDG_ERR_STR_TRUNC   500
#define LDG_ERR_STR_OVERLAP 501

// audio (600-699)
#define LDG_ERR_AUDIO_INIT             600
#define LDG_ERR_AUDIO_NOT_INIT         601
#define LDG_ERR_AUDIO_NOT_AVAILABLE    602
#define LDG_ERR_AUDIO_STREAM_NOT_FOUND 603
#define LDG_ERR_AUDIO_SINK_NOT_FOUND   604
#define LDG_ERR_AUDIO_SOURCE_NOT_FOUND 605
#define LDG_ERR_AUDIO_NO_DEFAULT       606
#define LDG_ERR_AUDIO_DUCK_FULL        607
#define LDG_ERR_AUDIO_DUCK_EMPTY       608
#define LDG_ERR_AUDIO_VOLUME_RANGE     609

// errlog
#define LDG_ERRLOG_LVL_ERR  0
#define LDG_ERRLOG_LVL_WARN 1
#define LDG_ERRLOG_LVL_INFO 2

#ifndef LDG_ERRLOG_HANDLER
#define LDG_ERRLOG_HANDLER(lvl, msg) ((void)0)
#endif

#define LDG_ERRLOG(lvl, msg) LDG_ERRLOG_HANDLER((lvl), (msg))

#define LDG_ERRLOG_ERR(msg)  LDG_ERRLOG(LDG_ERRLOG_LVL_ERR, (msg))
#define LDG_ERRLOG_WARN(msg) LDG_ERRLOG(LDG_ERRLOG_LVL_WARN, (msg))
#define LDG_ERRLOG_INFO(msg) LDG_ERRLOG(LDG_ERRLOG_LVL_INFO, (msg))

#endif
