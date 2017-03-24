#include "../WiiUQt/includes.h"
#include "../WiiUQt/nandbin.h"
#include "../WiiUQt/tools.h"

//yippie for global variables
QStringList args;
NandBin nand;

void Usage()
{
    qWarning() << "usage:" << QCoreApplication::arguments().at( 0 ) << "<input nand> <region(JPN/USA/EUR)>";
    exit( 1 );
}

void Fail( const QString& str )
{
    qCritical() << str;
    exit( 1 );
}

int main( int argc, char *argv[] )
{
    QCoreApplication a( argc, argv );
// qInstallMessageHandler( DebugHandler );

    args = QCoreApplication::arguments();

    qCritical() << "** nandFixer : Fix incomplete dump, add ECCs and HMACs **";
    qCritical() << "   built:" << __DATE__ << __TIME__;


    if( args.size() < 2 )
        Usage();

    if( !QFile( args.at( 1 ) ).exists() )
        Usage();

    if( !nand.SetPath( args.at( 1 ) ) || !nand.InitNand() )
        Fail( "Error setting path to nand object" );
    
    if (nand.DumpType() != NAND_DUMP_ECC) {
        Fail("ERROR: Must be full NAND dump! (with ecc and spare data)");
    }
    if (nand.NandType() != NAND_WIIU) {
        Fail("ERROR: Not WiiU SLC NAND");
    }

    QByteArray syshax_xml = nand.GetData("/sys/config/syshax.xml");
    if (syshax_xml.isEmpty())
        Fail( "Can't find system.xml backup, are you sure CBHC is installed?" );
    qDebug() << "Restoring system.xml backup";
    nand.SetData("/sys/config/system.xml", syshax_xml);
    if (!nand.WriteMetaData())
        Fail ( "Failed to write metadata, dump may be corrupted now");

    return 0;
}

