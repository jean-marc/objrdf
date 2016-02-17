#ifndef LOG_INFO_H
#define LOG_INFO_H
//would be nice to define per namespace

#ifndef LOG_EMERGENCY
#define LOG_EMERGENCY cerr<<"<0>"
#endif

#ifndef LOG_ALERT
#define LOG_ALERT cerr<<"<1>"
#endif

#ifndef LOG_CRITICAL
#define LOG_CRITICAL cerr<<"<2>"
#endif

#ifndef LOG_ERROR
#define LOG_ERROR cerr<<"<3>"
#endif

#ifndef LOG_WARNING
#define LOG_WARNING cerr<<"<4>"
#endif

#ifndef LOG_NOTICE
#define LOG_NOTICE cerr<<"<5>"
#endif

#ifndef LOG_INFO
#define LOG_INFO if(0) cerr<<"<6>"
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG if(0) cerr<<"<7>"
#endif

#endif

