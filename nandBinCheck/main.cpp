#include "../WiiUQt/includes.h"
#include "../WiiUQt/nandbin.h"
#include "../WiiUQt/tools.h"

//yippie for global variables
QStringList args;
NandBin nand;
QTreeWidgetItem *root;
QList<quint16> fats;
quint32 verbose = 0;
bool tryToKeepGoing = false;
bool color = true;

#ifdef Q_WS_WIN
#include <windows.h>
#define C_STICKY 31
#define C_CAP    192

int origColor;
HANDLE hConsole;

int GetColor()
{
    WORD wColor = 0;

    HANDLE hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    //We use csbi for the wAttributes word.
    if( GetConsoleScreenBufferInfo( hStdOut, &csbi ) )
    {
        wColor = csbi.wAttributes;
    }
    return wColor;
}

#else
#define LEN_STR_PAIR(s) sizeof (s) - 1, s
enum indicator_no
{
    C_LEFT, C_RIGHT, C_END, C_RESET, C_NORM, C_FILE, C_DIR, C_LINK,
    C_FIFO, C_SOCK,
    C_BLK, C_CHR, C_MISSING, C_ORPHAN, C_EXEC, C_DOOR, C_SETUID, C_SETGID,
    C_STICKY, C_OTHER_WRITABLE, C_STICKY_OTHER_WRITABLE, C_CAP, C_MULTIHARDLINK,
    C_CLR_TO_EOL
};

struct bin_str
{
    size_t len;			/* Number of bytes */
    const char *string;		/* Pointer to the same */
};

static struct bin_str color_indicator[] =
{
    { LEN_STR_PAIR ("\033[") },		/* lc: Left of color sequence */
    { LEN_STR_PAIR ("m") },			/* rc: Right of color sequence */
    { 0, NULL },					/* ec: End color (replaces lc+no+rc) */
    { LEN_STR_PAIR ("0") },			/* rs: Reset to ordinary colors */
    { 0, NULL },					/* no: Normal */
    { 0, NULL },					/* fi: File: default */
    { LEN_STR_PAIR ("01;34") },		/* di: Directory: bright blue */
    { LEN_STR_PAIR ("01;36") },		/* ln: Symlink: bright cyan */
    { LEN_STR_PAIR ("33") },		/* pi: Pipe: yellow/brown */
    { LEN_STR_PAIR ("01;35") },		/* so: Socket: bright magenta */
    { LEN_STR_PAIR ("01;33") },		/* bd: Block device: bright yellow */
    { LEN_STR_PAIR ("01;33") },		/* cd: Char device: bright yellow */
    { 0, NULL },					/* mi: Missing file: undefined */
    { 0, NULL },					/* or: Orphaned symlink: undefined */
    { LEN_STR_PAIR ("01;32") },		/* ex: Executable: bright green */
    { LEN_STR_PAIR ("01;35") },		/* do: Door: bright magenta */
    { LEN_STR_PAIR ("37;41") },		/* su: setuid: white on red */
    { LEN_STR_PAIR ("30;43") },		/* sg: setgid: black on yellow */
    { LEN_STR_PAIR ("37;44") },		/* st: sticky: black on blue */
    { LEN_STR_PAIR ("34;42") },		/* ow: other-writable: blue on green */
    { LEN_STR_PAIR ("30;42") },		/* tw: ow w/ sticky: black on green */
    { LEN_STR_PAIR ("30;41") },		/* ca: black on red */
    { 0, NULL },					/* mh: disabled by default */
    { LEN_STR_PAIR ("\033[K") },	/* cl: clear to end of line */
};

static void put_indicator( const struct bin_str *ind )
{
    fwrite( ind->string, ind->len, 1, stdout );
}
#endif
void PrintColoredString( const char *msg, int highlite )
{
    if( !color )
    {
        printf( "%s\n", msg );
    }
    else
    {
        QString str( msg );
        QStringList list = str.split( "\n", QString::SkipEmptyParts );
        foreach( const QString &s, list )
        {
            QString m = s;
            QString m2 = s.trimmed();
            m.resize( m.indexOf( m2 ) );
            printf( "%s", m.toLatin1().data() );				//print all leading whitespace
#ifdef Q_WS_WIN
            SetConsoleTextAttribute( hConsole, highlite );
#else
            put_indicator( &color_indicator[ C_LEFT ] );
            put_indicator( &color_indicator[ highlite ] );		//change color
            put_indicator( &color_indicator[ C_RIGHT ] );
#endif
            printf( "%s", m2.toLatin1().data() );				//print text
#ifdef Q_WS_WIN
            SetConsoleTextAttribute( hConsole, origColor );
#else
            put_indicator( &color_indicator[ C_LEFT ] );
            put_indicator( &color_indicator[ C_NORM ] );		//reset color
            put_indicator( &color_indicator[ C_RIGHT ] );
#endif
            printf( "\n" );
        }
    }
    fflush( stdout );
}

//redirect text output.  by default, qDebug() goes to stderr
void DebugHandler( QtMsgType type, const char *msg )
{
    switch( type )
    {
    case QtDebugMsg:
        printf( "%s\n", msg );
        break;
    case QtWarningMsg:
        PrintColoredString( msg, C_STICKY );
        break;
    case QtCriticalMsg:
        PrintColoredString( msg, C_CAP );
        break;
    case QtFatalMsg:
        fprintf( stderr, "Fatal Error: %s\n", msg );
        abort();
        break;
    }
}

void Usage()
{
    qWarning() << "usage:" << QCoreApplication::arguments().at( 0 ) << "nand.bin" << "<other options>";
    qDebug() << "\nif no <other options> are given, it will default to \"-all -v -v\"";
    qDebug() << "\nOther options:";
    qDebug() << "   -boot           verify boot1";
    qDebug() << "";
    qDebug() << "   -clInfo         shows free, used, and lost ( marked used, but dont belong to any file ) clusters";
    qDebug() << "";
    qDebug() << "   -spare          calculate & compare ecc for all pages in the nand";
    qDebug() << "                   calculate & compare hmac signatures for all files and superblocks";
    qDebug() << "";
    qDebug() << "   -all            does all of the above";
    qDebug() << "";
    qDebug() << "   -v              increase verbosity ( can be used more than once )";
    qDebug() << "";
    qDebug() << "   -continue       try to keep going as fas as possible on errors that should be fatal";
    qDebug() << "";
    qDebug() << "   -nocolor        don\'t use terminal color trickery";
    qDebug() << "";
    qDebug() << "   -about          info about this program";
    exit( 1 );
}

void About()
{
    qCritical()   << "   (c) giantpune 2010, 2011";
    qCritical()   << "   http://code.google.com/p/wiiqt/";
    qCritical()   << "   built:" << __DATE__ << __TIME__;
    qWarning()    << "This software is licensed under GPLv2.  It comes with no guarentee that it will work,";
    qWarning()    << "or that it will work well.";
    qDebug()      << "";
    qDebug()      << "This program is designed to gather information about a nand dump for a Nintendo Wii";
    exit( 1 );
}

void Fail( const QString& str )
{
    qCritical() << str;
    if( !tryToKeepGoing )
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

void CheckBoot1()
{
    if ( nand.CheckBoot1() )
        qDebug() << "Boot1 OK!";
    else
        qCritical() << "Boot1 check failed!";
    
}

void CheckLostClusters()
{
    QList<quint16> u = nand.GetFatsForEntry( 0 );//all clusters actually used for a file
    if( verbose )
        qDebug() << "total used clusters" << hex << u.size() << "of 0x8000";
    quint16 lost = 0;
    QList<quint16> ffs;
    QList<quint16> frs;
    fats = nand.GetFats();
    for( quint16 i = 0; i < 0x8000; i++ )
    {
        if( u.contains( fats.at( i ) ) )//this cluster is really used
            continue;
        switch( fats.at( i ) )
        {
        case 0xFFFB:
        case 0xFFFC:
        case 0xFFFD:
            break;
        case 0xFFFE:
            frs << i;
            break;
        case 0xFFFF:
            ffs << i;
            break;
        default:
            lost++;
            //qDebug() << hex << i << fats.at( i );
            break;
        }
    }
    qDebug() << "found" << lost << "lost clusters\nUNK ( 0xffff )" << hex << ffs.size() << ffs <<
            "\nfree           " << frs.size();
}

void CheckEcc()
{
    QList< quint32 > bad;
    QList< quint16 > clusters;
    fats = nand.GetFats();
    quint32 checked = 0;
    quint16 badClustersNotSpecial = 0;
    for( quint16 i = 0; i < 0x8000; i++ )
    {
        if( fats.at( i ) == 0xfffd || fats.at( i ) == 0xfffe )
            continue;

        for( quint8 j = 0; j < 8; j++, checked += 8 )
        {
            quint32 page = ( i * 8 ) + j;
            if( !nand.CheckEcc( page ) )
            {
                bad << page;
                if( !clusters.contains( i ) )
                    clusters << i;
            }
        }
    }
    QList< quint16 > blocks;
    QList< quint16 > clustersCpy = clusters;

    while( clustersCpy.size() )
    {
        quint16 p = clustersCpy.takeFirst();
        if( fats.at( p ) < 0xfff0 )
            badClustersNotSpecial++;

        quint16 block = p/8;
        if( !blocks.contains( block ) )
            blocks << block;
    }

    qDebug() << bad.size() << "out of" << checked << "pages had incorrect ecc.\nthey were spread through"
            << clusters.size() << "clusters in" << blocks.size() << "blocks:\n" << blocks;
    qDebug() << badClustersNotSpecial << "of those clusters are non-special (they belong to the fs)";

}

void SetUpTree()
{
    if( root )
        return;
    QTreeWidgetItem *r = nand.GetTree();
    if( r->childCount() != 1 || r->child( 0 )->text( 0 ) != "/" )
    {
        tryToKeepGoing = false;
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

int RecurseCheckHmac( QTreeWidgetItem *dir )
{
    int ret = 0;
    quint16 cnt = dir->childCount();
    for( quint16 i = 0; i < cnt; i++ )
    {
        QTreeWidgetItem *item = dir->child( i );
        if( item->text( 7 ).startsWith( "02" ) )
            ret += RecurseCheckHmac( item );
        else
        {
            bool ok = false;
            quint16 entry = item->text( 1 ).toInt( &ok );
            if( !ok )
                continue;

            if( !nand.CheckHmacData( entry ) )
            {
                qCritical() << "bad HMAC for" << PathFromItem( item );
                ret++;
            }
        }
    }
    return ret;
}

void CheckHmac()
{
    quint16 total = RecurseCountFiles( root );
    qDebug() << "verifying hmac for" << total << "files";
    quint16 bad = RecurseCheckHmac( root );
    qDebug() << bad << "files had bad HMAC data";
    qDebug() << "checking HMAC for superclusters...";
    QList<quint16> sclBad;
    for( quint16 i = nand.GetFirstSuperblockCluster(); i < 0x8000; i += 0x10 )
    {
        if( !nand.CheckHmacMeta( i ) )
            sclBad << i;
    }
    qDebug() << sclBad.size() << "superClusters had bad HMAC data";
    if( sclBad.size() )
        qCritical() << sclBad;
}

int main( int argc, char *argv[] )
{
    QCoreApplication a( argc, argv );
#ifdef Q_WS_WIN
    origColor = GetColor();
    hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
#endif
// qInstallMessageHandler( DebugHandler );

    args = QCoreApplication::arguments();
    if( args.contains( "-nocolor", Qt::CaseInsensitive ) )
        color = false;

    qCritical() << "** nandBinCheck : Wii nand info tool **";
    qCritical() << "   from giantpune";
    qCritical() << "   built:" << __DATE__ << __TIME__;

    if( args.contains( "-about", Qt::CaseInsensitive ) )
        About();

    if( args.size() < 2 )
        Usage();

    if( !QFile( args.at( 1 ) ).exists() )
        Usage();

    if( !nand.SetPath( args.at( 1 ) ) || !nand.InitNand() )
        Fail( "Error setting path to nand object" );

    if( args.size() < 3 )
    {
        args << "-all" << "-v" << "-v";
    }

    root = NULL;

    verbose = args.count( "-v" );

    if ( nand.NandType() == NAND_WIIU )
        qDebug() << "NAND Type: SLC (WiiU)";
    else
        qDebug() << "NAND Type: SLCCMPT (vWii)";

    if( args.contains( "-continue", Qt::CaseInsensitive ) )
        tryToKeepGoing = true;

    if( args.contains( "-boot", Qt::CaseInsensitive ) || args.contains( "-all", Qt::CaseInsensitive ) )
    {
        if ( nand.NandType() == NAND_WIIU ) {
            qDebug() << "checking boot1...";
            CheckBoot1();
        } else {
            qDebug() << "vWii - not checking boot";
        }
    }

    if( args.contains( "-clInfo", Qt::CaseInsensitive ) || args.contains( "-all", Qt::CaseInsensitive ) )
    {
        qDebug() << "checking for lost clusters...";
        CheckLostClusters();
    }

    if( args.contains( "-spare", Qt::CaseInsensitive ) || args.contains( "-all", Qt::CaseInsensitive ) )
    {
        qDebug() << "verifying ecc...";
        CheckEcc();

        SetUpTree();
        qDebug() << "verifying hmac...";
        CheckHmac();
    }
    return 0;
}

