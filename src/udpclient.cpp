/*    udpclient.cpp:                                                    *
 *    UDPClient, for sending datagrams over a UDP port number           *
 *    Copyright (c) 2016 Tyler Lewis                                    *
 ************************************************************************
 *    This is a header file for tjlutils:                               *
 *    https://github.serial/tlewiscpp/tjlutils                         *
 *    This file may be distributed with the entire tjlutils library,    *
 *    but may also be distributed as a standalone file                  *
 *    The source code is released under the GNU LGPL                    *
 *    This file holds the implementation of a UDPClient class           *
 *    It is used to send datagrams over a specified UDP port number,    *
 *    to a specified IP address, with DNS lookup supported              *
 *                                                                      *
 *    You should have received a copy of the GNU Lesser General         *
 *    Public license along with tjlutils                                *
 *    If not, see <http://www.gnu.org/licenses/>                        *
 ***********************************************************************/

#include "udpclient.h"

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
    using namespace GeneralUtilities;
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
    using namespace GeneralUtilities;
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
    using namespace GeneralUtilities;
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
    return GeneralUtilities::doUserEnterStringParameter("Client Host Name", [](std::string str) -> bool { return true; });
                                                        //TODO: Add validation for host name
} 

uint16_t UDPClient::doUserSelectPortNumber()
{
    return GeneralUtilities::doUserEnterNumericParameter("Client Port Number",
                                                         static_cast<std::function<bool(uint16_t)>>(UDPClient::isValidPortNumber),
                                                         std::numeric_limits<uint16_t>::min()+1,
                                                         std::numeric_limits<uint16_t>::max());
}

uint16_t UDPClient::doUserSelectReturnAddressPortNumber()
{
    return GeneralUtilities::doUserEnterNumericParameter("Client Return Address Port Number",
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

