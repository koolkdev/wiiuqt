#ifndef BLOCKS0TO1_H
#define BLOCKS0TO1_H

#include "includes.h"


struct Boot1Info
{
    quint8 unknownHeader[0x20];
    quint32 unknownSignatureRelated; // 2? maybe rsa key number
    quint8 signature[0x100]; // rsa signature on sha1 of 0x60 bytes, after the signature padding until the end of the struct
    quint8 signaturePadding[0x7c]; // zeros
    quint32 zero; // must be zero!
    quint32 unknownType; // should be 0x21 (can be also 0x22? and than unknownNumber not checked)
    quint32 rsaKeyIndex; // should be 2 for retail
    quint32 boot1Size;
    quint8 boot1Hash[0x14]; // sha1 hash of encrypted boot1
    quint32 unknownNumber; // checked against something
    quint8 padding[0x38]; // zeros
} __attribute__((packed));


class Blocks0to1
{
public:
    Blocks0to1( const QList<QByteArray> &blocks = QList<QByteArray>() );
    bool SetBlocks( const QList<QByteArray> &blocks );
    bool IsOk(){ return _ok; }

    bool CheckBoot1();

private:
    bool _ok;
    //should hold the blocks, without ecc
    QList<QByteArray>blocks;
};

#endif // BLOCKS0TO1_H
