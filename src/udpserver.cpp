/***********************************************************************
*    udpserver.cpp:                                                    *
*    UDPServer, for receiving datagrams over a UDP port number         *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.serial/tlewiscpp/tjlutils                         *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the implementation of a UDPServer class           *
*    It is used to receive datagrams over a specified UDP port number  *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#include "udpserver.h"

const uint16_t UDPServer::BROADCAST{1};

UDPServer::UDPServer() :
    UDPServer{UDPServer::DEFAULT_PORT_NUMBER}
{
    
}

UDPServer::UDPServer(uint16_t portNumber) :
    m_socketAddress{},
    m_isListening{false},
    m_setSocketResult{0},
    m_timeout{UDPServer::DEFAULT_TIMEOUT},
    m_datagramQueue{},
    m_shutEmDown{false}
{
    this->initialize(portNumber);
}

bool constexpr UDPServer::isValidPortNumber(int portNumber)
{
    return ((portNumber > 0) && (portNumber < std::numeric_limits<uint16_t>::max()));
}

void UDPServer::setPortNumber(uint16_t portNumber)
{
    if (!this->isValidPortNumber(portNumber)) {
        throw std::runtime_error("ERROR: Invalid port set for UDPServer, must be between 1 and " +
                                 std::to_string(std::numeric_limits<uint16_t>::max())
                                 + "("
                                 + std::to_string(portNumber)
                                 + ")");
    } else {
        this->m_socketAddress.sin_port = htons(portNumber);
    }
}

void UDPServer::setTimeout(long timeout)
{
    long tempTimeout{this->m_timeout};
    if (timeout < 0) {
        this->m_timeout = 0;
    } else {
        this->m_timeout = timeout;
    }
    struct timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = this->m_timeout * 3;
    if (setsockopt(this->m_setSocketResult, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        this->m_timeout = tempTimeout;
        throw std::runtime_error("In UDPServer::setTimeout(long): An error occurred while attempting to set the socket timeout");
    }
}


void UDPServer::setTimeout(int socketIndex, long timeout)
{
    long tempTimeout{this->m_timeout};
    if (timeout < 0) {
        this->m_timeout = 0;
    } else {
        this->m_timeout = timeout;
    }
    struct timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = this->m_timeout * 3;
    if (setsockopt(socketIndex, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        this->m_timeout = tempTimeout;
        throw std::runtime_error("In UDPServer::setTimeout(int, long): An error occurred while attempting to set the socket timeout");
    }
}

long UDPServer::timeout() const
{
    return this->m_timeout;
}

void UDPServer::flushRXTX()
{
    std::lock_guard<std::mutex> ioLock{this->m_ioMutex};
    this->m_datagramQueue.clear();
}

uint16_t UDPServer::portNumber() const 
{ 
    return ntohs(this->m_socketAddress.sin_port); 
}

void UDPServer::flushRX()
{
    return this->flushRXTX();
}

void UDPServer::flushTX()
{
    return this->flushRXTX();
}

void UDPServer::initialize(uint16_t portNumber)
{
    using namespace GeneralUtilities;
    if (!this->isValidPortNumber(portNumber)) {
        portNumber = UDPServer::DEFAULT_PORT_NUMBER;
        throw std::runtime_error("ERROR: Invalid port set for UDPServer, must be between 1 and " +
                                 std::to_string(std::numeric_limits<uint16_t>::max())
                                 + "("
                                 + std::to_string(portNumber)
                                 + ")");
    }


    this->m_setSocketResult = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->m_setSocketResult == -1) {
       throw std::runtime_error("ERROR: UDPClient could not set socket " + tQuoted(this->m_setSocketResult) + " (is something else using it?)");
    }

    setsockopt(this->m_setSocketResult, SOL_SOCKET, SO_SNDBUF, &UDPServer::BROADCAST, sizeof(UDPServer::BROADCAST));
    memset(&this->m_socketAddress, 0, sizeof(this->m_socketAddress));
    this->m_socketAddress.sin_family = AF_INET;
    this->m_socketAddress.sin_addr.s_addr = INADDR_ANY;
    this->m_socketAddress.sin_port = htons(portNumber);

    if (bind(this->m_setSocketResult, reinterpret_cast<sockaddr *>(&this->m_socketAddress), sizeof(sockaddr)) == -1) {
       throw std::runtime_error("ERROR: UDPClient could not bind socket " + tQuoted(this->m_setSocketResult) + " (is something else using it?)");
    }
}

void UDPServer::startListening(int socketNumber)
{
    if (!this->m_isListening) {
        this->m_isListening = true;
#if defined(__ANDROID__)
        if (this->m_asyncFuture) {
            this->m_asyncFuture.join();
            delete this->m_asyncFuture;
        }
        this->m_asyncFuture = new std::thread{&static_cast<void (UDPServer::*)(int)>(&UDPServer::asyncDatagramListener),
                                              this,
                                              socketNumber};
#else
    try {
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (this->m_asyncFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready);
        //this->m_asyncFuture.reset();
    } catch (std::exception &e) {
        
    }
    this->m_asyncFuture = std::async(std::launch::async,
                                    static_cast<void (UDPServer::*)(int)>(&UDPServer::asyncDatagramListener),
                                    this,
                                    socketNumber);
#endif
    }
}

void UDPServer::startListening()
{
    if (!this->m_isListening) {
        this->m_isListening = true;
#if defined(__ANDROID__)
        if (this->m_asyncFuture) {
            this->m_asyncFuture.join();
            delete this->m_asyncFuture;
        }
        this->m_asyncFuture = new std::thread{&static_cast<void (UDPServer::*)()>(&UDPServer::asyncDatagramListener),
                                              this};
#else
    try {
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (this->m_asyncFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready);
        //this->m_asyncFuture.reset();
    } catch (std::exception &e) {
        
    }
    this->m_asyncFuture = std::async(std::launch::async,
                                    static_cast<void (UDPServer::*)()>(&UDPServer::asyncDatagramListener),
                                    this);
#endif
    }
}

void UDPServer::stopListening()
{
    this->m_shutEmDown = true;
    this->m_isListening = false;
#if defined(__ANDROID__)
    if (this->m_asyncFuture) {
        this->m_asyncFuture->join();
        delete this->m_asyncFuture;
    }
#else
    try {
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (this->m_asyncFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready);
        //this->m_asyncFuture.reset();
    } catch (std::exception &e) {

    }

#endif
}

bool UDPServer::isListening() const
{
    return this->m_isListening;
}

void UDPServer::asyncDatagramListener()
{
    std::unique_lock<std::mutex> ioMutexLock{this->m_ioMutex, std::defer_lock};
    do {
        char lowLevelReceiveBuffer[UDPServer::RECEIVED_BUFFER_MAX];
        memset(lowLevelReceiveBuffer, 0, UDPServer::RECEIVED_BUFFER_MAX);
        std::string receivedString{""};
        sockaddr_in receivedAddress{};
        platform_socklen_t socketSize{sizeof(sockaddr)};
        ssize_t returnValue {recvfrom(this->m_setSocketResult,
                            lowLevelReceiveBuffer,
                            sizeof(lowLevelReceiveBuffer)-1,
                            0,
                            reinterpret_cast<sockaddr *>(&receivedAddress),
                            &socketSize)};
        if ((returnValue == EAGAIN) || (returnValue == EWOULDBLOCK) || (returnValue == -1)) {
            //No data;
        } else {
            receivedString = std::string{lowLevelReceiveBuffer};
            if (receivedString.length() > 0) {
                ioMutexLock.lock();
                this->m_datagramQueue.emplace_back(receivedAddress, receivedString);
                ioMutexLock.unlock();

            }
        }
    } while (!this->m_shutEmDown);
}


void UDPServer::asyncDatagramListener(int socketNumber)
{
    std::unique_lock<std::mutex> ioMutexLock{this->m_ioMutex, std::defer_lock};
    do {
        char lowLevelReceiveBuffer[UDPServer::RECEIVED_BUFFER_MAX];
        memset(lowLevelReceiveBuffer, 0, UDPServer::RECEIVED_BUFFER_MAX);
        std::string receivedString{""};
        sockaddr_in receivedAddress{};
        platform_socklen_t socketSize{sizeof(sockaddr)};
        ssize_t returnValue {recvfrom(socketNumber,
                            lowLevelReceiveBuffer,
                            sizeof(lowLevelReceiveBuffer)-1,
                            0,
                            reinterpret_cast<sockaddr *>(&receivedAddress),
                            &socketSize)};
        if ((returnValue == EAGAIN) || (returnValue == EWOULDBLOCK) || (returnValue == -1)) {
            //No data;
        } else {
            receivedString = std::string{lowLevelReceiveBuffer};
            if (receivedString.length() > 0) {
                ioMutexLock.lock();
                this->m_datagramQueue.emplace_back(receivedAddress, receivedString);
                ioMutexLock.unlock();

            }
        }
    } while (!this->m_shutEmDown);
}

void UDPServer::syncDatagramListener(int socketNumber)
{
    std::unique_lock<std::mutex> ioMutexLock{this->m_ioMutex, std::defer_lock};
    char lowLevelReceiveBuffer[UDPServer::RECEIVED_BUFFER_MAX];
    memset(lowLevelReceiveBuffer, 0, UDPServer::RECEIVED_BUFFER_MAX);
    std::string receivedString{""};
    sockaddr_in receivedAddress{};
    platform_socklen_t socketSize{sizeof(sockaddr)};
    ssize_t returnValue{recvfrom(socketNumber,
                        lowLevelReceiveBuffer,
                        sizeof(lowLevelReceiveBuffer)-1,
                        0,
                        reinterpret_cast<sockaddr *>(&receivedAddress),
                        &socketSize)};
    if ((returnValue == EAGAIN) || (returnValue == EWOULDBLOCK) || (returnValue == -1)) {
        return;
    }
    receivedString = std::string{lowLevelReceiveBuffer};
    if (receivedString.length() > 0) {
        ioMutexLock.lock();
        this->m_datagramQueue.emplace_back(receivedAddress, receivedString);
        ioMutexLock.unlock();
    }
}

void UDPServer::syncDatagramListener()
{
    std::unique_lock<std::mutex> ioMutexLock{this->m_ioMutex, std::defer_lock};
    char lowLevelReceiveBuffer[UDPServer::RECEIVED_BUFFER_MAX];
    memset(lowLevelReceiveBuffer, 0, UDPServer::RECEIVED_BUFFER_MAX);
    std::string receivedString{""};
    sockaddr_in receivedAddress{};
    platform_socklen_t socketSize{sizeof(sockaddr)};
    ssize_t returnValue{recvfrom(this->m_setSocketResult,
                        lowLevelReceiveBuffer,
                        sizeof(lowLevelReceiveBuffer)-1,
                        0,
                        reinterpret_cast<sockaddr *>(&receivedAddress),
                        &socketSize)};
    if ((returnValue == EAGAIN) || (returnValue == EWOULDBLOCK) || (returnValue == -1)) {
        return;
    }
    receivedString = std::string{lowLevelReceiveBuffer};
    if (receivedString.length() > 0) {
        ioMutexLock.lock();
        this->m_datagramQueue.emplace_back(receivedAddress, receivedString);
        ioMutexLock.unlock();
    }
}

void UDPServer::setLineEnding(const std::string &lineEnding)
{
    this->m_lineEnding = lineEnding;
}

std::string UDPServer::lineEnding() const
{
    return this->m_lineEnding;
}

std::string UDPServer::peek()
{
    this->syncDatagramListener();
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return "";
    } else {
        return this->m_datagramQueue.front().message();
    }
}


UDPDatagram UDPServer::peekDatagram()
{
    this->syncDatagramListener();
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return UDPDatagram{};
    } else {
        return this->m_datagramQueue.front();
    }
}

char UDPServer::peekByte()
{
    this->syncDatagramListener();
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return 0;
    } else {
        if (this->m_datagramQueue.front().message().size() == 0) {
            return 0;
        } else {
            return this->m_datagramQueue.front().message().at(0);
        }
    }
}

char UDPServer::readByte()
{
    this->syncDatagramListener();
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    std::string str{this->m_datagramQueue.front().message()};
    if (str.length() == 0) {
        return 0;
    }
    char charToReturn{str.at(0)};
    std::string newDatagramMessage{str.substr(1)};
    struct sockaddr_in newDatagramAddress;
    newDatagramAddress.sin_family = this->m_datagramQueue.front().socketAddress().sin_family;
    newDatagramAddress.sin_port = this->m_datagramQueue.front().socketAddress().sin_port;
    newDatagramAddress.sin_addr.s_addr = this->m_datagramQueue.front().socketAddress().sin_addr.s_addr;
    this->m_datagramQueue.pop_front();
    this->m_datagramQueue.emplace_front(newDatagramAddress, newDatagramMessage);
    return charToReturn;

}

UDPDatagram UDPServer::readDatagram()
{
    this->syncDatagramListener();
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return UDPDatagram{};
    } else {
        UDPDatagram returnDatagram{this->m_datagramQueue.front()};
        this->m_datagramQueue.pop_front();
        return returnDatagram;
    }
}


std::string UDPServer::readLine()
{
    this->syncDatagramListener();
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return "";
    }
    std::string stringToReturn{this->m_datagramQueue.front().message()};
    this->m_datagramQueue.pop_front();
    return stringToReturn;
}

void UDPServer::openPort()
{
    //this->startListening();
}

void UDPServer::closePort()
{
    //this->stopListening();
}

bool UDPServer::isOpen() const
{
    return this->m_isListening;
}

std::string UDPServer::readUntil(char until)
{
    return this->readUntil(std::string{1, until});
}

std::string UDPServer::readUntil(const char *until)
{
    return this->readUntil(static_cast<std::string>(until));
}

std::string UDPServer::readUntil(const std::string &until)
{
    std::string tempLineEnding{this->m_lineEnding};
    this->m_lineEnding = until;
    std::string returnString{this->readLine()};
    this->m_lineEnding = tempLineEnding;
    return returnString;
}

void UDPServer::putBack(const UDPDatagram &datagram)
{
    std::lock_guard<std::mutex> ioMutex{this->m_ioMutex};
    this->m_datagramQueue.push_front(datagram);
}

void UDPServer::putBack(char back)
{
    return UDPServer::putBack(std::string{1, back});
}

void UDPServer::putBack(const char *str)
{
    return UDPServer::putBack(static_cast<std::string>(str));
}

void UDPServer::putBack(const std::string &str)
{
    if (str.length() == 0) {
        return;
    }
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    std::string newDatagramMessage{str + this->m_datagramQueue.front().message()};
    struct sockaddr_in newDatagramAddress;
    newDatagramAddress.sin_family = this->m_datagramQueue.front().socketAddress().sin_family;
    newDatagramAddress.sin_port = this->m_datagramQueue.front().socketAddress().sin_port;
    newDatagramAddress.sin_addr.s_addr = this->m_datagramQueue.front().socketAddress().sin_addr.s_addr;
    this->m_datagramQueue.pop_front();
    this->m_datagramQueue.emplace_front(newDatagramAddress, newDatagramMessage);
}

uint16_t UDPServer::doUserSelectPortNumber()
{
    return GeneralUtilities::doUserEnterNumericParameter("Server Port Number",
                                                         static_cast<std::function<bool(uint16_t)>>(UDPServer::isValidPortNumber),
                                                         std::numeric_limits<uint16_t>::min()+1,
                                                         std::numeric_limits<uint16_t>::max());
}

std::shared_ptr<UDPServer> UDPServer::doUserSelectUDPServer()
{
    uint16_t portNumber{UDPServer::doUserSelectPortNumber()};
    std::shared_ptr<UDPServer> udpServer{std::make_shared<UDPServer>(portNumber)};
    return udpServer;
}

std::string UDPServer::readUntil(int socketNumber, char until)
{
    return this->readUntil(socketNumber, std::string{1, until});
}

std::string UDPServer::readUntil(int socketNumber, const char *until)
{
    return this->readUntil(socketNumber, static_cast<std::string>(until));
}

std::string UDPServer::readUntil(int socketNumber, const std::string &until)
{
    std::string tempLineEnding{this->m_lineEnding};
    this->m_lineEnding = until;
    std::string returnString{this->readLine(socketNumber)};
    this->m_lineEnding = tempLineEnding;
    return returnString;
}

std::string UDPServer::peek(int socketNumber)
{
    this->syncDatagramListener(socketNumber);
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return "";
    } else {
        return this->m_datagramQueue.front().message();
    }
}

UDPDatagram UDPServer::peekDatagram(int socketNumber)
{
    this->syncDatagramListener(socketNumber);
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return UDPDatagram{};
    } else {
        return this->m_datagramQueue.front();
    }
}

char UDPServer::peekByte(int socketNumber)
{
    this->syncDatagramListener(socketNumber);
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return 0;
    } else {
        if (this->m_datagramQueue.front().message().size() == 0) {
            return 0;
        } else {
            return this->m_datagramQueue.front().message().at(0);
        }
    }
}

ssize_t UDPServer::available() 
{
    this->syncDatagramListener();
    return this->m_datagramQueue.size();
}

ssize_t UDPServer::available(int socketNumber) 
{
    this->syncDatagramListener(socketNumber);
    return this->m_datagramQueue.size();
}

char UDPServer::readByte(int socketNumber)
{
    this->syncDatagramListener(socketNumber);
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    std::string str{this->m_datagramQueue.front().message()};
    if (str.length() == 0) {
        return 0;
    }
    char charToReturn{str.at(0)};
    std::string newDatagramMessage{str.substr(1)};
    struct sockaddr_in newDatagramAddress;
    newDatagramAddress.sin_family = this->m_datagramQueue.front().socketAddress().sin_family;
    newDatagramAddress.sin_port = this->m_datagramQueue.front().socketAddress().sin_port;
    newDatagramAddress.sin_addr.s_addr = this->m_datagramQueue.front().socketAddress().sin_addr.s_addr;
    this->m_datagramQueue.pop_front();
    this->m_datagramQueue.emplace_front(newDatagramAddress, newDatagramMessage);
    return charToReturn;

}

UDPDatagram UDPServer::readDatagram(int socketNumber)
{
    this->syncDatagramListener(socketNumber);
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return UDPDatagram{};
    } else {
        UDPDatagram returnDatagram{this->m_datagramQueue.front()};
        this->m_datagramQueue.pop_front();
        return returnDatagram;
    }
}


std::string UDPServer::readLine(int socketNumber)
{
    this->syncDatagramListener(socketNumber);
    std::lock_guard<std::mutex> ioMutexLock{this->m_ioMutex};
    if (this->m_datagramQueue.size() == 0) {
        return "";
    }
    std::string stringToReturn{this->m_datagramQueue.front().message()};
    this->m_datagramQueue.pop_front();
    return stringToReturn;
}

UDPServer::~UDPServer()
{
    this->stopListening();
    shutdown(this->m_setSocketResult, SHUT_RDWR);
}