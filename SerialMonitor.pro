QT -= gui
QT += serialport

CONFIG += c++17 console
CONFIG -= app_bundle

LIBS += -L/opt/target/sysroots/aarch64-tdx-linux/usr/lib/

target.path = /data

INSTALLS += target

SOURCES += \
        main.cpp

