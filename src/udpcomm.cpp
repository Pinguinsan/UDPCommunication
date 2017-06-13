/***********************************************************************
*    udpcomm.cpp:                                                      *
*    Interface program for communicating over udp                      *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a source file for UDPCommunication:                       *
*    https://github.com/tlewiscpp/UDPCommunication                    *
*    The source code is released under the GNU LGPL                    *
*    This file holds the main logic for the UDPCommunication program   *
*    It is used to communicate via a unix UDP socket. It can be used   *
*    as a receiver (listener), master (sender), or both (duplex),      *
*    which can be selected via command line options (use -h)           *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with UDPCommunication                        *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <future>
#include <string>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <cstdio>
#include <set>
#include <map>
#include <list>
#include <queue>
#include <regex>
#include <unistd.h>
#include <signal.h>

#include <generalutilities.h>
#include <udpduplex.h>
#include <prettyprinter.h>

#include <tscriptexecutor.h>
#include <tscriptreader.h>

#define SIGNAL_STRING_BUFFER_SIZE 255

using namespace GeneralUtilities;

static const char *PROGRAM_NAME{"udpcomm"};
static const char *LONG_PROGRAM_NAME{"UDP Communication"};
static const char *AUTHOR_NAME{"Tyler Lewis"};
static const int SOFTWARE_MAJOR_VERSION{0};
static const int SOFTWARE_MINOR_VERSION{1};
static const int SOFTWARE_PATCH_VERSION{0};

#if defined(__GNUC__)
    static const char *COMPILER_NAME{"g++"};
    static const int COMPILER_MAJOR_VERSION{__GNUC__};
    static const int COMPILER_MINOR_VERSION{__GNUC_MINOR__};
    static const int COMPILER_PATCH_VERSION{__GNUC_PATCHLEVEL__};
#elif defined(_MSC_VER)
    static const char *COMPILER_NAME{"msvc"};
    static const int COMPILER_MAJOR_VERSION{_MSC_VER};
    static const int COMPILER_MINOR_VERSION{0};
    static const int COMPILER_PATCH_VERSION{0};
#else
    static const char *COMPILER_NAME{"unknown"};
    static const int COMPILER_MAJOR_VERSION{0};
    static const int COMPILER_MINOR_VERSION{0};
    static const int COMPILER_PATCH_VERSION{0};
#endif

static std::list<const char *> CLIENT_PORT_NUMBER_SWITCHES{"-p", "--p", "-port", "--port", "-port-number", "--port-number", "-client-port-number", "--client-port-number"};
static std::list<const char *> CLIENT_HOST_NAME_SWITCHES{"-n", "--n", "-name", "--name", "-client-name", "--client-name", "-host", "--host", "-host-name", "--host-name", "-client-host-name", "--client-host-name"};
static std::list<const char *> SERVER_PORT_NUMBER_SWITCHES{"-d", "--d", "-server-port", "--server-port", "-server-port-number", "--server-port-number"};
static std::list<const char *> CLIENT_RETURN_ADDRESS_PORT_NUMBER_SWITCHES{"-g", "--g", "-client-return-address-port-number", "--client-return-address-port-number"};

static std::list<const char *> SEND_ONLY_SWITCHES{"-s", "--s", "-send", "--send", "-send-only", "--send-only"};
static std::list<const char *> LINE_ENDING_SWITCHES{"-e", "--e", "-line-ending", "--line-ending", "-line-endings", "--line-endings"};
static std::list<const char *> RECEIVE_ONLY_SWITCHES{"-receive", "--receive", "-receive-only", "--receive-only"};
static std::list<const char *> SYNCHRONOUS_COMMUNICATION_SWITCHES{"-sync", "--sync", "-sync-comm", "--sync-comm"};
static std::list<const char *> SCRIPT_FILE_SWITCHES{"-c", "--c", "-script", "--script", "-script-file", "--script-file", "-script-name", "--script-name"};
static std::list<const char *> VERSION_SWITCHES{"-v", "--v", "-version", "--version"};
static std::list<const char *> HELP_SWITCHES{"-h", "--h", "-help", "--help"};

bool isValidIpAddress(const char *str);
bool isValidWebAddress(const char *str);
static std::unique_ptr<PrettyPrinter> prettyPrinter{std::unique_ptr<PrettyPrinter>{new PrettyPrinter{}}};

static const BackgroundColor COMMON_BACKGROUND_COLOR{BackgroundColor::BG_DEFAULT};
static const int COMMON_FONT_ATTRIBUTE{(FontAttribute::FA_BOLD | FontAttribute::FA_UNDERLINED)};
static const ForegroundColor TX_COLOR{ForegroundColor::FG_BLUE};
static const ForegroundColor RX_COLOR{ForegroundColor::FG_RED};
static const ForegroundColor DELAY_COLOR{ForegroundColor::FG_GREEN};
static const ForegroundColor FLUSH_COLOR{ForegroundColor::FG_DARK_GRAY};
static const ForegroundColor LOOP_COLOR{ForegroundColor::FG_CYAN};
static const ForegroundColor LIST_COLOR{ForegroundColor::FG_YELLOW};

static const int TX_RESULT_WHITESPACE{4};
static const int RX_RESULT_WHITESPACE{4};
static const int DELAY_RESULT_WHITESPACE{4};
static const int FLUSH_RESULT_WHITESPACE{4};
static const int LOOP_RESULT_WHITESPACE{4};

void sendUDPString(const std::string &str);
std::string doUDPreadLine();
UDPObjectType udpObjectType{UDPObjectType::UDP_DUPLEX};

int countOccurences(const std::string &str, const std::string &id);
int getHistoryIndex(const std::string &str);

void printRxResult(const std::string &str);
void printTxResult(const std::string &str);
void printDelayResult(DelayType delayType, int howLong);
void printFlushResult(FlushType flushType);
void printLoopResult(LoopType loopType, int currentLoop, int loopCount);
std::string getPrettyLineEndings(const std::string &lineEnding);

static unsigned int currentCommandHistoryIndex{0};
static std::list<std::string> commandHistory;

static std::set<std::string> scriptFiles;
static std::map<std::string, std::unique_ptr<TScriptExecutor>> scriptFileMap;

void backspaceTerminal(unsigned int howFar);
void displayHelp();
void displayVersion();
void doAtExit();
void interruptHandler(int signalNumber);
void installSignalHandlers(void (*signalHandler)(int));

using StringFuture = std::future<std::string>;
using FuturePtr = std::unique_ptr<StringFuture>;

static std::mutex ioMutex;

std::string asyncStdinTask();
void startAsyncStdinTask(const std::function<std::string(void)> &func);
static FuturePtr asyncStdinTaskFuture;
static std::function<std::string(void)> packagedAsyncStdinTask{asyncStdinTask};

std::string asyncStdoutTask();
void startAsyncStdoutTask(const std::function<std::string(void)> &func);
static FuturePtr asyncStdoutTaskFuture;
static std::function<std::string(void)> packagedAsyncStdoutTask{asyncStdoutTask};

static std::function<void(const std::string&)> packagedRxResultTask{printRxResult};
static std::function<void(const std::string&)> packagedTxResultTask{printTxResult};
static std::function<void(DelayType, int)> packagedDelayResultTask{printDelayResult};
static std::function<void(FlushType)> packagedFlushResultTask{printFlushResult};
static std::function<void(LoopType, int, int)> packagedLoopResultTask{printLoopResult};
static std::shared_ptr<UDPDuplex> udpDuplex{nullptr};
static std::shared_ptr<UDPClient> udpClient{nullptr};
static std::shared_ptr<UDPServer> udpServer{nullptr};

static std::string clientHostName{UDPDuplex::DEFAULT_CLIENT_HOST_NAME};
static std::string clientPortNumber{std::to_string(UDPDuplex::DEFAULT_CLIENT_PORT_NUMBER)};
static std::string serverPortNumber{std::to_string(UDPDuplex::DEFAULT_SERVER_PORT_NUMBER)};
static std::string clientReturnAddressPortNumber{std::to_string(serverPortNumber)};

static bool sendOnly{false};
static bool receiveOnly{false};
static bool synchronousCommunication{false};
static std::vector<std::string> previousStringSent{};
static std::string lineEndings{""};

const uint16_t MAXIMUM_PORT_NUMBER{std::numeric_limits<uint16_t>::max()};

int main(int argc, char *argv[])
{
    installSignalHandlers(interruptHandler);
    
    for (int i = 1; i < argc; i++) { 
        if (isSwitch(argv[i], HELP_SWITCHES)) {
            displayHelp();
            return 0;
        } else if (isSwitch(argv[i], VERSION_SWITCHES)) {
            displayVersion();
            return 0;
        }
    }
    displayVersion();

    for (int i = 1; i < argc; i++) {
        if (isSwitch(argv[i], CLIENT_HOST_NAME_SWITCHES)) {
            if (argv[i+1]) {
                clientHostName = argv[i+1];
            } else {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no client host name was specified after, skipping option" << std::endl;
            }
            i++;
        } else if (isEqualsSwitch(argv[i], CLIENT_HOST_NAME_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no client host name was specified after, skipping option" << std::endl;
            } else {
                clientHostName = stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"");
            }
        } else if (isSwitch(argv[i], CLIENT_PORT_NUMBER_SWITCHES)) {
            if (argv[i+1]) {
                std::string maybePortString{static_cast<std::string>(argv[i+1])};
                int maybePort{0};
                try {
                    maybePort = std::stoi(maybePortString);
                    if (maybePort < 0) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i+1]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a positive number (" << maybePort << " < 0), skipping option" << std::endl;
                    } else if (maybePort > MAXIMUM_PORT_NUMBER) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i+1]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not greater than maximum port number (" << maybePort << " > " << MAXIMUM_PORT_NUMBER << "), skipping option" << std::endl;
                    } else {
                        clientPortNumber = static_cast<std::string>(maybePortString);
                    }
                } catch (std::exception &e) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a number between 0 and " << MAXIMUM_PORT_NUMBER << ", skipping option" << std::endl;
                }
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no client port number was specified after, skipping option" << std::endl;
            }
            i++;
        } else if (isEqualsSwitch(argv[i], CLIENT_PORT_NUMBER_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no client port number was specified after, skipping option" << std::endl;
            } else {
                std::string maybePortString{stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")};
                int maybePort{0};
                try {
                    maybePort = std::stoi(maybePortString);
                    if (maybePort < 0) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a positive number (" << maybePort << " < 0), skipping option" << std::endl;
                    } else if (maybePort > MAXIMUM_PORT_NUMBER) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not greater than maximum port number (" << maybePort << " > " << MAXIMUM_PORT_NUMBER << "), skipping option" << std::endl;
                    } else {
                        clientPortNumber = static_cast<std::string>(maybePortString);
                    }
                } catch (std::exception &e) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a number between 0 and " << MAXIMUM_PORT_NUMBER << ", skipping option" << std::endl;
                }
            }
        } else if (isSwitch(argv[i], SERVER_PORT_NUMBER_SWITCHES)) {
            if (argv[i+1]) {
                std::string maybePortString{static_cast<std::string>(argv[i+1])};
                int maybePort{0};
                try {
                    maybePort = std::stoi(maybePortString);
                    if (maybePort < 0) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified server port number " << tQuoted(maybePortString) << " is not a positive number (" << maybePort << " < 0), skipping option" << std::endl;
                    } else if (maybePort > MAXIMUM_PORT_NUMBER) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified server port number " << tQuoted(maybePortString) << " is not greater than maximum port number (" << maybePort << " > " << MAXIMUM_PORT_NUMBER << "), skipping option" << std::endl;
                    } else {
                        serverPortNumber = static_cast<std::string>(maybePortString);
                    }
                } catch (std::exception &e) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified server port number " << tQuoted(maybePortString) << " is not a number between 0 and " << MAXIMUM_PORT_NUMBER << ", skipping option" << std::endl;
                }
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no server host name name was specified after, skipping option" << std::endl;
            }
            i++;
        } else if (isEqualsSwitch(argv[i], SERVER_PORT_NUMBER_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no client port number was specified after, skipping option" << std::endl;
            } else {
                std::string maybePortString{stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")};
                int maybePort{0};
                try {
                    maybePort = std::stoi(maybePortString);
                    if (maybePort < 0) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a positive number (" << maybePort << " < 0), skipping option" << std::endl;
                    } else if (maybePort > MAXIMUM_PORT_NUMBER) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not greater than maximum port number (" << maybePort << " > " << MAXIMUM_PORT_NUMBER << "), skipping option" << std::endl;
                    } else {
                        serverPortNumber = maybePortString;
                    }
                } catch (std::exception &e) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a number between 0 and " << MAXIMUM_PORT_NUMBER << ", skipping option" << std::endl;
                }
            }
        } else if (isSwitch(argv[i], CLIENT_RETURN_ADDRESS_PORT_NUMBER_SWITCHES)) {
            if (argv[i+1]) {
                std::string maybePortString{static_cast<std::string>(argv[i+1])};
                int maybePort{0};
                try {
                    maybePort = std::stoi(maybePortString);
                    if (maybePort < 0) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified server port number " << tQuoted(maybePortString) << " is not a positive number (" << maybePort << " < 0), skipping option" << std::endl;
                    } else if (maybePort > MAXIMUM_PORT_NUMBER) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified server port number " << tQuoted(maybePortString) << " is not greater than maximum port number (" << maybePort << " > " << MAXIMUM_PORT_NUMBER << "), skipping option" << std::endl;
                    } else {
                        clientReturnAddressPortNumber = static_cast<std::string>(maybePortString);
                    }
                } catch (std::exception &e) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified server port number " << tQuoted(maybePortString) << " is not a number between 0 and " << MAXIMUM_PORT_NUMBER << ", skipping option" << std::endl;
                }
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no server host name name was specified after, skipping option" << std::endl;
            }
            i++;
        } else if (isEqualsSwitch(argv[i], CLIENT_RETURN_ADDRESS_PORT_NUMBER_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no client port number was specified after, skipping option" << std::endl;
            } else {
                std::string maybePortString{stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")};
                int maybePort{0};
                try {
                    maybePort = std::stoi(maybePortString);
                    if (maybePort < 0) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a positive number (" << maybePort << " < 0), skipping option" << std::endl;
                    } else if (maybePort > MAXIMUM_PORT_NUMBER) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not greater than maximum port number (" << maybePort << " > " << MAXIMUM_PORT_NUMBER << "), skipping option" << std::endl;
                    } else {
                        clientReturnAddressPortNumber = maybePortString;
                    }
                } catch (std::exception &e) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a number between 0 and " << MAXIMUM_PORT_NUMBER << ", skipping option" << std::endl;
                }
            }
        } else if (isSwitch(argv[i], LINE_ENDING_SWITCHES)) {
            if (lineEndings != "") {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but lineEndings have already been set by another option (" << lineEndings << "), skipping option" << std::endl;
            } else if (argv[i+1]) {
                try {
                    lineEndings = static_cast<std::string>(argv[i+1]);
                } catch (std::exception &e) {
                    (void)e;
                    std::cout << "WARNING: Switch " << argv[i] << " accepted, but " << std::quoted(argv[i+1]) << " is an invalid line ending, skipping option";
                }
                i++;
            } else {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no line endings were specified after, skipping option" << std::endl;
            }
        } else if (isEqualsSwitch(argv[i], LINE_ENDING_SWITCHES)) {
            if (lineEndings != "") {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but lineEndings have already been set by another option (" << lineEndings << "), skipping option" << std::endl;
            } else {
                std::string copyString{static_cast<std::string>(argv[i])};
                size_t foundPosition{copyString.find("=")};
                size_t foundEnd{copyString.substr(foundPosition).find(" ")};
                if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                    std::cout << "WARNING: Switch " << argv[i] << " accepted, but no line endings were specified after, skipping option" << std::endl;
                } else {
                    try {
                        lineEndings = stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"");
                    } catch (std::exception &e) {
                        std::cout << "WARNING: Switch " << argv[i] << " accepted, but " << std::quoted(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")) << " is an invalid line ending option, skipping option" << std::endl;
                    }
                }   
            }
        }  else if (isSwitch(argv[i], SCRIPT_FILE_SWITCHES)) {
            if (argv[i+1]) {
                scriptFiles.emplace(static_cast<std::string>(argv[i+1]));
            } else {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no stop bits number were specified after, skipping option" << std::endl;
            }
            i++;
        } else if (isEqualsSwitch(argv[i], SCRIPT_FILE_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no script file were specified after, skipping option" << std::endl;
            } else {
                scriptFiles.emplace(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\""));
            }  
        } else if (isSwitch(argv[i], SEND_ONLY_SWITCHES)) {
            if (receiveOnly) {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but ReceiveOnly option is already enabled, skipping option" << std::endl;
            } else if (synchronousCommunication) {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but synchronousCommunication option is already enabled, skipping option" << std::endl;
            } else {
                sendOnly = true;
            }
        } else if (isSwitch(argv[i], RECEIVE_ONLY_SWITCHES)) {
            if (sendOnly) {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but SendOnly option is already enabled, skipping option" << std::endl;
            } else if (synchronousCommunication) {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but synchronousCommunication option is already enabled, skipping option" << std::endl;
            } else {
                receiveOnly = true;
            }
        } else if (isSwitch(argv[i], SYNCHRONOUS_COMMUNICATION_SWITCHES)) {
            if (sendOnly) {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but SendOnly option is already enabled, skipping option" << std::endl;
            } else if (receiveOnly) {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but ReceiveOnly option is already enabled, skipping option" << std::endl;
            } else {
                synchronousCommunication = true;
            }
        } else if (((isValidIpAddress(argv[i])) || (isValidWebAddress(argv[i]))) && (!startsWith(std::string{argv[i]}, "-"))) {
            if (clientHostName == UDPDuplex::DEFAULT_CLIENT_HOST_NAME) {
                clientHostName = argv[i];
            } else {
                std::cout << "WARNING: Host name \"" << argv[i] << "\" accepted, but client host name has already been specified (clientHostName = " << tQuoted(clientHostName) << "), skipping option" << std::endl;
            }
        } else {
            std::cout << "WARNING: Switch " << argv[i] << " is an invalid option, skipping" << std::endl;
        }
    }

    if (receiveOnly) {
        
    } else if (sendOnly) {
        
    } else {

    }

    prettyPrinter->setForegroundColor(LIST_COLOR);
    prettyPrinter->setFontAttributes(COMMON_FONT_ATTRIBUTE);
    std::cout << "Using ClientHostName=";
    prettyPrinter->println(clientHostName);
    
    std::cout << "Using ClientPortNumber=";
    prettyPrinter->println(clientPortNumber);
    
    std::cout << "Using ServerPortNumber=";
    if (udpObjectType == UDPObjectType::UDP_SERVER) {
        prettyPrinter->println(serverPortNumber);
    } else {
        prettyPrinter->println(clientReturnAddressPortNumber);
    }

    std::cout << "Using ClientReturnAddressPortNumber=";
    prettyPrinter->println(clientReturnAddressPortNumber);
    
    std::cout << "Using LineEndings=";
    prettyPrinter->println(getPrettyLineEndings(lineEndings));

    int i{1};
    for (auto &it : scriptFiles) {
        std::cout << "Using ScriptFile=" << it << " (" << i++ << "/" << scriptFiles.size() << ")" << std::endl;
    }
    if (receiveOnly) {
        udpObjectType = UDPObjectType::UDP_SERVER;
    } else if (sendOnly) {
        udpObjectType = UDPObjectType::UDP_CLIENT;
    } else {
        udpObjectType = UDPObjectType::UDP_DUPLEX;
    }
    std::cout << std::endl;
    try {
            udpDuplex = std::make_shared<UDPDuplex>(clientHostName,
                                                    std::stoi(clientPortNumber), 
                                                    std::stoi(serverPortNumber),
                                                    std::stoi(clientReturnAddressPortNumber),
                                                    udpObjectType);
        try {
            udpDuplex->openPort();
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            return 1;
        }
        delayMilliseconds(500);
        udpDuplex->setTimeout(25);        
        std::cout << "Successfully opened UDP port ";
        prettyPrinter->println(udpDuplex->portName() + "\n");
        for (auto &it : scriptFiles) {
            scriptFileMap.emplace(it, std::unique_ptr<TScriptExecutor>{new TScriptExecutor{it}});
        }
        i = 1;
        for (auto &it : scriptFileMap) {
            if (!it.second->hasCommands()) {
                std::cout << "ScriptFile " << it.first << " (" << i++ << "/" << scriptFiles.size() << ") has no commands, skipping script" << std::endl;
                continue;
            }
            std::cout << "Executing ScriptFile " << it.first << " (" << i++ << "/" << scriptFiles.size() << ")" << std::endl;
            it.second->execute(udpDuplex, 
                               packagedRxResultTask, 
                               packagedTxResultTask, 
                               packagedDelayResultTask, 
                               packagedFlushResultTask, 
                               packagedLoopResultTask);
        }
        delayMilliseconds(250);
        udpDuplex->flushRXTX();
        prettyPrinter->setBackgroundColor(COMMON_BACKGROUND_COLOR);
        prettyPrinter->setFontAttributes(COMMON_FONT_ATTRIBUTE);
        std::string returnString{""};
        std::string stringToSend{""};
        if (sendOnly) {
            std::cout << "Beginning ";
            prettyPrinter->print("send-only");
            std::cout << " communication loop, enter desired string and press enter to send strings, or press CTRL+C to quit" << std::endl << std::endl;
            while (true) {
                std::getline(std::cin, stringToSend);
                stringToSend = stripNonAsciiCharacters(stringToSend);
                if ((stringToSend.find("[A") == 0) || (stringToSend.find("[B")) == 0) {
                    stringToSend = previousStringSent.at(getHistoryIndex(stringToSend));
                }
                //Strip stupid [B and [C control characters from Cygwin shell
                for (char i = 'C'; i < 'Z'; i++) {
                    stringToSend = stripAllFromString(stringToSend, (std::string{1, '['} + std::string{1, static_cast<char>(i)}));
                }
                sendUDPString(stringToSend);
            }
        } else if (receiveOnly) {
            std::cout << "Beginning ";
            prettyPrinter->print("receive-only");
            std::cout << " communication loop, messages received will be displayed, or press CTRL+C to quit" << std::endl << std::endl;
            startAsyncStdoutTask(packagedAsyncStdoutTask);
            while (true) {
                if (asyncStdoutTaskFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    printRxResult(asyncStdoutTaskFuture->get());
                    startAsyncStdoutTask(packagedAsyncStdoutTask);
                }  
            }   
        } else if (synchronousCommunication) {
            std::cout << "Beginning ";
            prettyPrinter->print("synchronous");
            std::cout << " communication loop, enter desired string and press enter to send strings, or press CTRL+C to quit" << std::endl << std::endl;
            while (true) {
                std::getline(std::cin, stringToSend);
                stringToSend = stripNonAsciiCharacters(stringToSend);
                if ((stringToSend.find("[A") == 0) || (stringToSend.find("[B")) == 0) {
                    stringToSend = previousStringSent.at(getHistoryIndex(stringToSend));
                }
                //Strip stupid [B and [C control characters from Cygwin shell
                for (char i = 'C'; i < 'Z'; i++) {
                    stringToSend = stripAllFromString(stringToSend, (std::string{1, '['} + std::string{1, static_cast<char>(i)}));
                }
                if ((stringToSend != "") && (!isWhitespace(stringToSend))) {
                    sendUDPString(stringToSend);
                }
                returnString = doUDPreadLine();
                if (returnString != "") {
                    printRxResult(returnString);
                }
            }
        } else {
            std::cout << "Beginning ";
            prettyPrinter->print("asynchronous");
            std::cout << " communication loop, enter desired string and press enter to send strings, or press CTRL+C to quit" << std::endl << std::endl;
            startAsyncStdinTask(packagedAsyncStdinTask);
            startAsyncStdoutTask(packagedAsyncStdoutTask);
            while(true) {
                if (asyncStdinTaskFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    std::string temp{stripNonAsciiCharacters(asyncStdinTaskFuture->get())};
                    if ((temp.find("[A") == 0) || (temp.find("[B")) == 0) {
                        stringToSend = getHistoryIndex(temp);
                    } else {
                        stringToSend = temp;
                    }
                    //Strip stupid [B and [C control characters from Cygwin shell
                    for (char i = 'C'; i < 'Z'; i++) {
                        stringToSend = stripAllFromString(stringToSend, (std::string{1, '['} + std::string{1, static_cast<char>(i)}));
                    }
                    sendUDPString(stringToSend);
                    startAsyncStdinTask(packagedAsyncStdinTask);
                }
                if (asyncStdoutTaskFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    printRxResult(asyncStdoutTaskFuture->get());
                    startAsyncStdoutTask(packagedAsyncStdoutTask);
                }  
            }
        }
        udpDuplex->closePort();
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int countOccurences(const std::string &str, const std::string &id)
{
    int counter{0};
    std::string::size_type start{0};
    while ((start = str.find(id, start)) != std::string::npos) {
        counter++;
        start += id.length();
    }
    return counter;
}

int getHistoryIndex(const std::string &str)
{
    int indexPlusCount{countOccurences(str, "[A")};
    int indexMinusCount{countOccurences(str, "[B")};
    int rawIndex{indexPlusCount - indexMinusCount};
    if (rawIndex < 0) {
        rawIndex = 0;
    } else if (rawIndex >= previousStringSent.size()) {
        rawIndex = previousStringSent.size() - 1;
    }
    return rawIndex;
}

void displayHelp() 
{
    std::cout << "Usage: " << PROGRAM_NAME << " [options][=][argument]" << std::endl << std::endl;
    std::cout << "Options: " << std::endl;
    std::cout << "    -n, --name, -client-host-name, --client-host-name: Specify where to send datagrams (host name)" << std::endl;
    std::cout << "    -p, --p, -client-port-number, --client-port-number: Specify which port to send datagrams to" << std::endl;
    std::cout << "    -d, --d, -server-port-number, --server-port-number: Specify which port to receive datagrams from" << std::endl;
    std::cout << "    -c, --c, -script-file, --script-file: Specify script file to be run after serial port is opened" << std::endl;
    std::cout << "    -e, --e, -line-ending, --line-ending: Specify what type of line ending should be used" << std::endl;
    std::cout << "    -a, --a, -client-return-address-host-name: Specify the return address host name for the UDP client" << std::endl;
    std::cout << "    -g, --g, -client-return-address-port-number: Specify the return address port number for the UDP client" << std::endl; 
    std::cout << "    -h, --h, -help, --help: Show this help text" << std::endl;
    std::cout << "    -v, --v, -version, --version: Display version" << std::endl;
    std::cout << "Example: " << std::endl;
    std::cout << "    Command line input: udpcomm --line-ending=cr --client-host-name=www.google.com --client-port-number=8887 --server-port-number=8888" << std::endl;
    std::cout << "    Output:" << std::endl;
    std::cout << "        Using ClientHostName=www.google.com" << std::endl;
    std::cout << "        Using ClientPortNumber=8887" << std::endl;
    std::cout << "        Using ServerPortNumber=8888" << std::endl;
    std::cout << "        Using LineEnding=\\r (Carriage Return)" << std::endl;
    std::cout << "        Successfully opened udp port www.google.com" << std::endl;
    std::cout << "        Rx << 0123456789" << std::endl;
}

void displayVersion() 
{
    std::cout << PROGRAM_NAME << ", v" << SOFTWARE_MAJOR_VERSION << "." << SOFTWARE_MINOR_VERSION << "." << SOFTWARE_PATCH_VERSION << std::endl;
    std::cout << "Written by " << AUTHOR_NAME << ", " << std::endl;
    std::cout << "Built with " << COMPILER_NAME << " v" << COMPILER_MAJOR_VERSION << "." << COMPILER_MINOR_VERSION << "." << COMPILER_PATCH_VERSION << ", " << __DATE__ << std::endl << std::endl;
}

void doAtExit()
{
    return interruptHandler(0);
}

void interruptHandler(int signalNumber)
{
    if ((signalNumber == SIGUSR1) || (signalNumber == SIGUSR2) || (signalNumber == SIGCHLD)) {
        return;
    }
    std::unique_ptr<char[]> signalString{new char[SIGNAL_STRING_BUFFER_SIZE]};
    memset(signalString.get(), '\0', SIGNAL_STRING_BUFFER_SIZE);
    signalString.reset(strsignal(signalNumber));
    std::cout << std::endl << "Caught signal " << signalNumber << " (" << signalString.get() << "), exiting " << PROGRAM_NAME << std::endl;

    if (udpDuplex) {
        //udpDuplex->closePort();
    }
    exit (signalNumber);
}

void installSignalHandlers(void (*signalHandler)(int))
{
    static struct sigaction signalInterruptHandler;
    signalInterruptHandler.sa_handler = signalHandler;
    sigemptyset(&signalInterruptHandler.sa_mask);
    signalInterruptHandler.sa_flags = 0;
    sigaction(SIGHUP, &signalInterruptHandler, NULL);
    sigaction(SIGINT, &signalInterruptHandler, NULL);
    sigaction(SIGQUIT, &signalInterruptHandler, NULL);
    sigaction(SIGILL, &signalInterruptHandler, NULL);
    sigaction(SIGABRT, &signalInterruptHandler, NULL);
    sigaction(SIGFPE, &signalInterruptHandler, NULL);
    sigaction(SIGPIPE, &signalInterruptHandler, NULL);
    sigaction(SIGALRM, &signalInterruptHandler, NULL);
    sigaction(SIGTERM, &signalInterruptHandler, NULL);
    sigaction(SIGUSR1, &signalInterruptHandler, NULL);
    sigaction(SIGUSR2, &signalInterruptHandler, NULL);
    sigaction(SIGCHLD, &signalInterruptHandler, NULL);
    sigaction(SIGCONT, &signalInterruptHandler, NULL);
    sigaction(SIGTSTP, &signalInterruptHandler, NULL);
    sigaction(SIGTTIN, &signalInterruptHandler, NULL);
    sigaction(SIGTTOU, &signalInterruptHandler, NULL);
}

void startAsyncStdinTask(const std::function<std::string(void)> &func)
{
    asyncStdinTaskFuture = FuturePtr{new StringFuture{std::async(std::launch::async, func)}};
}

void startAsyncStdoutTask(const std::function<std::string(void)> &func)
{
    asyncStdoutTaskFuture = FuturePtr{new StringFuture{std::async(std::launch::async, func)}};
}

std::string asyncStdinTask()
{
    std::string returnString{""};
    std::getline(std::cin, returnString);
    return returnString;
}

std::string asyncStdoutTask()
{
    using namespace GeneralUtilities;
    std::string returnString{""};
    do {
        if (!udpDuplex) {
            return "";
        }
        if (udpDuplex->available()) {
            returnString += udpDuplex->readLine();
        }
    } while ((returnString.length() == 0) || (isWhitespace(returnString)));


    return returnString;
}

void backspaceTerminal(unsigned int howFar)
{
    std::unique_lock<std::mutex> ioLock{ioMutex};
    for (unsigned int i = 0; i < howFar; i++) {
        std::cout << '\b';
    }
    for (unsigned int i = 0; i < howFar; i++) {
        std::cout << ' ';
    }
    for (unsigned int i = 0; i < howFar; i++) {
        std::cout << '\b';
    }
}

std::string doUDPreadLine()
{
    if (udpDuplex->available()) {
        return udpDuplex->readLine();
    } else {
        return "";
    }
}

void sendUDPString(const std::string &str)
{
    udpDuplex->writeLine(str);
    if ((str != "") && (!isWhitespace(str))) {
        previousStringSent.insert(previousStringSent.begin(), str);
    }
    std::cout << "\033[1A\r"; // Goes back up a line and clears the line
    printTxResult(str);
}

void printRxResult(const std::string &str)
{
    using namespace GeneralUtilities;
    std::unique_lock<std::mutex> ioLock{ioMutex};
    prettyPrinter->setForegroundColor(RX_COLOR);
    std::cout << tWhitespace(RX_RESULT_WHITESPACE);
    prettyPrinter->print("Rx << " + str);
    std::cout << std::endl;
}

void printTxResult(const std::string &str)
{
    using namespace GeneralUtilities;
    std::unique_lock<std::mutex> ioLock{ioMutex};
    prettyPrinter->setForegroundColor(TX_COLOR);
    std::cout << tWhitespace(TX_RESULT_WHITESPACE);
    prettyPrinter->print("Tx >> " + str);
    std::cout << std::endl;
}

void printDelayResult(DelayType delayType, int howLong)
{
    using namespace GeneralUtilities;
    std::unique_lock<std::mutex> ioLock{ioMutex};
    prettyPrinter->setForegroundColor(DELAY_COLOR);
    std::string stringToPrint{"Delay <> " + std::to_string(howLong)};
    if (delayType == DelayType::SECONDS) {
        stringToPrint += "sec";
    } else if (delayType == DelayType::MILLISECONDS) {
        stringToPrint += "ms";
    } else if (delayType == DelayType::MICROSECONDS) {
        stringToPrint += "us";
    }
    std::cout << tWhitespace(RX_RESULT_WHITESPACE);
    prettyPrinter->print(stringToPrint);
    std::cout << std::endl;

}

void printFlushResult(FlushType flushType)
{
    using namespace GeneralUtilities;
    std::unique_lock<std::mutex> ioLock{ioMutex};
    prettyPrinter->setForegroundColor(FLUSH_COLOR);
    std::string stringToPrint{"Flush "};
    if (flushType == FlushType::RX) {
        stringToPrint += "vv ";
    } else if (flushType == FlushType::TX) {
        stringToPrint += "^^ ";
    } else if (flushType == FlushType::RX_TX) {
        stringToPrint += "^v";
    }
    std::cout << tWhitespace(FLUSH_RESULT_WHITESPACE);
    prettyPrinter->print(stringToPrint);
    std::cout << std::endl;
}

void printLoopResult(LoopType loopType, int currentLoop, int loopCount)
{
    using namespace GeneralUtilities;
    std::unique_lock<std::mutex> ioLock{ioMutex};
    prettyPrinter->setForegroundColor(LOOP_COLOR);
    if (loopCount == -1) {
        if (loopType == LoopType::START) {
            if (currentLoop == 0) {
                std::cout << tWhitespace(LOOP_RESULT_WHITESPACE);
                prettyPrinter->print("***Beginning infinite loop***");
                std::cout << std::endl;
            }
            std::cout << tWhitespace(LOOP_RESULT_WHITESPACE);
            prettyPrinter->print("Begin loop (" + std::to_string(currentLoop + 1) + "/infinite)");
            std::cout << std::endl;
        } else if (loopType == LoopType::END) {
            std::cout << tWhitespace(LOOP_RESULT_WHITESPACE);
            prettyPrinter->print("End loop (" + std::to_string(currentLoop + 1) + "/infinite)");
            std::cout << std::endl;
        }
    } else {
        if (loopType == LoopType::START) {
            if (currentLoop == 0) {
                std::cout << tWhitespace(LOOP_RESULT_WHITESPACE);
                prettyPrinter->print("***Beginning " + std::to_string(loopCount) + " loops***");
                std::cout << std::endl;
            }
            std::cout << tWhitespace(LOOP_RESULT_WHITESPACE);
            prettyPrinter->print("Begin loop (" + std::to_string(currentLoop + 1) + "/" + std::to_string(loopCount) + ")");
            std::cout << std::endl;
        } else if (loopType == LoopType::END) {
            std::cout << tWhitespace(LOOP_RESULT_WHITESPACE);
            prettyPrinter->print("End loop (" + std::to_string(currentLoop + 1) + "/" + std::to_string(loopCount) + ")");
            std::cout << std::endl;
            if (currentLoop+1 == loopCount) {
                std::cout << tWhitespace(LOOP_RESULT_WHITESPACE);
                prettyPrinter->print("***Ending " + std::to_string(loopCount) + " loops***");
                std::cout << std::endl;
            }
        }
    }
}

bool isValidIpAddress(const char *str)
{
    std::string copyString{str};
    std::vector<std::string> result{GeneralUtilities::parseToContainer<std::vector<std::string>, std::string::const_iterator>(copyString.begin(), copyString.end(), '.')};
    if (result.size() != 4) {
        return false;
    }
    for (auto &it : result) {
        try {
            int test{std::stoi(it)};
            if ((test < 0) || (test > 255)) {
                return false;
            }
        } catch (std::exception &e) {
            (void)e;
            return false;
        }
    }
    return true;
}

bool isValidWebAddress(const char *str)
{
    if (!str) {
        return false;
    }
    std::string copyString{str};
    /*
     * lol
     * Thanks eyelidlessness
     * http://stackoverflow.com/questions/161738/what-is-the-best-regular-expression-to-check-if-a-string-is-a-valid-url
     * and Ciro Costa
     * http://stackoverflow.com/questions/5620235/cpp-regular-expression-to-validate-url
     */
    //static const std::regex targetRegex{R"lol(/^[a-z](?:[-a-z0-9\+\.])*:(?:\/\/(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:])*@)?(?:\[(?:(?:(?:[0-9a-f]{1,4}:){6}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|::(?:[0-9a-f]{1,4}:){5}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:[0-9a-f]{1,4})?::(?:[0-9a-f]{1,4}:){4}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:[0-9a-f]{1,4}:[0-9a-f]{1,4})?::(?:[0-9a-f]{1,4}:){3}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:(?:[0-9a-f]{1,4}:){0,2}[0-9a-f]{1,4})?::(?:[0-9a-f]{1,4}:){2}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:(?:[0-9a-f]{1,4}:){0,3}[0-9a-f]{1,4})?::[0-9a-f]{1,4}:(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:(?:[0-9a-f]{1,4}:){0,4}[0-9a-f]{1,4})?::(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:(?:[0-9a-f]{1,4}:){0,5}[0-9a-f]{1,4})?::[0-9a-f]{1,4}|(?:(?:[0-9a-f]{1,4}:){0,6}[0-9a-f]{1,4})?::)|v[0-9a-f]+[-a-z0-9\._~!\$&'\(\)\*\+,;=:]+)\]|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3}|(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=@])*)(?::[0-9]*)?(?:\/(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))*)*|\/(?:(?:(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))+)(?:\/(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))*)*)?|(?:(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))+)(?:\/(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))*)*|(?!(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@])))(?:\?(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@])|[\x{E000}-\x{F8FF}\x{F0000}-\x{FFFFD}\x{100000}-\x{10FFFD}\/\?])*)?(?:\#(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@])|[\/\?])*)?$/i)lol", std::regex::extended};
    //static const std::regex targetRegex{R"lol(/((([A-Za-z]{3,9}:(?:\/\/)?)(?:[-;:&=\+\$,\w]+@)?[A-Za-z0-9.-]+|(?:www.|[-;:&=\+\$,\w]+@)[A-Za-z0-9.-]+)((?:\/[\+~%\/.\w-_]*)?\??(?:[-\+=&;%@.\w_]*)#?(?:[\w]*))?)/)lol", std::regex::extended};
    static const std::regex targetRegex{R"(^(([^:\/?#]+):)?(//([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)", std::regex::extended};
    std::smatch match;
    unsigned int counter{0};
    return (std::regex_match(copyString, match, targetRegex));
}

std::string getPrettyLineEndings(const std::string &lineEnding)
{
    using namespace GeneralUtilities;
    if (lineEnding == "\n") {
        return "\\n (Line Feed)";
    } else if (lineEnding == "\r") {
        return "\\r (Carriage Return)";
    } else if ((lineEnding == "\r\n") || (lineEnding == "\n\r")) {
        return "\\r\\n (Carriage Return & Line Feed)";
    } else if (lineEnding == "") {
        return "None";
    } else {
        throw std::runtime_error("Invalid lineEnding passed to getPrettyLineEndings(const std::string &): " + tQuoted(lineEnding));
    }
}
