/***********************************************************************
*    udpduplex.cpp:                                                    *
*    UDPDuplex for sending/receiving datagrams over a UDP port number  *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.serial/tlewiscpp/tjlutils                         *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the implementation of a UDPDuplex class           *
*    It is used to send and receive datagrams over a specified UDP     *
*    port number, to a specified IP address, with DNS lookup supported *
*    This class also implements the TStream interface                  *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#include "udpduplex.h"


//Loopback
const char *UDPDuplex::DEFAULT_CLIENT_HOST_NAME{UDPClient::DEFAULT_HOST_NAME};

UDPDuplex::UDPDuplex(UDPObjectType udpObjectType) :
    UDPDuplex(UDPDuplex::DEFAULT_CLIENT_HOST_NAME, 
              UDPDuplex::DEFAULT_CLIENT_PORT_NUMBER, 
              UDPDuplex::DEFAULT_SERVER_PORT_NUMBER,
              UDPDuplex::DEFAULT_CLIENT_RETURN_ADDRESS_PORT_NUMBER,
              udpObjectType)
{

}

UDPDuplex::UDPDuplex(uint16_t serverPortNumber, UDPObjectType udpObjectType) :
    UDPDuplex(UDPDuplex::DEFAULT_CLIENT_HOST_NAME, 
              UDPDuplex::DEFAULT_CLIENT_PORT_NUMBER, 
              serverPortNumber,
              UDPDuplex::DEFAULT_CLIENT_RETURN_ADDRESS_PORT_NUMBER,
              udpObjectType)
{

}

UDPDuplex::UDPDuplex(const std::string &clientHostName, UDPObjectType udpObjectType) :
    UDPDuplex(clientHostName, 
              UDPDuplex::DEFAULT_CLIENT_PORT_NUMBER, 
              UDPDuplex::DEFAULT_SERVER_PORT_NUMBER,
              UDPDuplex::DEFAULT_CLIENT_RETURN_ADDRESS_PORT_NUMBER,
              udpObjectType)
{

}

UDPDuplex::UDPDuplex(const std::string &clientHostName, uint16_t clientPortNumber, UDPObjectType udpObjectType) :
    UDPDuplex(clientHostName, 
              clientPortNumber, 
              UDPDuplex::DEFAULT_SERVER_PORT_NUMBER,
              UDPDuplex::DEFAULT_CLIENT_RETURN_ADDRESS_PORT_NUMBER,
              udpObjectType)
{

}

UDPDuplex::UDPDuplex(const std::string &clientHostName, uint16_t clientPortNumber, uint16_t serverPortNumber, UDPObjectType udpObjectType) :
    UDPDuplex(clientHostName, 
              clientPortNumber, 
              serverPortNumber,
              UDPDuplex::DEFAULT_CLIENT_RETURN_ADDRESS_PORT_NUMBER,
              udpObjectType)

{

}


UDPDuplex::UDPDuplex(const std::string &clientHostName, uint16_t clientPortNumber, uint16_t serverPortNumber, uint16_t clientReturnAddressPortNumber, UDPObjectType udpObjectType) :
    m_udpClient{nullptr},
    m_udpServer{nullptr},
    m_udpObjectType{udpObjectType}
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        this->m_udpClient = std::unique_ptr<UDPClient>{new UDPClient{clientHostName, 
                                                                     clientPortNumber,
                                                                     clientReturnAddressPortNumber}};
    }
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        if (this->m_udpClient != nullptr) {
            this->m_udpServer = std::unique_ptr<UDPServer>{new UDPServer{}};
        } else {
            this->m_udpServer = std::unique_ptr<UDPServer>{new UDPServer{serverPortNumber}};
        }
    }
    this->setServerTimeout(UDPDuplex::DEFAULT_SERVER_TIMEOUT);
}

std::string UDPDuplex::lineEnding() const
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->lineEnding();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_CLIENT) {
        return this->m_udpClient->lineEnding();
    } else {
        return this->m_udpServer->lineEnding() + " " + this->m_udpClient->lineEnding();
    }
}

void UDPDuplex::setLineEnding(const std::string &lineEnding)
{
    if (this->m_udpObjectType == UDPObjectType::UDP_CLIENT) {
        this->m_udpClient->setLineEnding(lineEnding);
    } else if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        this->m_udpServer->setLineEnding(lineEnding);
    } else {
        this->m_udpClient->setLineEnding(lineEnding);
        this->m_udpServer->setLineEnding(lineEnding);
    }
}

void UDPDuplex::openPort()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        this->m_udpServer->openPort();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_CLIENT) {
        this->m_udpClient->openPort();
    } else {
        this->m_udpServer->openPort();
        this->m_udpClient->openPort();
    }
}

void UDPDuplex::closePort()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        this->m_udpServer->closePort();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_CLIENT) {
        this->m_udpClient->closePort();
    } else {
        this->m_udpServer->closePort();
        this->m_udpClient->closePort();
    }
}

bool UDPDuplex::isOpen() const
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->isOpen();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_CLIENT) {
        return this->m_udpClient->isOpen();
    } else {
        return ((this->m_udpClient->isOpen()) && (this->m_udpServer->isOpen()));
    }
}

void UDPDuplex::setClientPortNumber(uint16_t portNumber)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
       this->m_udpClient->setPortNumber(portNumber);
    }
}

void UDPDuplex::setClientHostName(const std::string &hostName)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        this->m_udpClient->setHostName(hostName);
    }
}

void UDPDuplex::setClientReturnAddressPortNumber(uint16_t portNumber)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
       this->m_udpClient->setReturnAddressPortNumber(portNumber);
    }
}

void UDPDuplex::setClientTimeout(long timeout)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        this->m_udpClient->setTimeout(timeout);   
    }
}

void UDPDuplex::setTimeout(long timeout)
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        this->m_udpServer->setTimeout(timeout);
    } else if (this->m_udpObjectType == UDPObjectType::UDP_CLIENT) {
        this->m_udpClient->setTimeout(timeout);
    } else {
        this->m_udpClient->setTimeout(timeout);
        this->m_udpServer->setTimeout(timeout);
    }
}

uint16_t UDPDuplex::clientPortNumber() const
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpClient->portNumber();
    } else {
        return 0;
    }
}

std::string UDPDuplex::clientHostName() const
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpClient->hostName();
    } else {
        return "";
    }
}
    
uint16_t UDPDuplex::clientReturnAddressPortNumber() const
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpClient->m_udpSocketIndex;
    } else {
        return 0;
    }
}

long UDPDuplex::clientTimeout() const
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpClient->timeout();
    } else {
        return 0;
    }
}

uint16_t UDPDuplex::serverPortNumber() const
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpServer->portNumber();   
    } else {
        return 0;
    }
}

long UDPDuplex::serverTimeout() const
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpServer->timeout();
    } else {
        return 0;
    }
}

void UDPDuplex::setServerPortNumber(uint16_t portNumber)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        this->m_udpServer->setPortNumber(portNumber);
    }
}

void UDPDuplex::setServerTimeout(long timeout)
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        this->m_udpServer->setTimeout(timeout); 
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        this->m_udpServer->setTimeout(this->m_udpClient->m_udpSocketIndex, timeout);
    }
}

long UDPDuplex::timeout() const
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->timeout();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_CLIENT) {
        return this->m_udpClient->timeout();
    } else {
        return (this->m_udpClient->timeout() > this->m_udpServer->timeout() 
                ? this->m_udpClient->timeout() : this->m_udpServer->timeout());
    }
    
}

std::string UDPDuplex::portName() const
{   
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpClient->hostName();
    } else {
        return "server - no port name";
    }
}

void UDPDuplex::flushRXTX()
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        this->m_udpServer->flushRXTX();
    }
} 

void UDPDuplex::flushRX()
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        this->m_udpServer->flushRX();
    }
}

void UDPDuplex::flushTX()
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        this->m_udpServer->flushTX();
    }
}  

ssize_t UDPDuplex::writeLine(const char *str)
{ 
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpClient->writeLine(str); 
    } else {
        return 0;
    }
}

ssize_t UDPDuplex::writeLine(const std::string &str)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_CLIENT) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpClient->writeLine(str);
    } else {
        return 0;
    }
}

UDPDatagram UDPDuplex::readDatagram()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->readDatagram();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->readDatagram(this->m_udpClient->m_udpSocketIndex);
    } else {
        return UDPDatagram{};
    }    
}

char UDPDuplex::readByte()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->readByte();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->readByte(this->m_udpClient->m_udpSocketIndex);
    } else {
        return 0;
    }
}

std::string UDPDuplex::readLine()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->readLine();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->readLine(this->m_udpClient->m_udpSocketIndex);
    } else {
        return "";
    }
}

std::string UDPDuplex::readUntil(const std::string &str)
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->readUntil(str);
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->readUntil(this->m_udpClient->m_udpSocketIndex, str);
    } else {
        return "";
    }
}

std::string UDPDuplex::readUntil(const char *str)
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->readUntil(str);
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->readUntil(this->m_udpClient->m_udpSocketIndex, str);
    }  else {
        return "";
    }
}

std::string UDPDuplex::readUntil(char until)
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->readUntil(until);
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->readUntil(this->m_udpClient->m_udpSocketIndex, until);
    } else {
        return "";
    }
}

void UDPDuplex::putBack(const UDPDatagram &datagram)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpServer->putBack(datagram);
    }
}

void UDPDuplex::putBack(const std::string &str)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpServer->putBack(str);
    }
}

void UDPDuplex::putBack(const char *str)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpServer->putBack(str);
    }
}

void UDPDuplex::putBack(char back)
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpServer->putBack(back);
    }
}

std::string UDPDuplex::peek()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->peek();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->peek(this->m_udpClient->m_udpSocketIndex);
    } else {
        return "";
    }
}

UDPDatagram UDPDuplex::peekDatagram()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->peekDatagram();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->peekDatagram(this->m_udpClient->m_udpSocketIndex);
    } else {
        return UDPDatagram{};
    }
}

char UDPDuplex::peekByte()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->peekByte();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->peekByte(this->m_udpClient->m_udpSocketIndex);
    } else {
        return 0;
    }
}
    
ssize_t UDPDuplex::available()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        return this->m_udpServer->available();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return this->m_udpServer->available(this->m_udpClient->m_udpSocketIndex);
    } else {
        return 0;
    }
}

void UDPDuplex::startListening()
{
    if (this->m_udpObjectType == UDPObjectType::UDP_SERVER) {
        this->m_udpServer->startListening();
    } else if (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX) {
        this->m_udpServer->startListening(this->m_udpClient->m_udpSocketIndex);
    }
}

void UDPDuplex::stopListening()
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpServer->stopListening();
    }
}

bool UDPDuplex::isListening() const
{
    if ((this->m_udpObjectType == UDPObjectType::UDP_SERVER) || (this->m_udpObjectType == UDPObjectType::UDP_DUPLEX)) {
        return this->m_udpServer->isListening();    
    } else {
        return 0;
    }
}

UDPObjectType UDPDuplex::udpObjectType() const
{
    return this->m_udpObjectType;
}

bool constexpr UDPDuplex::isValidPortNumber(int portNumber)
{
    return ((portNumber > 0) && (portNumber < std::numeric_limits<uint16_t>::max()));
}

uint16_t UDPDuplex::doUserSelectClientPortNumber()
{
    return UDPClient::doUserSelectPortNumber();
}

uint16_t UDPDuplex::doUserSelectClientReturnAddressPortNumber()
{
    return UDPClient::doUserSelectReturnAddressPortNumber();
}

std::string UDPDuplex::doUserSelectClientHostName()
{
    return UDPClient::doUserSelectHostName();
}

uint16_t UDPDuplex::doUserSelectServerPortNumber()
{
    return UDPServer::doUserSelectPortNumber();
}

UDPObjectType UDPDuplex::doUserSelectUDPObjectType()
{
    std::vector<std::string> udpObjectTypeChoices{UDPDuplex::udpObjectTypeToString(UDPObjectType::UDP_DUPLEX),
                                                  UDPDuplex::udpObjectTypeToString(UDPObjectType::UDP_SERVER),
                                                  UDPDuplex::udpObjectTypeToString(UDPObjectType::UDP_CLIENT)};
    return GeneralUtilities::doUserSelectParameter("UDP Object Type",
                                                    static_cast< std::function<UDPObjectType(const std::string &)> >(UDPDuplex::parseUDPObjectTypeFromRaw),
                                                    udpObjectTypeChoices,
                                                    UDPDuplex::udpObjectTypeToString(UDPObjectType::UDP_DUPLEX).c_str());

}

std::shared_ptr<UDPDuplex> UDPDuplex::doUserSelectUDPDuplex()
{
    std::shared_ptr<UDPDuplex> udpDuplex{nullptr};
    UDPObjectType udpObjectType{UDPDuplex::doUserSelectUDPObjectType()};
    if (udpObjectType == UDPObjectType::UDP_DUPLEX) {
        std::string clientHostName{UDPDuplex::doUserSelectClientHostName()};
        uint16_t clientPortNumber{UDPDuplex::doUserSelectClientPortNumber()};
        uint16_t clientReturnAddressPortNumber{UDPDuplex::doUserSelectClientReturnAddressPortNumber()};
        uint16_t serverPortNumber{UDPDuplex::doUserSelectServerPortNumber()};
        udpDuplex = std::make_shared<UDPDuplex>(clientHostName, 
                                                clientPortNumber,
                                                serverPortNumber,
                                                clientReturnAddressPortNumber, 
                                                udpObjectType);

    } else if (udpObjectType == UDPObjectType::UDP_SERVER) {
        uint16_t serverPortNumber{UDPDuplex::doUserSelectServerPortNumber()};
        udpDuplex = std::make_shared<UDPDuplex>(serverPortNumber, udpObjectType);
    } else if (udpObjectType == UDPObjectType::UDP_CLIENT) {
        std::string clientHostName{UDPDuplex::doUserSelectClientHostName()};
        uint16_t clientPortNumber{UDPDuplex::doUserSelectClientPortNumber()};
        uint16_t clientReturnAddressPortNumber{UDPDuplex::doUserSelectClientReturnAddressPortNumber()};
        udpDuplex = std::make_shared<UDPDuplex>(clientHostName,
                                                clientPortNumber,
                                                clientReturnAddressPortNumber,
                                                udpObjectType);
    } else {
        throw std::runtime_error("Unknown UDPObjectType passed to UDPDuplex::doUserSelectUDPDuplex()");
    }
    return udpDuplex;
    
}

UDPObjectType UDPDuplex::parseUDPObjectTypeFromRaw(const std::string &udpObjectType)
{
    using namespace GeneralUtilities;
    std::string copyString{udpObjectType};
    std::transform(copyString.begin(), copyString.end(), copyString.begin(), ::tolower);
    std::string secondCopy{""};
    for (auto &it : copyString) {
        if (it != ' ') {
            secondCopy += it;
        }
    }
    if (secondCopy == "udpduplex") {
        return UDPObjectType::UDP_DUPLEX;
    } else if (secondCopy == "udpserver") {
        return UDPObjectType::UDP_SERVER;
    } else if (secondCopy == "udpclient") {
        return UDPObjectType::UDP_CLIENT;
    } else {
        throw std::runtime_error("Could not parse UDPObjectType in UDPDuplex::parseUDPObjectTypeFromRaw(const std::string &): unknown identifier " + tQuoted(udpObjectType));
    }
}

std::string UDPDuplex::udpObjectTypeToString(UDPObjectType udpObjectType)
{
    if (udpObjectType == UDPObjectType::UDP_DUPLEX) {
        return "UDP Duplex";
    } else if (udpObjectType == UDPObjectType::UDP_SERVER) {
        return "UDP Server";
    } else if (udpObjectType == UDPObjectType::UDP_CLIENT) {
        return "UDP Client";
    } else {
        throw std::runtime_error("Unknown UDPObjectType passed to UDPDuplex::udpObjectTypeToString(UDPObjectType)");
    }
}
