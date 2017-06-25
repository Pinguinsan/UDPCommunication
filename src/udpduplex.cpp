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


#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <memory.h>

#include "udpduplex.h"

inline bool endsWith(const std::string &stringToCheck, const std::string &matchString)
{
    return (matchString.size() > stringToCheck.size()) ? false : std::equal(matchString.rbegin(), matchString.rend(), stringToCheck.rbegin());
}

inline bool endsWith(const std::string &stringToCheck, char matchChar)
{
    return endsWith(stringToCheck, std::string(1, matchChar));
}

template <typename T> static inline std::string toStdString(const T &t) { 
    return dynamic_cast<std::stringstream &>(std::stringstream{} << t).str(); 
}
template <typename T> static inline std::string tQuoted(const T &t) {
    return "\"" + toStdString(t) + "\"";
}

template<typename T, typename TLow, typename THigh>
T doUserEnterNumericParameter(const std::string &name,
                                const std::function<bool(T)> &validator,
                                TLow lowLimit,
                                THigh highLimit) {
    std::cout << std::endl;
    std::string userOption{""};
    while (true) {
        userOption = "";
        std::cout << "Enter a number to use for " << tQuoted(name) <<
        " between (inclusive) " << lowLimit << " and " << highLimit <<
        ", or press CTRL+C to quit: ";
        std::getline(std::cin, userOption);
        if (userOption == "") {
            continue;
        }
        int userEntry{0};
        try {
            userEntry = std::stoi(userOption);
            if (userEntry < lowLimit) {
                std::cout << tQuoted(userEntry) << " is less than the minimum value for " <<
                name << " (" << name << " < " << lowLimit << std::endl << std::endl;
                continue;
            } else if (userEntry > highLimit) {
                std::cout << tQuoted(userEntry) <<
                " is greater than the maximum value for " << name << " (" << name <<
                " > " << highLimit << std::endl << std::endl;
                continue;
            } else if (!validator(userEntry)) {
                std::cout << tQuoted(userEntry) << " is an invalid " << name << std::endl <<
                std::endl;
                continue;
            }
            return userEntry;
        } catch (std::exception &e) {
            std::cout << tQuoted(userOption) << " is not a number" << std::endl <<
            std::endl;
        }
    }
}

template<typename T>
std::string doUserEnterStringParameter(const T &name,
                                        const std::function<bool(std::string)> &validator) {
    std::cout << std::endl;
    std::string userOption{""};
    while (true) {
        userOption = "";
        std::cout << "Please enter a string to use for " << tQuoted(name) <<
        ", or press CTRL+C to quit: ";
        std::getline(std::cin, userOption);
        if (userOption == "") {
            continue;
        }
        try {
            if (!validator(userOption)) {
                std::cout << tQuoted(userOption) << " is an invalid " << name <<
                std::endl << std::endl;
                continue;
            }
            return userOption;
        } catch (std::exception &e) {
            std::cout << tQuoted(userOption) << " is an invalid " << name << std::endl <<
            std::endl;
        }
    }
}

template<typename T, typename TOps>
T doUserSelectParameter(const std::string &name,
                        const std::function<T(const std::string &)> &func,
                        const std::vector<TOps> &availableOptions,
                        const char *defaultOption) {
    std::cout << std::endl;
    if (availableOptions.size() == 0) {
        throw std::runtime_error("No " + name + " are available");
    } else if (availableOptions.size() == 1) {
        return func(availableOptions.at(0));
    }
    unsigned int quitOption{0};
    for (unsigned int selectionIndex = 1;
            selectionIndex <= availableOptions.size(); selectionIndex++) {
        std::cout << selectionIndex << ".) " << availableOptions.at(selectionIndex - 1);
        if (static_cast<std::string>(availableOptions.at(selectionIndex - 1)) ==
            static_cast<std::string>(defaultOption)) {
            std::cout << "    <----DEFAULT" << std::endl;
        } else {
            std::cout << std::endl;
        }
        quitOption = selectionIndex + 1;
    }
    std::cout << quitOption << ".) Quit" << std::endl << std::endl;
    std::string userOption{""};
    while (true) {
        userOption = "";
        std::cout << "Select a " << name <<
        " from the above options, or press CTRL+C to quit: ";
        std::getline(std::cin, userOption);
        if (userOption == "") {
            return func(defaultOption);
        }
        unsigned int userOptionIndex{0};
        bool userSelectedQuit{false};
        try {
            userOptionIndex = std::stoi(userOption);
            if (userOptionIndex > availableOptions.size() + 1) {
                std::cout << tQuoted(userOption) <<
                " wasn't one of the selections, please a number between (inclusive) 1 and " <<
                quitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
                continue;
            }
            if (userOptionIndex == quitOption) {
                userSelectedQuit = true;
                throw std::invalid_argument(
                        "In GeneralUtilities::doUserSelectParameter(): User selected quit option");
            }
            return func(availableOptions.at(userOptionIndex - 1));
        } catch (std::exception &e) {
            if (userSelectedQuit) {
                throw e;
            }
            std::cout << tQuoted(userOption) <<
            " wasn't one of the selections, enter a number between (inclusive) 1 and " <<
            quitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
        }
    }
}

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
    return doUserEnterNumericParameter("Server Port Number",
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

//Loopback
const char *UDPClient::DEFAULT_HOST_NAME{"127.0.0.1"};
const std::string UDPClient::DEFAULT_LINE_ENDING{"\r\n"};

UDPClient::UDPClient() :
    UDPClient(static_cast<std::string>(UDPClient::DEFAULT_HOST_NAME),
              UDPClient::DEFAULT_PORT_NUMBER,
              UDPClient::DEFAULT_RETURN_ADDRESS_PORT_NUMBER)
{
    
}

UDPClient::UDPClient(uint16_t portNumber) :
    UDPClient(static_cast<std::string>(UDPClient::DEFAULT_HOST_NAME),
              portNumber,
              UDPClient::DEFAULT_RETURN_ADDRESS_PORT_NUMBER)
{

}

UDPClient::UDPClient(const std::string &hostName) :
    UDPClient(hostName,
              UDPClient::DEFAULT_PORT_NUMBER,
              UDPClient::DEFAULT_RETURN_ADDRESS_PORT_NUMBER)
{

}

UDPClient::UDPClient(const std::string &hostName, uint16_t portNumber) :
    UDPClient(hostName,
              portNumber,
              UDPClient::DEFAULT_RETURN_ADDRESS_PORT_NUMBER)
{

}

UDPClient::UDPClient(const std::string &hostName, uint16_t portNumber, uint16_t returnAddressPortNumber) :
    m_destinationAddress{},
    m_returnAddress{},
    m_udpSocketIndex{0},
    m_timeout{DEFAULT_TIMEOUT},
    m_lineEnding{DEFAULT_LINE_ENDING}
{
    this->initialize(hostName,
                     portNumber,
                     returnAddressPortNumber);

}

uint16_t UDPClient::returnAddressPortNumber() const
{
    return ntohs(this->m_returnAddress.sin_port);
}

void UDPClient::setReturnAddressPortNumber(uint16_t portNumber)
{
    if (!isValidPortNumber(portNumber)) {
        throw std::runtime_error("ERROR: Invalid port set for UDPServer, must be between 1 and " +
                                 std::to_string(std::numeric_limits<uint16_t>::max()) 
                                 + "("
                                 + std::to_string(portNumber) 
                                 + ")");
    }
    this->m_returnAddress.sin_port = htons(portNumber);
}

void UDPClient::setPortNumber(uint16_t portNumber)
{
    if (!isValidPortNumber(portNumber)) {
        throw std::runtime_error("ERROR: Invalid port set for UDPServer, must be between 1 and " +
                                 std::to_string(std::numeric_limits<uint16_t>::max()) 
                                 + "("
                                 + std::to_string(portNumber) 
                                 + ")");
    }
    this->m_destinationAddress.sin_port = htons(portNumber);
}

void UDPClient::setLineEnding(const std::string &lineEnding)
{
    this->m_lineEnding = lineEnding;
}

std::string UDPClient::lineEnding() const
{
    return this->m_lineEnding;
}

void UDPClient::setTimeout(unsigned long int timeout)
{
    this->m_timeout = timeout;
}

void UDPClient::setHostName(const std::string &hostName)
{
    sockaddr_storage temp{};
    if (resolveAddressHelper (hostName, AF_INET, std::to_string(this->portNumber()), &temp) != 0) {
       throw std::runtime_error("ERROR: UDPClient could not resolve adress " + tQuoted(hostName));
    }
    inet_pton(AF_INET, hostName.c_str(), &(this->m_destinationAddress.sin_addr));
}

void UDPClient::openPort()
{
    
}

void UDPClient::closePort()
{

}

bool UDPClient::isOpen() const
{
    return true;
}

std::string UDPClient::hostName() const
{
    char lowLevelTempBuffer[INET_ADDRSTRLEN];
    memset(lowLevelTempBuffer, '\0', INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(this->m_destinationAddress.sin_addr), lowLevelTempBuffer, INET_ADDRSTRLEN);
    return std::string{lowLevelTempBuffer};
}

uint16_t UDPClient::portNumber() const
{
    return ntohs(this->m_destinationAddress.sin_port);
}

unsigned long int UDPClient::timeout() const
{
    return this->m_timeout;
}

void UDPClient::initialize(const std::string &hostName, uint16_t portNumber, uint16_t returnAddressPortNumber)
{
    this->m_udpSocketIndex = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


    if (!this->isValidPortNumber(portNumber)) {
        uint16_t temp{portNumber};
        portNumber = UDPClient::DEFAULT_PORT_NUMBER;
        throw std::runtime_error("ERROR: Invalid port set for UDPServer, must be between 1 and " +
                                 std::to_string(std::numeric_limits<uint16_t>::max()) 
                                 + "("
                                 + std::to_string(temp) 
                                 + ")");
    }
    if (!this->isValidPortNumber(returnAddressPortNumber)) {
        uint16_t temp{returnAddressPortNumber};
        returnAddressPortNumber = UDPClient::DEFAULT_RETURN_ADDRESS_PORT_NUMBER;
        throw std::runtime_error("ERROR: Invalid port set for UDPServer, must be between 1 and " +
                                 std::to_string(std::numeric_limits<uint16_t>::max()) 
                                 + "("
                                 + std::to_string(temp) 
                                 + ")");
    }

    this->m_destinationAddress.sin_family = AF_INET;
    this->m_destinationAddress.sin_port = htons(portNumber);
    
    this->m_returnAddress.sin_family = AF_INET;
    this->m_returnAddress.sin_port = htons(returnAddressPortNumber);
    //This cannot be set with UDP sockets:
    //inet_pton(AF_INET, returnAddressHostName.c_str(), &(this->m_returnAddress.sin_addr));
    
    std::unique_ptr<sockaddr_storage> temp{std::unique_ptr<sockaddr_storage>{new sockaddr_storage{}}};
    if (resolveAddressHelper (hostName, AF_INET, std::to_string(this->portNumber()), temp.get()) != 0) {
       throw std::runtime_error("ERROR: UDPClient could not resolve adress " + tQuoted(hostName));
    } else {
        inet_pton(AF_INET, inet_ntoa(reinterpret_cast<sockaddr_in *>(temp.get())->sin_addr), &(this->m_destinationAddress.sin_addr));
    }

    if (bind(this->m_udpSocketIndex, reinterpret_cast<sockaddr*>(&this->m_returnAddress), sizeof(this->m_returnAddress)) != 0) {
       throw std::runtime_error("ERROR: UDPClient could not bind socket " + tQuoted(this->m_udpSocketIndex) + " (is something else using it?)");
    }
    
}

int UDPClient::resolveAddressHelper(const std::string &hostName, int family, const std::string &service, sockaddr_storage* addressPtr)
{
    int result{0};
    addrinfo* resultList{nullptr};
    addrinfo hints{};
    hints.ai_family = family;
    hints.ai_socktype = SOCK_DGRAM;
    result = getaddrinfo(hostName.c_str(), service.c_str(), &hints, &resultList);
    if (result == 0) {
        memcpy(addressPtr, resultList->ai_addr, resultList->ai_addrlen);
        freeaddrinfo(resultList);
    }
    return result;
}

ssize_t UDPClient::writeByte(char toSend) 
{ 
    return this->writeLine(std::string{1, toSend}); 
}

ssize_t UDPClient::writeLine(const char *str) 
{ 
    return this->writeLine(static_cast<std::string>(str)); 
}

ssize_t UDPClient::writeLine(const std::string &str)
{
    std::string copyString{str};
    if (!endsWith(copyString, this->m_lineEnding)) {
        copyString += this->m_lineEnding;
    }
    unsigned int retryCount{0};
    do {
        ssize_t bytesWritten{sendto(this->m_udpSocketIndex, 
                            copyString.c_str(), 
                            strlen(copyString.c_str()),
                            MSG_DONTWAIT,
                            reinterpret_cast<sockaddr*>(&this->m_destinationAddress),
                            sizeof(this->m_destinationAddress)) };
        if ((bytesWritten != -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK))  {
            return bytesWritten;
        }
    } while (retryCount++ < UDPClient::SEND_RETRY_COUNT);
    return 0;
}

bool constexpr UDPClient::isValidPortNumber(int portNumber)
{
    return ((portNumber > 0) && (portNumber < std::numeric_limits<uint16_t>::max()));
}

std::string UDPClient::doUserSelectHostName()
{
    return doUserEnterStringParameter("Client Host Name", [](std::string str) -> bool { return true; });
                                                        //TODO: Add validation for host name
} 

uint16_t UDPClient::doUserSelectPortNumber()
{
    return doUserEnterNumericParameter("Client Port Number",
                                                         static_cast<std::function<bool(uint16_t)>>(UDPClient::isValidPortNumber),
                                                         std::numeric_limits<uint16_t>::min()+1,
                                                         std::numeric_limits<uint16_t>::max());
}

uint16_t UDPClient::doUserSelectReturnAddressPortNumber()
{
    return doUserEnterNumericParameter("Client Return Address Port Number",
                                                         static_cast<std::function<bool(uint16_t)>>(UDPClient::isValidPortNumber),
                                                         std::numeric_limits<uint16_t>::min()+1,
                                                         std::numeric_limits<uint16_t>::max());
}

std::shared_ptr<UDPClient> UDPClient::doUserSelectUDPClient()
{
    uint16_t portNumber{UDPClient::doUserSelectPortNumber()};
    std::string hostName{UDPClient::doUserSelectHostName()};
    uint16_t returnAddressPortNumber{UDPClient::doUserSelectReturnAddressPortNumber()};
    std::shared_ptr<UDPClient> udpClient{std::make_shared<UDPClient>(hostName, 
                                                                     portNumber, 
                                                                     returnAddressPortNumber)};
    return udpClient;
}

UDPClient::~UDPClient()
{
    shutdown(this->m_udpSocketIndex, SHUT_RDWR);
}




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
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        this->m_udpClient = std::unique_ptr<UDPClient>{new UDPClient{clientHostName, 
                                                                     clientPortNumber,
                                                                     clientReturnAddressPortNumber}};
    }
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
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
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->lineEnding();
    } else if (this->m_udpObjectType == UDPObjectType::Client) {
        return this->m_udpClient->lineEnding();
    } else {
        return this->m_udpServer->lineEnding() + " " + this->m_udpClient->lineEnding();
    }
}

void UDPDuplex::setLineEnding(const std::string &lineEnding)
{
    if (this->m_udpObjectType == UDPObjectType::Client) {
        this->m_udpClient->setLineEnding(lineEnding);
    } else if (this->m_udpObjectType == UDPObjectType::Server) {
        this->m_udpServer->setLineEnding(lineEnding);
    } else {
        this->m_udpClient->setLineEnding(lineEnding);
        this->m_udpServer->setLineEnding(lineEnding);
    }
}

void UDPDuplex::openPort()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        this->m_udpServer->openPort();
    } else if (this->m_udpObjectType == UDPObjectType::Client) {
        this->m_udpClient->openPort();
    } else {
        this->m_udpServer->openPort();
        this->m_udpClient->openPort();
    }
}

void UDPDuplex::closePort()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        this->m_udpServer->closePort();
    } else if (this->m_udpObjectType == UDPObjectType::Client) {
        this->m_udpClient->closePort();
    } else {
        this->m_udpServer->closePort();
        this->m_udpClient->closePort();
    }
}

bool UDPDuplex::isOpen() const
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->isOpen();
    } else if (this->m_udpObjectType == UDPObjectType::Client) {
        return this->m_udpClient->isOpen();
    } else {
        return ((this->m_udpClient->isOpen()) && (this->m_udpServer->isOpen()));
    }
}

void UDPDuplex::setClientPortNumber(uint16_t portNumber)
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
       this->m_udpClient->setPortNumber(portNumber);
    }
}

void UDPDuplex::setClientHostName(const std::string &hostName)
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        this->m_udpClient->setHostName(hostName);
    }
}

void UDPDuplex::setClientReturnAddressPortNumber(uint16_t portNumber)
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
       this->m_udpClient->setReturnAddressPortNumber(portNumber);
    }
}

void UDPDuplex::setClientTimeout(long timeout)
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        this->m_udpClient->setTimeout(timeout);   
    }
}

void UDPDuplex::setTimeout(long timeout)
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        this->m_udpServer->setTimeout(timeout);
    } else if (this->m_udpObjectType == UDPObjectType::Client) {
        this->m_udpClient->setTimeout(timeout);
    } else {
        this->m_udpClient->setTimeout(timeout);
        this->m_udpServer->setTimeout(timeout);
    }
}

uint16_t UDPDuplex::clientPortNumber() const
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpClient->portNumber();
    } else {
        return 0;
    }
}

std::string UDPDuplex::clientHostName() const
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpClient->hostName();
    } else {
        return "";
    }
}
    
uint16_t UDPDuplex::clientReturnAddressPortNumber() const
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpClient->m_udpSocketIndex;
    } else {
        return 0;
    }
}

long UDPDuplex::clientTimeout() const
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpClient->timeout();
    } else {
        return 0;
    }
}

uint16_t UDPDuplex::serverPortNumber() const
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpServer->portNumber();   
    } else {
        return 0;
    }
}

long UDPDuplex::serverTimeout() const
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpServer->timeout();
    } else {
        return 0;
    }
}

void UDPDuplex::setServerPortNumber(uint16_t portNumber)
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        this->m_udpServer->setPortNumber(portNumber);
    }
}

void UDPDuplex::setServerTimeout(long timeout)
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        this->m_udpServer->setTimeout(timeout); 
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        this->m_udpServer->setTimeout(this->m_udpClient->m_udpSocketIndex, timeout);
    }
}

long UDPDuplex::timeout() const
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->timeout();
    } else if (this->m_udpObjectType == UDPObjectType::Client) {
        return this->m_udpClient->timeout();
    } else {
        return (this->m_udpClient->timeout() > this->m_udpServer->timeout() 
                ? this->m_udpClient->timeout() : this->m_udpServer->timeout());
    }
    
}

std::string UDPDuplex::portName() const
{   
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpClient->hostName();
    } else {
        return "server - no port name";
    }
}

void UDPDuplex::flushRXTX()
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        this->m_udpServer->flushRXTX();
    }
} 

void UDPDuplex::flushRX()
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        this->m_udpServer->flushRX();
    }
}

void UDPDuplex::flushTX()
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        this->m_udpServer->flushTX();
    }
}  

ssize_t UDPDuplex::writeLine(const char *str)
{ 
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpClient->writeLine(str); 
    } else {
        return 0;
    }
}

ssize_t UDPDuplex::writeLine(const std::string &str)
{
    if ((this->m_udpObjectType == UDPObjectType::Client) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpClient->writeLine(str);
    } else {
        return 0;
    }
}

UDPDatagram UDPDuplex::readDatagram()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->readDatagram();
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->readDatagram(this->m_udpClient->m_udpSocketIndex);
    } else {
        return UDPDatagram{};
    }    
}

char UDPDuplex::readByte()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->readByte();
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->readByte(this->m_udpClient->m_udpSocketIndex);
    } else {
        return 0;
    }
}

std::string UDPDuplex::readLine()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->readLine();
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->readLine(this->m_udpClient->m_udpSocketIndex);
    } else {
        return "";
    }
}

std::string UDPDuplex::readUntil(const std::string &str)
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->readUntil(str);
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->readUntil(this->m_udpClient->m_udpSocketIndex, str);
    } else {
        return "";
    }
}

std::string UDPDuplex::readUntil(const char *str)
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->readUntil(str);
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->readUntil(this->m_udpClient->m_udpSocketIndex, str);
    }  else {
        return "";
    }
}

std::string UDPDuplex::readUntil(char until)
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->readUntil(until);
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->readUntil(this->m_udpClient->m_udpSocketIndex, until);
    } else {
        return "";
    }
}

void UDPDuplex::putBack(const UDPDatagram &datagram)
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpServer->putBack(datagram);
    }
}

void UDPDuplex::putBack(const std::string &str)
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpServer->putBack(str);
    }
}

void UDPDuplex::putBack(const char *str)
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpServer->putBack(str);
    }
}

void UDPDuplex::putBack(char back)
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpServer->putBack(back);
    }
}

std::string UDPDuplex::peek()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->peek();
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->peek(this->m_udpClient->m_udpSocketIndex);
    } else {
        return "";
    }
}

UDPDatagram UDPDuplex::peekDatagram()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->peekDatagram();
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->peekDatagram(this->m_udpClient->m_udpSocketIndex);
    } else {
        return UDPDatagram{};
    }
}

char UDPDuplex::peekByte()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->peekByte();
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->peekByte(this->m_udpClient->m_udpSocketIndex);
    } else {
        return 0;
    }
}
    
ssize_t UDPDuplex::available()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        return this->m_udpServer->available();
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        return this->m_udpServer->available(this->m_udpClient->m_udpSocketIndex);
    } else {
        return 0;
    }
}

void UDPDuplex::startListening()
{
    if (this->m_udpObjectType == UDPObjectType::Server) {
        this->m_udpServer->startListening();
    } else if (this->m_udpObjectType == UDPObjectType::Duplex) {
        this->m_udpServer->startListening(this->m_udpClient->m_udpSocketIndex);
    }
}

void UDPDuplex::stopListening()
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
        return this->m_udpServer->stopListening();
    }
}

bool UDPDuplex::isListening() const
{
    if ((this->m_udpObjectType == UDPObjectType::Server) || (this->m_udpObjectType == UDPObjectType::Duplex)) {
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
    std::vector<std::string> udpObjectTypeChoices{UDPDuplex::udpObjectTypeToString(UDPObjectType::Duplex),
                                                  UDPDuplex::udpObjectTypeToString(UDPObjectType::Server),
                                                  UDPDuplex::udpObjectTypeToString(UDPObjectType::Client)};
    return doUserSelectParameter("UDP Object Type",
                                                    static_cast< std::function<UDPObjectType(const std::string &)> >(UDPDuplex::parseUDPObjectTypeFromRaw),
                                                    udpObjectTypeChoices,
                                                    UDPDuplex::udpObjectTypeToString(UDPObjectType::Duplex).c_str());

}

std::shared_ptr<UDPDuplex> UDPDuplex::doUserSelectUDPDuplex()
{
    std::shared_ptr<UDPDuplex> udpDuplex{nullptr};
    UDPObjectType udpObjectType{UDPDuplex::doUserSelectUDPObjectType()};
    if (udpObjectType == UDPObjectType::Duplex) {
        std::string clientHostName{UDPDuplex::doUserSelectClientHostName()};
        uint16_t clientPortNumber{UDPDuplex::doUserSelectClientPortNumber()};
        uint16_t clientReturnAddressPortNumber{UDPDuplex::doUserSelectClientReturnAddressPortNumber()};
        uint16_t serverPortNumber{UDPDuplex::doUserSelectServerPortNumber()};
        udpDuplex = std::make_shared<UDPDuplex>(clientHostName, 
                                                clientPortNumber,
                                                serverPortNumber,
                                                clientReturnAddressPortNumber, 
                                                udpObjectType);

    } else if (udpObjectType == UDPObjectType::Server) {
        uint16_t serverPortNumber{UDPDuplex::doUserSelectServerPortNumber()};
        udpDuplex = std::make_shared<UDPDuplex>(serverPortNumber, udpObjectType);
    } else if (udpObjectType == UDPObjectType::Client) {
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
    std::string copyString{udpObjectType};
    std::transform(copyString.begin(), copyString.end(), copyString.begin(), ::tolower);
    std::string secondCopy{""};
    for (auto &it : copyString) {
        if (it != ' ') {
            secondCopy += it;
        }
    }
    if (secondCopy == "udpduplex") {
        return UDPObjectType::Duplex;
    } else if (secondCopy == "udpserver") {
        return UDPObjectType::Server;
    } else if (secondCopy == "udpclient") {
        return UDPObjectType::Client;
    } else {
        throw std::runtime_error("Could not parse UDPObjectType in UDPDuplex::parseUDPObjectTypeFromRaw(const std::string &): unknown identifier " + tQuoted(udpObjectType));
    }
}

std::string UDPDuplex::udpObjectTypeToString(UDPObjectType udpObjectType)
{
    if (udpObjectType == UDPObjectType::Duplex) {
        return "UDP Duplex";
    } else if (udpObjectType == UDPObjectType::Server) {
        return "UDP Server";
    } else if (udpObjectType == UDPObjectType::Client) {
        return "UDP Client";
    } else {
        throw std::runtime_error("Unknown UDPObjectType passed to UDPDuplex::udpObjectTypeToString(UDPObjectType)");
    }
}
