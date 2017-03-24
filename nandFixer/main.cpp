#include "../WiiUQt/includes.h"
#include "../WiiUQt/nandbin.h"
#include "../WiiUQt/tools.h"

//yippie for global variables
QStringList args;
NandBin nand;
QTreeWidgetItem *root;
QList<quint16> fats;

void Usage()
{
    qWarning() << "usage:" << QCoreApplication::arguments().at( 0 ) << "input_nand.bin output_nand.bin";
    exit( 1 );
}

void Fail( const QString& str )
{
    qCritical() << str;
    exit( 1 );
}

QTreeWidgetItem *FindItem( const QString &s, QTreeWidgetItem *parent )
{
    int cnt = parent->childCount();
    for( int i = 0; i <cnt; i++ )
    {
        QTreeWidgetItem *r = parent->child( i );
        if( r->text( 0 ) == s )
        {
            return r;
        }
    }
    return NULL;
}

QTreeWidgetItem *ItemFromPath( const QString &path )
{
    QTreeWidgetItem *item = root;
    if( !path.startsWith( "/" ) || path.contains( "//" ))
    {
        return NULL;
    }
    int slash = 1;
    while( slash )
    {
        int nextSlash = path.indexOf( "/", slash + 1 );
        QString lookingFor = path.mid( slash, nextSlash - slash );
        item = FindItem( lookingFor, item );
        if( !item )
        {
            //if( verbose )
            //	qWarning() << "ItemFromPath ->item not found" << path;
            return NULL;
        }
        slash = nextSlash + 1;
    }
    return item;
}

QString PathFromItem( QTreeWidgetItem *item )
{
    QString ret;
    while( item )
    {
        ret.prepend( "/" + item->text( 0 ) );
        item = item->parent();
        if( item->text( 0 ) == "/" )// dont add the root
            break;
    }
    return ret;

}

void FixEcc()
{
    for( quint16 i = 0; i < 0x8000; i++ )
    {
        for( quint8 j = 0; j < 8; j++ )
        {
            quint32 page = ( i * 8 ) + j;
            if( !nand.FixEcc( page ) )
            {
                Fail("Failed to fix ECC");
            }
        }
    }
}

void SetUpTree()
{
    if( root )
        return;
    QTreeWidgetItem *r = nand.GetTree();
    if( r->childCount() != 1 || r->child( 0 )->text( 0 ) != "/" )
    {
        Fail( "The nand FS is seriously broken.  I Couldn't even find a correct root" );
    }

    root = r->takeChild( 0 );
    delete r;
}

int RecurseCountFiles( QTreeWidgetItem *dir )
{
    int ret = 0;
    quint16 cnt = dir->childCount();
    for( quint16 i = 0; i < cnt; i++ )
    {
        QTreeWidgetItem *item = dir->child( i );
        if( item->text( 7 ).startsWith( "02" ) )
            ret += RecurseCountFiles( item );
        else
        {
            ret++;
        }
    }
    return ret;
}

void RecurseFixHmac( QTreeWidgetItem *dir )
{
    quint16 cnt = dir->childCount();
    for( quint16 i = 0; i < cnt; i++ )
    {
        QTreeWidgetItem *item = dir->child( i );
        if( item->text( 7 ).startsWith( "02" ) )
            RecurseFixHmac( item );
        else
        {
            bool ok = false;
            quint16 entry = item->text( 1 ).toInt( &ok );
            if( !ok )
                continue;

            if( !nand.FixHmacData( entry ) )
            {
                Fail( "Failed to fix HMAC data" );
            }
        }
    }
}

void FixHmac()
{
    quint16 total = RecurseCountFiles( root );
    qDebug() << "fixing hmac for" << total << "files";
    RecurseFixHmac( root );
    qDebug() << "fixing HMAC for superclusters...";
    QList<quint16> sclBad;
    for( quint16 i = nand.GetFirstSuperblockCluster(); i < 0x8000; i += 0x10 )
    {
        nand.FixHmacMeta( i );
    }
}

int main( int argc, char *argv[] )
{
    QCoreApplication a( argc, argv );
// qInstallMessageHandler( DebugHandler );

    args = QCoreApplication::arguments();

    qCritical() << "** nandFixer : Fix incomplete dump, add ECCs and HMACs **";
    qCritical() << "   built:" << __DATE__ << __TIME__;

    if( args.size() < 3 )
        Usage();

    NandBin check;
    QFile input( args.at( 1 ) );
    if ( !input.exists() || !check.SetPath( args.at( 1 ) ) || !check.InitNand() ) {
        Fail( "Failed to load input NAND!" );
    }
    if( !input.open( QIODevice::ReadOnly ) )
        Fail( "Failed to open input NAND!" );

    if( input.size() != 0x20000000 )
        Fail( "Invalid input NAND size (should be dump without ECC))" );

    QFile output( args.at( 2 ) );
    if( !output.open( QIODevice::WriteOnly ))
        Fail( "Failed to open output NAND!" );

    qDebug() << "Copying NAND...";
    QByteArray part = input.read(0x800);
    QByteArray spare( 0x40, 0xff );
    while ( !part.isEmpty() ) {
        if ( output.write(part) != 0x800 ) {
            Fail( "Failed to write to output" );
        }
        if ( output.write(spare) != 0x40 ) {
            Fail( "Failed to write to output" );
        }
        part = input.read(0x800);
    }
    input.close();
    output.close();

    if( !nand.SetPath( args.at( 2 ) ) || !nand.InitNand() )
        Fail( "Error setting path to nand object" );

    if( args.size() < 3 )
    {
        args << "-all" << "-v" << "-v";
    }

    root = NULL;

    qDebug() << "Fixing ecc...";
    FixEcc();

    SetUpTree();
    qDebug() << "Fixing hmac...";
    FixHmac();

    return 0;
}

