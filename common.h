#ifndef COMMON_SERVER_HEADER
#define COMMON_SERVER_HEADER

#ifdef LOG_ON
#define PRINT_LOG(...) printf("[LOG]: " __VA_ARGS__)
#else
#define PRINT_LOG(...) 
#endif

#define PRINT_ERR(...) printf("[ERROR]: " __VA_ARGS__)

#ifdef MULTITHREAD

#define MULTITHREAD_ON(...) __VA_ARGS__
#define BACKLOG_COUNT 16

#else

#define BACKLOG_COUNT 1
#define MULTITHREAD_ON(...) 

#endif

#endif /* COMMON_SERVER_HEADER */
