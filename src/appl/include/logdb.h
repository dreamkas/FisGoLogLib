//
// Created by krazh on 03.11.17.
//

#ifndef LOGDB_LOGDB_H
#define LOGDB_LOGDB_H

// Created by krazh on 02.10.17.
//
// 4 уровня логгирования(выбор INFO, WARNING, ERROR, DEBUG)
// Сообщения так же делятся по регионам
// запись в 1 БД
// отдельный поток на демон
// Динамический размер буфера(сообщение о заполнении буфера на запись в БД о заполнении)
// В перспективе, доп плюшки:
// (логгирование списка процессов? Исп Память?), фиксировать пики потребления более Х %?
// Архивирование старого лога?
//
// ФУНКЦИИ
// Задать уровень логгирования (ERROR, WARNING, INFO, DEBUG)
// Добавить сообщение в очередь с указанием региона и уровня
// Задать макс глубину БД(макс кол-во записей). Если записей более МАКС, то подчищаем старые (МАКС-(МАКС/х))

#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <sqlite3/sqlite3.h>
#include <string.h>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

#include "logdb_c_cpp.h" // Для доступа к энамам
#include "EncodeConvertor.h"
#include "termcolor/termcolor.hpp"

using namespace std;


//=============================================================================================================
// Конвертер из char в string
string _charToString(const char *source);


//=============================================================================================================
// Структура одной записи лога(для очереди)
struct LOG_MESSAGE
{
    LOG_LEVELS levelOfMess;         // Уровень сообщения
    LOG_REGIONS regionOfMess;       // область, к которой относится сообщение
    string mess;                    // тело сообщения
};


//=============================================================================================================
// Класс логгер
// В логгер сообщения попадают посредством очереди структур типа "Сообщение" messagesQuery,
// формируемой вызовами извне

class Log_DB
{

private:
    atomic<bool> _work;             // Работа логгера
    //---------------
    // Мутексы
    mutex          mutexQuery;      // Мутекс на доступ к очереди(вектору) на запись в БД
    //---------------
    sqlite3          *loggerDb;     // Указатель на БД
    string          sqlRequest;     // Строка SQL-запроса в БД
    string             dB_Name;     // Имя  БД лога
    LOG_LEVELS        logLevel;     // Уровень лога(что пишем, а что нет)
    unsigned int  maxQuerySize;     // макс длина очереди лога(для защиты от переполнения памяти)
    unsigned int     maxDBSize;     // макс количество записей в БД лога
    uint64_t maxRowDbSize = 10000;  // Максимальное количество строк в базе лога
    unsigned int writeDBPeriod;     // Период записи сообщений в БД лога в микросекундах

    vector<LOG_MESSAGE> messagesQuery;    // Очередь сообщений, ожидающих запись в БД лога

    // Запись сообщения в БД лога (по факту добавление в очередь для последующего логгирования)
    bool _log_in_sql(LOG_LEVELS lvl, LOG_REGIONS region, string mess);

    //---------------
    // Работа с БД
    bool _makeLoggerRequest();                  // Выполнение запросов к БД (открытие+запрос+получение ответа)

    bool _createLoggerTable();                 // Метод создания таблицы логгера
    bool _isLogDBExist();                      // Метод, проверки существует ли  таблица(БД) логгера
    int _countOfTables();                      // Метод, возвращающий количество БД логгера
    bool _writeMessQToDB();                    // Метод добавления сообщений из очереди в БД лога
    int _sizeOfLogDB();                        // Метод, возвращающий размер таблицы лога
    bool _deleteFromLogDB(int nRecords);       // Метод удаления из БД лога nRecords старых записей(защита от роста)
    //---------------
public:

     Log_DB();
    ~Log_DB();


    // Узнать/Задать Имя БД лога
    void   setDBName(string name) { dB_Name = name; };
    string getDBName()            { return dB_Name; };

    // Узнать/Задать макс размер длины очереди сообщений на запись в БД
    void setMaxQuerySize(unsigned int size) { maxQuerySize = size; }
    unsigned int getMaxQuerySize() { return maxQuerySize; }

    // Узнать/Задать период записи в базу очереди  сообщений
    void setWriteDBPeriod(unsigned usPer) { writeDBPeriod = usPer; };
    unsigned getWriteDBPeriod() { return writeDBPeriod; };

    // Узнать/Задать макс число записей в БД
    void setMaxDBSize(unsigned sz) { maxDBSize = sz; };
    unsigned int getMaxDBSize() { return maxDBSize; };

    // Узнать/Задать Уровень лога
    void setLogLevel(LOG_LEVELS ll) { logLevel = ll; };
    LOG_LEVELS getLogLevel() { return logLevel; };

    // Готовый интерфейс логгера с форматированным вводом
    bool log_ERR (const LOG_REGIONS region,  string  strMess );
    bool log_WARN(const LOG_REGIONS region,  string  strMess );
    bool log_INFO(const LOG_REGIONS region,  string  strMess );
    bool log_DBG (const LOG_REGIONS region,  string  strMess );

    //------------------------------------------------------------------------
    // Метод daemon, который должен быть зпущен в отд. потоке,
    // будет писать сообщения в БД из очереди(вектора) сообщений. Зациклен.
    void logDaemon();
    //------------------------------------------------------------------------
    /**
     * @brief stopLogger - выключить логгер
     */
    void stopLogger();
    /**
     * @brief setTermColor - установка цвета вывода в терминал
     * @param lvl - уровень лога
     */
    void setTermColor(LOG_LEVELS lvl);
    /**
     * @brief setMaxRowDbSize установка максимального количества строк в базе лога
     * @param value максимальное количество строк в базе лога
     */
    void setMaxRowDbSize(const uint64_t &value) {maxRowDbSize = value;}
    /**
     * @brief getMaxRowDbSize получить максимальное количество строк в базе лога
     * @return максимальное количество строк в базе лога
     */
    uint64_t getMaxRowDbSize() {return maxRowDbSize;}
};

// Экземпляр класса логгера
static Log_DB logger;

#endif //LOGDB_LOGDB_H
