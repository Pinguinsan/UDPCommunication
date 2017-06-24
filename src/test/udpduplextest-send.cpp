#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <mutex>
#include <mathutilities.h>
#include <udpduplex.h>

std::string readFromUdp(UDPDuplex *udpDuplex);
std::string asyncGetline();
std::mutex ioMutex;

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    std::unique_ptr<UDPDuplex> udpDuplex{std::make_unique<UDPDuplex>("127.0.0.1", 8888, 8887)};
    std::string readString{""};
    std::string sendMessage{""};
    udpDuplex->startListening();
    auto readFuture = std::async(std::launch::async, readFromUdp, udpDuplex.get());
    auto stdinFuture = std::async(std::launch::async, asyncGetline);
    do {
        if (readFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            std::string str{readFuture.get()};
            std::string stringToSend{str + std::to_string(MathUtilities::randomBetween(0, 65535))};
            std::cout << "    Rx << " << str << std::endl;
            readFuture = std::async(std::launch::async, readFromUdp, udpDuplex.get());
        }
        if (stdinFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            std::string str{stdinFuture.get()};
            std::cout << "    Tx >> " << str << std::endl;
            udpDuplex->writeString(str);
            stdinFuture = std::async(std::launch::async, asyncGetline);
        }
    } while (true);
            
    return 0;
}


std::string readFromUdp(UDPDuplex *udpDuplex)
{
    do {
        if (udpDuplex->available()) {
            std::string str{udpDuplex->readString()};
            if (str.length() != 0) {
                return str;
            }
        }
    } while (true);
}

std::string asyncGetline()
{
    std::string returnString{""};
    std::getline(std::cin, returnString);
    return returnString;
}
