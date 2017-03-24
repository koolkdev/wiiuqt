#-------------------------------------------------
#
# Project created by QtCreator 2010-12-17T16:22:53
#
#-------------------------------------------------

#QT       += core\
#	    gui

TARGET = nandFixer
CONFIG   += console
CONFIG   -= app_bundle
DEFINES += NAND_BIN_CAN_WRITE

TEMPLATE = app

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += main.cpp \
    ../WiiQt/blocks0to1.cpp \
    ../WiiQt/nandbin.cpp \
    ../WiiQt/tools.cpp \
    ../WiiQt/aes.c \
    ../WiiQt/sha1.c \
    ../WiiQt/nandspare.cpp

HEADERS += ../WiiQt/tiktmd.h \
    ../WiiQt/nandbin.h \
    ../WiiQt/tools.h \
    ../WiiQt/blocks0to1.h \
    ../WiiQt/nandspare.h

