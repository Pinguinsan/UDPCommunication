#include <iostream>
#include <string>
#include <memory>
#include <udpduplex.h>
#include <mathutilities.h>
#include <generalutilities.h>
#include <prettyprinter.h>

static std::unique_ptr<PrettyPrinter> prettyPrinter{std::make_unique<PrettyPrinter>()};

int main()
{
    using namespace GeneralUtilities;
    //std::shared_ptr<UDPDuplex> udpDuplex{UDPDuplex::doUserSelectUDPDuplex()};
    std::shared_ptr<UDPDuplex> udpDuplex{std::make_shared<UDPDuplex>("127.0.0.1", 8888, 8888)};
    udpDuplex->openPort();
    do {
        if (udpDuplex->available()) {
            UDPDatagram datagram{udpDuplex->readDatagram()};
            udpDuplex->setClientHostName(datagram.hostName());
            udpDuplex->setClientPortNumber(datagram.portNumber());
            prettyPrinter->setForegroundColor(TStream::RX_COLOR);
            prettyPrinter->println("  Rx << " + datagram.message());
            prettyPrinter->println("    From:");
            prettyPrinter->println("      IP Address: " + datagram.hostName());  
            prettyPrinter->println("      Port Number: " + toString(datagram.portNumber()));
            
            std::string copyString{datagram.message()};
            std::reverse(copyString.begin(), copyString.end());
            udpDuplex->writeString(copyString);
            prettyPrinter->println("  Tx >> " + copyString);
            prettyPrinter->println("    To:");
            prettyPrinter->println("      IP Address: " + udpDuplex->clientHostName());
            prettyPrinter->println("      Port Number: " + toString(udpDuplex->clientPortNumber()));
            prettyPrinter->println();
        }    
    } while (true);
    return 0;

}
