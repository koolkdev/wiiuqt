#include "blocks0to1.h"
#include "tools.h"

Blocks0to1::Blocks0to1( const QList<QByteArray> &blocks )
{
    _ok = false;
    if( !blocks.isEmpty() )
        SetBlocks( blocks );
}

bool Blocks0to1::SetBlocks( const QList<QByteArray> &blocks )
{
    _ok = false;
    quint16 cnt = blocks.size();
    if( cnt != 2 )
        return false;
    for( quint16 i = 0; i < cnt; i++ )
    {
        if( blocks.at( i ).size() != 0x20000 )
        {
            qWarning() << "Blocks0to1::SetBlocks -> block" << i << "is" << hex << blocks.at( i ).size() << "bytes";
            return false;
        }
    }
    this->blocks = blocks;
    _ok = true;
    return true;
}

bool Blocks0to1::CheckBoot1()
{
    if( blocks.size() != 2 )
    {
        qWarning() << "Blocks0to1::CheckBoot1 -> not enough blocks" << blocks.size();
        return false;
    }
    if ( blocks.at(0) != blocks.at(1) ){
        qWarning() << "Blocks0to1::CheckBoot1 -> Boot0 block 0 and block 1 not identical!";
        return false;
    }

    Boot1Info boot1Info;
    QByteArray stuff = blocks.at( 0 );
    QBuffer b( &stuff );
    b.open(QIODevice::ReadOnly);
    b.read( (char*)&boot1Info, sizeof(boot1Info) );
    boot1Info.unknownSignatureRelated = qFromBigEndian(boot1Info.unknownSignatureRelated);
    boot1Info.zero = qFromBigEndian(boot1Info.zero);
    boot1Info.unknownType = qFromBigEndian(boot1Info.unknownType);
    boot1Info.rsaKeyIndex = qFromBigEndian(boot1Info.rsaKeyIndex);
    boot1Info.boot1Size = qFromBigEndian(boot1Info.boot1Size);
    boot1Info.unknownNumber = qFromBigEndian(boot1Info.unknownNumber);

    if ( boot1Info.boot1Size % 0x1000 || boot1Info.boot1Size > 0xF000 ) {
        qWarning() << "Blocks0to1::CheckBoot1 -> Invalid boot1 size" << boot1Info.boot1Size;
        return false;
    }

    if ( boot1Info.rsaKeyIndex != 2 ) {
        qWarning() << "Blocks0to1::CheckBoot1 -> Invalid RSA key index" << boot1Info.rsaKeyIndex;
        return false;
    }

    if ( boot1Info.zero ) {
        qWarning() << "Blocks0to1::CheckBoot1 -> Should be zero" << boot1Info.zero;
        return false;
    }

    if ( boot1Info.unknownType != 0x21 || boot1Info.unknownType != 0x21 ) {
        qWarning() << "Blocks0to1::CheckBoot1 -> Invalid something type" << boot1Info.unknownType;
        return false;
    }

    // TODO: check RSA?

    QByteArray boot1Data = b.read( boot1Info.boot1Size );
    QByteArray hash = GetSha1( boot1Data );

    if ( hash != QByteArray::fromRawData( (const char*)boot1Info.boot1Hash, 0x14) ) {
        qWarning() << "Blocks0to1::CheckBoot1 -> Wrong hash" << hash.toHex() << QByteArray::fromRawData( (const char*)boot1Info.boot1Hash, 0x14).toHex();
        return false;        
    }
    qWarning() << "Boot1 hash:" << QByteArray::fromRawData( (const char*)boot1Info.boot1Hash, 0x14).toHex();

    return true;
}