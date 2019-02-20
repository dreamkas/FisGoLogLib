//
// Created by krazh on 03.11.17.
//

#include "logdb.h"
#include "logdb_c_cpp.h"

using namespace std;


unsigned int   loggerDBSize = 0;    // буфер для хранения размера БД(кол-во записей)
mutex mutexLogDB;                   // глобапльный мутекс обращения к БД лога


#define MAX_MESS_SIZE  65536

char fmt_new[MAX_MESS_SIZE],
        mess[MAX_MESS_SIZE];     // Буфер для сообщений


//===================================================================================
// Конструктор с дефолтныйми значениями
Log_DB::Log_DB()
{
    _work.store(true);
    dB_Name = "./logDb.db";     // Имя БД лога
    maxQuerySize = 1000;         // Макс число сообщений, ожидающих запись в БД лога

    maxDBSize = 20000;          // макс количество записей в БД лога

    writeDBPeriod = 500000;        // Период записи в БД лога сообщений в микросекундах

    logLevel = LOG_LEVELS::WARNING;

    // Чистка очереди сообщений под мутексом
    mutexQuery.lock();
    messagesQuery.clear();
    mutexQuery.unlock();
}


//===================================================================================
// Деструктор
Log_DB::~Log_DB()
{
    // Чистка очереди сообщений под мутексом
    mutexQuery.lock();
    messagesQuery.clear();
    mutexQuery.unlock();
}

//===================================================================================
// log_ERR
bool Log_DB::log_ERR (const LOG_REGIONS region, string  strMess )
{
    //cout << "_2_ >>>>>>>>>>>>> log_ERR :: START! strMess = |" << fmt << "|" << endl;
    return _log_in_sql(LOG_LEVELS::ERROR, region, strMess);
}


//===================================================================================
// log_WARN
bool Log_DB::log_WARN (const LOG_REGIONS region, string  strMess )
{
    //cout << "_2_ >>>>>>>>>>>>> log_WARN :: START! strMess = |" << strMess << "|" << endl;
    return _log_in_sql(LOG_LEVELS::WARNING, region, strMess);

}


//===================================================================================
// log_INFO
bool Log_DB::log_INFO (const LOG_REGIONS region,  string  strMess )
{
    //cout << "_2_ >>>>>>>>>>>>> log_INFO :: START! strMess = |" << strMess << "|" << endl;
    return _log_in_sql(LOG_LEVELS::INFO, region, strMess);
}


//===================================================================================
// log_DBG
bool Log_DB::log_DBG (const LOG_REGIONS region,  string  strMess )
{
    //cout << "_2_ >>>>>>>>>>>>> log_DBG :: START! strMess = |" << fmt << "|" << endl;
    return _log_in_sql(LOG_LEVELS::DEBUG, region, strMess);
}


//===================================================================================
// Метод, который должен быть зпущен в отд. потоке, будет писать сообщения в БД из очереди(вектора) сообщений. Зациклен.
void Log_DB::logDaemon()
{
    int qSize = 0;
    cout << "logDaemon():: START" << endl;
    _work.store(true);
    while(_work.load())
    {
        qSize = messagesQuery.size();
        // Если есть сообщения на запись в БД
        if( qSize > 0 )
        {
            // Проверяем есть ли БД лога
            if(!_isLogDBExist())
            {
                //cout << "logDaemon()::  _createLoggerTable()..." << endl;
                // Пытаемся создать БД лога
                if( !_createLoggerTable() )
                {
                    cout << "logDaemon()::  Error! Can't create table in log DB. Start regeneration!" << endl;
                    string strRm = "rm -r " + dB_Name;
                    cout << "logDaemon():: regeneration! rm -r=" << strRm << endl;
                    system(strRm.c_str());
                    cout << "logDaemon():: regeneration! sync..." << endl;
                    system("sync");
                    cout << "logDaemon():: regeneration! sleep..." << endl;
                    sleep(2);
                    cout << "logDaemon():: regeneration! rm -r continue!" << endl;

                    continue;
                }
                else
                {
                    cout << "logDaemon()::  DATABASE CREATED! name = '" << logger.getDBName() << "'" << endl;
                }
            }
            // Запись в БД лога одного сообщения
            if( _writeMessQToDB())
            {
                // Обновляем инфу о размере БД
                _sizeOfLogDB();
                if(loggerDBSize > maxDBSize)
                {
                    if( !_deleteFromLogDB ( (int)(maxDBSize / 2) ) )
                    {
                        _log_in_sql(LOG_LEVELS ::ERROR, LOG_REGIONS::REG_DATABASE, "CANT DELETE RECORDS!");
                    }
                }
            }
            else // Ошибка при записи
            {
                cout << "logDaemon():: Error! Can't write mess'" << messagesQuery.front().mess << "' in log DB" << endl;
            }

        }// if
        else
        {
            //cout << " logDaemon():: No Messages" << endl;
        }
        this_thread::sleep_for(chrono::microseconds(writeDBPeriod));
        break;
    }// while
}




//===================================================================================
//===================================================================================
//=============  PRIVATE МЕТОДЫ               =======================================
//===================================================================================
//===================================================================================

//===================================================================================
//callbackLogger - Функция обработки ответа от БД на sql запрос

static int _callbackLogger(void *data, int argc, char **argv, char **azColName)
{
    //cout << " callbackLogger:: Start" << endl;
    string          strLoggerSize;          // Строка с записанным размером БД

    for(int i = 0; i < argc; i++)
    {
        string column = _charToString((char *)(azColName[i]));

        // Если запрашиваем число записей
        if(column.compare("COUNT(*)") == 0)
        {
            strLoggerSize = _charToString((char *)(argv[i] ? argv[i] : "NULL"));
            if(strLoggerSize.compare("NULL") == 0)
            {
                loggerDBSize = 0;
                cout << " _callbackLogger():: loggerDBSize = NULL" << endl;
            }
            else
            {
                loggerDBSize = atoi(strLoggerSize.c_str());
            }

        }
        // Если Парсим записи
        string value = _charToString((char *)(argv[i] ? argv[i] : "NULL"));
    }
    return 0;
}


//===================================================================================
// Выполнение запросов к БД (открытие+запрос+получение ответа)
bool Log_DB::_makeLoggerRequest()
{
    int rc = 0;
    char *zErrMsg = 0;

    if (dB_Name.empty())
    {
        cout << " _makeLoggerRequest():: Error! Database name is empty!" << endl;
    }
    //open loggerDB
    rc = sqlite3_open( dB_Name.c_str(), &loggerDb);

    if( rc )
    {
        cout << " _makeLoggerRequest():: Error! CAN'T OPEN '"<< dB_Name
             << "' ERR MESS = "              <<  sqlite3_errmsg(loggerDb)
             << endl;
        return false;
    }
    else
    {
        //cout << "sqlRequest.c_str() = " << sqlRequest.c_str() << endl;
    }
    // Выполнение запроса к БД
    rc = sqlite3_exec(loggerDb, sqlRequest.c_str(), _callbackLogger, (void*)this, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        cout << " _makeLoggerRequest():: Error! SQL ERROR: " << endl
             << zErrMsg                  << endl
             << sqlite3_errmsg(loggerDb) << endl
             << rc                       << endl ;

        sqlite3_free(zErrMsg) ;
        return false;
    }
    sqlite3_close(loggerDb);
    if(zErrMsg != nullptr)
    {
        sqlite3_free(zErrMsg);
    }
    return true;
}


//===================================================================================
// Метод создания таблицы логгера
bool Log_DB::_createLoggerTable()
{
    mutexLogDB.lock();
    sqlRequest = "CREATE TABLE LOG("
            "ID        INTEGER PRIMARY KEY AUTOINCREMENT,"  // Ключ ID(счетчик)
            "DT        datetime default current_timestamp," // ДАТАВРЕМЯ сообщения
            "MESS          TEXT,"                           // тело сообщения
            "LVL        INTEGER,"                           // Уровень(инфо, ошибка...)
            "REGION     INTEGER);";                         // Область(ФН, Дисплей и тд)
    // Выполнение запроса
    if( !_makeLoggerRequest() )
    {
        cout << "_createLoggerTable()::FAILED TO CREATE TABLE LOG!" << endl;
        mutexLogDB.unlock();
        return false;
    }
    mutexLogDB.unlock();
    // -----------------------------------

    return true;
}


//=================================================
//Метод добавления сообщения в БД лога
bool Log_DB::_writeMessQToDB( )
{
    // Если нет очереди сообщений - просто выходим
    if(messagesQuery.empty())
    {
        return true;
    }
    // -----------------------------------
    mutexLogDB.lock();
    mutexQuery.lock();
    sqlRequest.clear();
    // Формирование SQL запроса из очереди сообщений
    sqlRequest = "INSERT INTO LOG(MESS,LVL,REGION) VALUES(";
    for( unsigned int i =0; i < messagesQuery.size(); i++)
    {
        sqlRequest +=   "'" + messagesQuery.at(i).mess                      + "',"
                        + to_string(messagesQuery.at(i).levelOfMess)        + ","
                        + to_string(messagesQuery.at(i).regionOfMess);
        // Завершение запроса
        sqlRequest +=   ((i+1) < messagesQuery.size())
                        ? "),("
                        : ");" ;
    }
    messagesQuery.clear();
    mutexQuery.unlock();
    // Выполнение запроса
    if( !_makeLoggerRequest() )
    {
        cout << " _writeMessQToDB: Failed to insert in  LOG query" << endl;
        mutexLogDB.unlock();

        sqlRequest.clear();
        return false;
    }
    sqlRequest.clear();
    mutexLogDB.unlock();
    // -----------------------------------
    return true;
}


//===================================================================================
// Метод удаления из БД лога nRecords старых записей(защита от роста)
bool Log_DB::_deleteFromLogDB   (int nRecords)
{
    if(nRecords < 0 || nRecords > 32000)
    {
        return false;
    }
    // -----------------------------------
    // Оборачиваем мутексом обращение к БД
    mutexLogDB.lock();
    sqlRequest = "DELETE FROM LOG WHERE ID IN ( SELECT ID FROM LOG DESC LIMIT " + to_string(nRecords) + ");";

    if( !_makeLoggerRequest() )// Выполнение запроса
    {
        cout << " _deleteFromLogDB: ERROR!!!! " << endl;
        mutexLogDB.unlock();
        return false;
    }
    mutexLogDB.unlock();
    // -----------------------------------
    // Актуализируем размер БД
    _sizeOfLogDB();

    return true;
}


//===================================================================================
// Метод, возвращающий размер таблицы лога
int Log_DB::_sizeOfLogDB()
{
    // -----------------------------------
    // Оборачиваем мутексом обращение к БД
    mutexLogDB.lock();
    sqlRequest = "SELECT COUNT(*) FROM LOG;";
    if( !_makeLoggerRequest() )// Выполнение запроса
    {
        cout << " _sizeOfLogDB: ERROR " << endl;
        mutexLogDB.unlock();
        return -1;
    }
    mutexLogDB.unlock();
    // -----------------------------------
    if (loggerDBSize > 0)
    {
        //cout << " _sizeOfLogDB:CONFIG DB SIZE IS = " << loggerDBSize << endl;
    }
    else
    {
        cout << " Log DB SIZE IS = 0, It's emply!" << endl;
    }

    return loggerDBSize;
}


//===================================================================================
// Метод, проверки существует ли  таблица(БД) логгера
bool Log_DB::_isLogDBExist()
{
    if(_sizeOfLogDB() > 0)
    {
        //cout << " _isLogDBExist: TRUE, SIZE > 0" << endl;
        return true;
    }
    else
    {
        cout << " Log DB Not Exist, SIZE = 0" << endl;
        return false;
    }

}


//===================================================================================
// Перевод из char* в String
string _charToString(const char *source)
{
    int size = strlen(source);
    string str(source, size);
    return str;
}


int                pInt    =    0;
EncodeConvertor    ecStr;
bool               isCP866 = true;  // Флаг того, что текст поступает в логгер в 866 кодировке

//===================================================================================
// Обработка строки для безопасного помещения в sql запрос
string _prepareMess(string &sourceStr )
{
    char *safeMess;
    mutexLogDB.lock();
    if (isCP866)
    {
        pInt = 0;
        sourceStr = ecStr.CP866toUTF8(sourceStr, &pInt);
    }
    safeMess =  sqlite3_mprintf("%q", sourceStr.c_str());
    string tmp     = _charToString(safeMess);
    sqlite3_free(safeMess);
    mutexLogDB.unlock();
    return tmp;
}

void Log_DB::setTermColor(LOG_LEVELS lvl)
{
    switch (lvl)
    {
        case ERROR:
        {
            cout << termcolor::on_red;
            break;
        }
        case WARNING:
        {
            cout << termcolor::underline << termcolor::yellow;
            break;
        }
        case INFO:
        {
            cout << termcolor::blue;
            break;
        }
        case DEBUG:
        {
            cout << termcolor::cyan;
            break;
        }
        default:
        {
            cout << termcolor::reset;
            break;
        }
    }
}
//===================================================================================
// Запись ИНФО сообщения в лог(по факту добавление в очередь для последующего логгирования)
bool Log_DB::_log_in_sql( LOG_LEVELS lvl, LOG_REGIONS region, string mess )
{
    if(!_work.load())
    {
        return true;
    }
    //cout << "_log_in_sql: query size = " <<  messagesQuery.size() << endl;
    // Если размер очереди на запись в БД равен максимальному, то не добавляем новое сообщение
    if( messagesQuery.size() >= maxQuerySize )
    {
        cout << "Log_DB::_log_in_sql(): ERROR!!! Too much messages in Query!" << endl;
        return false;
    }
    // Формируем структуру одной записи(1-го сообщения, его уровень и область, к которой относится сообщение)
    LOG_MESSAGE                       tmpMess;
    tmpMess.mess         = _prepareMess(mess);
    tmpMess.regionOfMess =             region;
    tmpMess.levelOfMess  =                lvl;

    // Добавляем запись в очередь под мутексом
    mutexQuery.lock();
    messagesQuery.push_back(tmpMess);
    // вывод сообщения на экран

    setTermColor(lvl);
    cout << tmpMess.mess << termcolor::reset << " " << endl;
    mutexQuery.unlock();

    return true;
}

void Log_DB::stopLogger()
{
    _work.store(false);
}

//===================================================================================
//===================================================================================
//===================================================================================
//===================================================================================
//===================================================================================
//===================================================================================
//============================        DAEMON        =================================
//===================================================================================
//===================================================================================
//===================================================================================

//===================================================================================
// Запуск асинхронной записи в БД
void runLogDaemon()
{
    logger.logDaemon();
}

void stopLogDaemon()
{
    logger.stopLogger();
}

//===================================================================================
//===================================================================================
//============================   C  ИНТЕРФЕЙСЫ    ===================================
//===================================================================================
//===================================================================================
#ifdef __cplusplus
extern "C" {
#endif

//===================================================================================
void logINFO_c (LOG_REGIONS  region, const char *const  fmt, ... )
{
    if(logger.getLogLevel()< LOG_LEVELS::INFO)
        {return;}
    va_list args;
    va_start(args, fmt);
    vsnprintf ( mess, MAX_MESS_SIZE - 1, fmt, args );
    va_end(args);
    logger.log_INFO(region, mess );
}


//===================================================================================
//
void logWARN_c (LOG_REGIONS  region, const char *const  fmt, ... )
{
    if(logger.getLogLevel()< LOG_LEVELS::WARNING)
        {return;}
    va_list args;
    va_start(args, fmt);
    vsnprintf ( mess, MAX_MESS_SIZE - 1, fmt, args );
    va_end(args);
    logger.log_WARN(region, mess );
}


//===================================================================================
//
void logERR_c (LOG_REGIONS  region, const char *const  fmt, ... )
{
    if(logger.getLogLevel()< LOG_LEVELS::ERROR)
        {return;}
    va_list args;
    va_start(args, fmt);
    vsnprintf ( mess, MAX_MESS_SIZE - 1, fmt, args );
    va_end(args);
    logger.log_ERR(region, mess );
}


//===================================================================================
//
void logDBG_c (LOG_REGIONS  region, const char *const  fmt, ... )
{
    if(logger.getLogLevel()< LOG_LEVELS::DEBUG)
        {return;}
//    sprintf( fmt_new,
//             "%.*s",
//             MAX_MESS_SIZE-2,  fmt );

    va_list args;
    va_start(args, fmt);
//    vsprintf ( mess,fmt_new, args );
    vsnprintf ( mess, MAX_MESS_SIZE - 1, fmt, args );
    //cout << " logDBG_c():: Mess to LOGGER: size = " << sizeof(mess) << " mess=|" << mess << "|" << endl;
    va_end(args);
    logger.log_DBG(region, mess );
}


//===================================================================================
// Задать/Узнать уровень лога
void          setLogLevel_c (LOG_LEVELS lvl)     {   logger.setLogLevel(lvl);    printf("--------> setLogLevel_c():: LOG LEVEL = %d\n", logger.getLogLevel());      };
LOG_LEVELS getLogLevel_c                  ()     {   return logger.getLogLevel();      };


//===================================================================================
// Задать кодировку входного текста
void   setCode_CP866_c() {isCP866 = true;  };
void   setCode_UTF8_c()  {isCP866 = false; };

//===================================================================================
// Узнать/Задать Имя БД лога
void   setDBName_c(const char      *name) {logger.setDBName( _charToString(name) ); };
//string getDBName_c                () { return logger.getDBName();                   };


//===================================================================================
// Узнать/Задать макс размер длины очереди сообщений на запись в БД
void         setMaxQuerySize_c(unsigned int size) { logger.setMaxQuerySize(size);    };
unsigned int getMaxQuerySize_c                 () { return logger.getMaxQuerySize(); };


//===================================================================================
// Узнать/Задать период записи в базу очереди сообщений
void     setWriteDBPeriod_c(unsigned usPer) { logger.setWriteDBPeriod(usPer);       };
unsigned getWriteDBPeriod_c            ()   { return logger.getWriteDBPeriod();     };


//===================================================================================
// Узнать/Задать макс число записей в БД
void         setMaxDBSize_c(unsigned sz) { logger.setMaxDBSize(sz);                 };
unsigned int getMaxDBSize_c           () { return logger.getMaxDBSize();            };


#ifdef __cplusplus
}
#endif

/*
//===================================================================================
//===================================================================================
//========================== C++ ====================================================
//===================================================================================
//===================================================================================
//
void logINFO_cpp (const char *fmt, ... )
{
    va_list args;
    va_start(args, fmt);


    logger.log_INFO(LOG_REGIONS::DATABASE_REG,fmt, args);
    va_end(args);
}


//===================================================================================
//
void logWARN_cpp (const char *const  fmt, ... )
{
    va_list args;
    va_start(args, fmt);
    logger.log_WARN(LOG_REGIONS::DATABASE_REG,fmt, args);
    va_end(args);
}


//===================================================================================
//
void logERR_cpp (const char *const  fmt, ... )
{
    va_list args;
    va_start(args, fmt);
    logger.log_ERR(LOG_REGIONS::DATABASE_REG,fmt, args);
    va_end(args);
}


//===================================================================================
//
void logDBG_cpp (const char *const  fmt, ... )
{
    va_list args;
    va_start(args, fmt);
    logger.log_DBG(LOG_REGIONS::DATABASE_REG,fmt, args);
    va_end(args);
}
 */

/*
 * Лог с внутренним форматированием
 bool Log_DB::log_INFO (LOG_REGIONS region , const char *fmt, ... )
{
    cout << ">>>>>>>>>>>>> log_INFO :: START!" << endl;
    //-----------------------------------------
    // Сформировать новую строку формата
    char fmt_new[512];
    sprintf( fmt_new,
             "%.*s\n",
             256,  fmt );
    cout << ">>>>>>>>>>>>> log_INFO :: fmt = '" << fmt << "'" << endl;
    cout << ">>>>>>>>>>>>> log_INFO :: fmt_new = '" << fmt_new << "'" << endl;
//    sprintf( fmt_new,
//             "%.s\n", fmt );
    //-----------------------------------------
    char mess[1024];
    va_list args;
    va_start( args, fmt );
    cout << "*************************************************************" << endl;
    cout << "*************************************************************" << endl;
    cout << "log_INFO vprintf( fmt, args): ";
    vprintf( fmt, args);
    cout << endl;
    cout << "*************************************************************" << endl;
    cout << "*************************************************************" << endl;
    cout << "vsprintf result = '" << vsprintf ( mess,fmt_new, args ) << "'" << endl;// mess = "loader_()::DBG i = 144"
    va_end( args );
    //-----------------------------------------
    string strMess = mess;
cout << "_2_ >>>>>>>>>>>>> log_INFO :: START! strMess = |" << fmt << "|" << endl;
string strMess = fmt;
return _log_in_sql(LOG_LEVELS::INFO, region, strMess);
}


*/
