//
// Created by krazh on 08.11.17.
//

#ifndef LOGDB_LOGDB_C_CPP_H
#define LOGDB_LOGDB_C_CPP_H



#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================================================
// Уровни сообщений лога(типы)
typedef enum
{
    ERROR =    1,
    WARNING =  2,
    INFO =     3,
    DEBUG =    4
} LOG_LEVELS;



//=============================================================================================================
//  область, к которой относится данныое сообщение лога(кабинет, офд, меню, фн и тд)
typedef enum
{
    REG_COMMON =            0,

    REG_CONFIG =            1,
    REG_DATABASE =          2,
    REG_FN =                3,
    REG_GOODS =             4,
    REG_JSON =              5,
    REG_KABINET =           6,
    REG_LCD =               7,
    REG_MENU =              8,
    REG_NET =               9,
    REG_OFD =              10,
    REG_RECEIPT =          11,
    REG_REMOTE_CONTROL =   12,
    REG_TESTS =            13,
    REG_THREADS =          14,
    REG_USERS =            15,
    REG_WIFI =             16,
    REG_SCANNER =          17,
    REG_PRINTER =          18,
    REG_KEYPAD =           19,
    REG_TIME   =           20,
    REG_FD   =             21,
    REG_PULSE   =          22,

    OTHER =               100
}LOG_REGIONS;


//=============================================================================================================
// Интерфейс C

#include <stdio.h>
#include <stdarg.h>

// Вывод в лог сообщения. По факту сообщение добавляется в очередь для последующей записи в БД
void logINFO_c(LOG_REGIONS region, const char *const fmt, ...);
void logWARN_c(LOG_REGIONS region, const char *const fmt, ...);
void logERR_c (LOG_REGIONS region, const char *const fmt, ...);
void logDBG_c (LOG_REGIONS region, const char *const fmt, ...);

// Задать/Узнать уровень лога
void            setLogLevel_c ( LOG_LEVELS           lvl );
LOG_LEVELS      getLogLevel_c                           ();

// Задать Имя БД лога
void         setDBName_c( const char          *name);

// Узнать/Задать макс размер длины очереди сообщений на запись в БД
void         setMaxQuerySize_c(unsigned int   size );
unsigned int getMaxQuerySize_c                    ();

// Узнать/Задать период записи в базу очереди сообщений
void         setWriteDBPeriod_c(unsigned int usPer );
unsigned int getWriteDBPeriod_c                   ();

// Узнать/Задать макс число записей в БД
void         setMaxDBSize_c(unsigned int        sz );
unsigned int getMaxDBSize_c                       ();

// Задать кодировку входного текста
void   setCode_CP866_c();
void   setCode_UTF8_c ();
//===================================================================================



#ifdef __cplusplus
}
#endif

//C++
// Запуск асинхронной записи в БД. Должен быть запущен в отд потоке. Зациклен внутри. Пишет очередь в БД
void runLogDaemon();
// Стоп логгер
void stopLogDaemon();

#endif //LOGDB_LOGDB_C_CPP_H
