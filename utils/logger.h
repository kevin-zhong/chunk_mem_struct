#ifndef _XDRIVE_LOGGER_H_20110421
#define _XDRIVE_LOGGER_H_20110421

#ifdef _LOG_USE_LOG4CPLUS   //use log4cpluse..

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>

#define SH_DECL_LOGGER(_logger) static log4cplus::Logger _logger;
#define SH_IMPL_LOGGER(classname, _logger) log4cplus::Logger classname::_logger = log4cplus::Logger::getInstance(# classname);
#define SH_TEMPLATE_IMPL_LOGGER(classname, _logger) template<class T> log4cplus::Logger classname<T>::_logger = log4cplus::Logger::getInstance(# classname);
#define SH_TEMPLATE_IMPL_LOGGER_2(classname, logger) template<class T, class T2> log4cplus::Logger classname<T, T2>::logger = log4cplus::Logger::getInstance(# classname);
#define SH_TEMPLATE_IMPL_LOGGER_3(classname, logger) template<class T, class T2, class T3> log4cplus::Logger classname<T, T2, T3>::logger = log4cplus::Logger::getInstance(# classname);


#define SH_LOG_TRACE(e) LOG4CPLUS_TRACE(_logger, e)
#define SH_LOG_DEBUG(e) LOG4CPLUS_DEBUG(_logger, e)
#define SH_LOG_INFO(e)    LOG4CPLUS_INFO(_logger, e)
#define SH_LOG_WARN(e)  LOG4CPLUS_WARN(_logger, e)
#define SH_LOG_ERROR(e) LOG4CPLUS_ERROR(_logger, e)
#define SH_LOG_FATAL(e) LOG4CPLUS_FATAL(_logger, e)

#define LLOG_TRACE(_logger, e) LOG4CPLUS_TRACE(_logger, e)
#define LLOG_DEBUG(_logger, e) LOG4CPLUS_DEBUG(_logger, e)
#define LLOG_INFO(_logger, e)    LOG4CPLUS_INFO(_logger, e)
#define LLOG_WARN(_logger, e)  LOG4CPLUS_WARN(_logger, e)
#define LLOG_ERROR(_logger, e) LOG4CPLUS_ERROR(_logger, e)
#define LLOG_FATAL(_logger, e) LOG4CPLUS_FATAL(_logger, e)

#define _SH_LOG_DEBUG(logger, e) LOG4CPLUS_DEBUG(logger, e)

#elif defined(_LOG_USE_STDOUT) //use simple stdout..
#include <sstream>
#include <time.h>
#include <sys/time.h>

#define _LOGGER_SYS_OUT(_logger, e, tag)  \
        do { \
                std::ostringstream oss; \
                oss << e; \
                char buff[64]; struct tm stm; struct timeval tv; struct timezone tz; \
                gettimeofday(&tv, &tz); \
                strftime(buff, sizeof(buff), "%y%m%d %T", localtime_r(&tv.tv_sec, &stm)); \
                fprintf(stdout, "[%s.%06ld][%s:%d] [%s] %s\n", buff, tv.tv_usec, __FILE__, __LINE__, tag, oss.str().c_str()); \
                fflush(stdout); \
        } while (0);

#define SH_DECL_LOGGER(_logger)
#define SH_IMPL_LOGGER(classname, _logger)
#define SH_TEMPLATE_IMPL_LOGGER(classname, _logger)
#define SH_TEMPLATE_IMPL_LOGGER_2(classname, logger)
#define SH_TEMPLATE_IMPL_LOGGER_3(classname, logger)


#define SH_LOG_TRACE(e)   _LOGGER_SYS_OUT(std::cout, e, "TRACE");
#define SH_LOG_DEBUG(e)  _LOGGER_SYS_OUT(std::cout, e, "DEBUG");
#define SH_LOG_INFO(e)   _LOGGER_SYS_OUT(std::cout, e, "INFO");
#define SH_LOG_WARN(e)   _LOGGER_SYS_OUT(std::cerr, e, "WARN");
#define SH_LOG_ERROR(e)  _LOGGER_SYS_OUT(std::cerr, e, "ERROR");
#define SH_LOG_FATAL(e)  _LOGGER_SYS_OUT(std::cerr, e, "FATAL");

#define LLOG_TRACE(_logger, e)   _LOGGER_SYS_OUT(std::cout, e, "TRACE");
#define LLOG_DEBUG(_logger, e)  _LOGGER_SYS_OUT(std::cout, e, "DEBUG");
#define LLOG_INFO(_logger, e)   _LOGGER_SYS_OUT(std::cout, e, "INFO");
#define LLOG_WARN(_logger, e)   _LOGGER_SYS_OUT(std::cerr, e, "WARN");
#define LLOG_ERROR(_logger, e)  _LOGGER_SYS_OUT(std::cerr, e, "ERROR");
#define LLOG_FATAL(_logger, e)  _LOGGER_SYS_OUT(std::cerr, e, "FATAL");

#define _SH_LOG_DEBUG(logger, e) SH_LOG_DEBUG(e)

#else

#define SH_DECL_LOGGER(_logger)
#define SH_IMPL_LOGGER(classname, _logger)
#define SH_TEMPLATE_IMPL_LOGGER(classname, _logger)

#define SH_LOG_TRACE(e)
#define SH_LOG_DEBUG(e)
#define SH_LOG_INFO(e)
#define SH_LOG_WARN(e)
#define SH_LOG_ERROR(e)
#define SH_LOG_FATAL(e)

#define LLOG_TRACE(_logger, e)
#define LLOG_DEBUG(_logger, e)
#define LLOG_INFO(_logger, e)
#define LLOG_WARN(_logger, e)
#define LLOG_ERROR(_logger, e)
#define LLOG_FATAL(_logger, e)


#define _SH_LOG_DEBUG(logger, e)

#endif

#ifdef  _DEBUG
#define DEBUG_LOG_DECL(_logger) SH_DECL_LOGGER(_logger)
#define DEBUG_LOG_IMPL(classname, _logger) SH_IMPL_LOGGER(classname, _logger)
#define DEBUG_LOG_IMPL_TPL(classname, _logger) SH_TEMPLATE_IMPL_LOGGER(classname, _logger)
#define DEBUG_LOG(e) SH_LOG_DEBUG(e)
#else
#define DEBUG_LOG_DECL(_logger)
#define DEBUG_LOG_IMPL(classname, _logger)
#define DEBUG_LOG_IMPL_TPL(classname, _logger)
#define DEBUG_LOG(e)
#endif

template<typename TCls>
class  LoggerTpl2
{
protected:
        SH_DECL_LOGGER(_logger);
};

SH_TEMPLATE_IMPL_LOGGER(LoggerTpl2, _logger);

template<typename TCls>
class  LoggerTpl
{
protected:
        SH_DECL_LOGGER(__logger);
};

SH_TEMPLATE_IMPL_LOGGER(LoggerTpl, __logger);


#endif  //
