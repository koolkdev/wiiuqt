#-------------------------------------------------
#
# Project created by QtCreator 2010-12-17T16:22:53
#
#-------------------------------------------------

#QT       += core\
#	    gui

TARGET = nandBinCheck
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += main.cpp \
    ../WiiUQt/blocks0to1.cpp \
    ../WiiUQt/nandbin.cpp \
    ../WiiUQt/tools.cpp \
    ../WiiUQt/aes.c \
    ../WiiUQt/sha1.c \
    ../WiiUQt/nandspare.cpp

HEADERS += ../WiiUQt/nandbin.h \
    ../WiiUQt/tools.h \
    ../WiiUQt/blocks0to1.h \
    ../WiiUQt/nandspare.h
