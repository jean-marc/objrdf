#ifndef LOG_INFO_H
#define LOG_INFO_H
/*
 *	we could also use a journal function to add more information
 *	http://0pointer.de/blog/projects/journal-submit.html
 */

#ifndef LOG_EMERGENCY
#define LOG_EMERGENCY if(verbosity>=0) cerr<<"<0>"<<_context_<<":"
#endif

#ifndef LOG_ALERT
#define LOG_ALERT if(verbosity>=1) cerr<<"<1>"<<_context_<<":"
#endif

#ifndef LOG_CRITICAL
#define LOG_CRITICAL if(verbosity>=2) cerr<<"<2>"<<_context_<<":"
#endif

#ifndef LOG_ERROR
#define LOG_ERROR if(verbosity>=3) cerr<<"<3>"<<_context_<<":"
#endif

#ifndef LOG_WARNING
#define LOG_WARNING if(verbosity>=4) cerr<<"<4>"<<_context_<<":"
#endif

#ifndef LOG_NOTICE
#define LOG_NOTICE if(verbosity>=5) cerr<<"<5>"<<_context_<<":"
#endif

#ifndef LOG_INFO
#define LOG_INFO if(verbosity>=6) cerr<<"<6>"<<_context_<<":"
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG if(verbosity>=7) cerr<<"<7>"<<_context_<<":"
#endif

#endif

