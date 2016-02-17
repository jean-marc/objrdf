#ifndef LOG_INFO_H
#define LOG_INFO_H
//would be nice to define per namespace

#ifndef LOG_EMERGENCY
#define LOG_EMERGENCY cerr
#endif

#ifndef LOG_ALERT
#define LOG_ALERT cerr
#endif

#ifndef LOG_CRITICAL
#define LOG_CRITICAL cerr
#endif

#ifndef LOG_ERROR
#define LOG_ERROR cerr
#endif

#ifndef LOG_WARNING
#define LOG_WARNING cerr
#endif

#ifndef LOG_NOTICE
#define LOG_NOTICE cerr
#endif

#ifndef LOG_INFO
#define LOG_INFO if(0) cerr
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG if(0) cerr
#endif

#endif

