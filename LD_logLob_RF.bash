#Скрипт загрузки библиотеки оггера прямо в проект fiscat и на кассу Дримкас РФ(Касса Ф)
#разработчика
#!/bin/bash

if ! [ -e cmake-build-release/liblogDB.so ];
then
    echo "Failed to find liblogDB.so!"
    echo "Deploy aborted..."
    exit 2;
fi

if ! [ -d src/appl/include ];
then
    echo "Includes not found!"
    echo "Deploy aborted..."
    exit 2;
fi

if ! [ -d /usr/local/usr/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/lib/ ];
then
    echo "/usr/local/usr/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/lib/ directory not found";
    echo "Deploy aborted..."
    exit 2;
fi

md5sum cmake-build-release/liblogDB.so \
    > cmake-build-release/ethalonLib_logDB_MD5

sudo cp cmake-build-release/liblogDB.so /usr/local/usr/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/lib/
if ! [ $? -eq 0 ];
then
    echo "Failed to patch compiler!"
    echo "Deploy aborted..."
    exit 2;
fi

if ! [ -d ../FisGo/liblogDB/ ];
then
    mkdir -p ../FisGo/liblogDB/
fi

cp src/appl/include/logdb_c_cpp.h ../FisGo/liblogDB/
if ! [ $? -eq 0 ];
then
    echo "Failed to patch FisGo(liblogDB.so) project!"
    echo "Deploy aborted..."
    exit 2;
fi

echo "Do you need to patch device? y/n"
read need

if [ $need = "y" ];
then
    echo "Please, insert device ip!"
    read deviceIp;

    scp cmake-build-release/liblogDB.so root@$deviceIp:/lib/
    scp cmake-build-release/ethalonLib_logDB_MD5 root@$deviceIp:/FisGo/

    if ! [ $? -eq 0 ];
    then
        echo "Failed to patch device!"
        echo "DEPLOY FINISHED WITH ERRORS!"
        exit 1;
    fi
fi

echo "DEPLOY FINISHED CORRECT!"

exit 0