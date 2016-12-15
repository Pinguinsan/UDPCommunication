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
#include <unistd.h>
#include <signal.h>

#include <generalutilities.h>
#include <systemcommand.h>
#include <udpduplex.h>
#include <datetime.h>
#include <prettyprinter.h>

#include "scriptreader.h"
#include "scriptexecutor.h"
#include "udpcommunicationstrings.h"

using namespace DateTime;
using namespace GeneralUtilities;
using namespace UDPCommunicationStrings;

static const char *PROGRAM_NAME{"udpcomm"};
static const char *LONG_PROGRAM_NAME{"UDP Communication"};
static const char *AUTHOR_NAME{"Tyler Lewis"};
static const int SOFTWARE_MAJOR_VERSION{0};
static const int SOFTWARE_MINOR_VERSION{1};
static const int SOFTWARE_PATCH_VERSION{0};

#ifdef __GNUC__
    const int GCC_MAJOR_VERSION{__GNUC__};
    const int GCC_MINOR_VERSION{__GNUC_MINOR__};
    const int GCC_PATCH_VERSION{__GNUC_PATCHLEVEL__};
#else
    #error "The compiler must define __GNUC__ to use this program, but the compiler does not have it defined"
#endif

static std::list<const char *> CLIENT_PORT_NUMBER_SWITCHES{"-p", "--p", "-client-port-number", "--client-port-number"};
static std::list<const char *> CLIENT_HOST_NAME_SWITCHES{"-n", "--n", "-host", "--host", "-client-host-name", "--client-host-name"};
static std::list<const char *> SERVER_PORT_NUMBER_SWITCHES{"-d", "--d", "-host-port-number", "--host-port-number"};

static std::list<const char *> SEND_ONLY_SWITCHES{"-send", "--send", "-send-only", "--send-only"};
static std::list<const char *> MAXIMUM_READ_SIZE_SWITCHES{"-m", "--m", "-max", "--max", "-max-read", "--max-read", "-max-size", "--max-size", "-max-read-size", "--max-read-size"};
static std::list<const char *> LINE_ENDING_SWITCHES{"-e", "--e", "-line-ending", "--line-ending", "-line-endings", "--line-endings"};
static std::list<const char *> READ_UNTIL_SWITCHES{"-r", "--r", "-read-until", "--read-until"};
static std::list<const char *> RECEIVE_ONLY_SWITCHES{"-receive", "--receive", "-receive-only", "--receive-only"};
static std::list<const char *> SYNCHRONOUS_COMMUNICATION_SWITCHES{"-sync", "--sync", "-sync-comm", "--sync-comm"};
static std::list<const char *> SCRIPT_FILE_SWITCHES{"-c", "--c", "-script", "--script", "-script-file", "--script-file", "-script-name", "--script-name"};
static std::list<const char *> VERSION_SWITCHES{"-v", "--v", "-version", "--version"};
static std::list<const char *> HELP_SWITCHES{"-h", "--h", "-help", "--help"};


static std::unique_ptr<PrettyPrinter> prettyPrinter{std::make_unique<PrettyPrinter>()};

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
std::string doUDPReadString();

void printRxResult(const std::string &str);
void printTxResult(const std::string &str);
void printDelayResult(DelayType delayType, int howLong);
void printFlushResult(FlushType flushType);
void printLoopResult(LoopType loopType, int currentLoop, int loopCount);

static unsigned int currentCommandHistoryIndex{0};
static std::list<std::string> commandHistory;

static std::set<std::string> scriptFiles;
static std::map<std::string, std::unique_ptr<ScriptExecutor>> scriptFileMap;

void backspaceTerminal(unsigned int howFar);
void displayHelp();
void displayVersion();
void doAtExit();
void interruptHandler(int signalNumber);

static std::mutex ioMutex;

std::string asyncStdinTask();
void startAsyncStdinTask(const std::function<std::string(void)> &func);
static std::unique_ptr<std::shared_future<std::string>> asyncStdinTaskFuture;
static std::function<std::string(void)> packagedAsyncStdinTask{asyncStdinTask};

std::string asyncStdoutTask();
void startAsyncStdoutTask(const std::function<std::string(void)> &func);
static std::unique_ptr<std::shared_future<std::string>> asyncStdoutTaskFuture;
static std::function<std::string(void)> packagedAsyncStdoutTask{asyncStdoutTask};

static std::function<void(const std::string&)> packagedRxResultTask{printRxResult};
static std::function<void(const std::string&)> packagedTxResultTask{printTxResult};
static std::function<void(DelayType, int)> packagedDelayResultTask{printDelayResult};
static std::function<void(FlushType)> packagedFlushResultTask{printFlushResult};
static std::function<void(LoopType, int, int)> packagedLoopResultTask{printLoopResult};
static std::shared_ptr<UDPDuplex> udpDuplex;

static std::string clientHostName{UDPDuplex::s_DEFAULT_CLIENT_HOST_NAME};
static std::string clientPortNumber{std::to_string(UDPDuplex::s_DEFAULT_CLIENT_PORT_NUMBER)};
static std::string serverPortNumber{std::to_string(UDPDuplex::s_DEFAULT_SERVER_PORT_NUMBER)};

bool sendOnly{false};
bool receiveOnly{false};
bool synchronousCommunication{false};
std::string previousStringSent{""};
std::string readUntil{""};
int maximumReadSize{-1};
std::string getPrettyLineEndings(const std::string &lineEnding);
LineEnding lineEndings;

const uint16_t MAXIMUM_PORT_NUMBER{std::numeric_limits<uint16_t>::max()};


int main(int argc, char *argv[])
{
    //Signal handling stuff
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = interruptHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    //End signal handling
    
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
    std::string clientHostName{""};

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
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a positive number (" << maybePort << " < 0), skipping option" << std::endl;
                    } else if (maybePort > MAXIMUM_PORT_NUMBER) {
                        std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not greater than maximum port number (" << maybePort << " > " << MAXIMUM_PORT_NUMBER << "), skipping option" << std::endl;
                    } else {
                        clientPortNumber = static_cast<std::string>(argv[i]);
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
                        clientPortNumber = static_cast<std::string>(argv[i]);
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
                        serverPortNumber = static_cast<std::string>(argv[i]);
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
                        serverPortNumber = static_cast<std::string>(argv[i]);
                    }
                } catch (std::exception &e) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but specified client port number " << tQuoted(maybePortString) << " is not a number between 0 and " << MAXIMUM_PORT_NUMBER << ", skipping option" << std::endl;
                }
            }
        } else if (isSwitch(argv[i], LINE_ENDING_SWITCHES)) {
            if (UDPDuplex::lineEndingToString(lineEndings) != "") {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but lineEndings have already been set by another option (" << getPrettyLineEndings(UDPDuplex::lineEndingToString(lineEndings)) << "), skipping option" << std::endl;
            } else if (argv[i+1]) {
                try {
                    lineEndings = UDPDuplex::parseLineEndingFromRaw(static_cast<std::string>(argv[i+1]));
                } catch (std::exception &e) {
                    (void)e;
                    std::cout << "WARNING: Switch " << argv[i] << " accepted, but " << std::quoted(argv[i+1]) << " is an invalid line ending, skipping option";
                }
                i++;
            } else {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no line endings were specified after, skipping option" << std::endl;
            }
        } else if (isEqualsSwitch(argv[i], LINE_ENDING_SWITCHES)) {
            if (UDPDuplex::lineEndingToString(lineEndings) != "") {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but lineEndings have already been set by another option (" << getPrettyLineEndings(UDPDuplex::lineEndingToString(lineEndings)) << "), skipping option" << std::endl;
            } else {
                std::string copyString{static_cast<std::string>(argv[i])};
                size_t foundPosition{copyString.find("=")};
                size_t foundEnd{copyString.substr(foundPosition).find(" ")};
                if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                    std::cout << "WARNING: Switch " << argv[i] << " accepted, but no line endings were specified after, skipping option" << std::endl;
                } else {
                    try {
                        lineEndings = UDPDuplex::parseLineEndingFromRaw(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\""));
                    } catch (std::exception &e) {
                        std::cout << "WARNING: Switch " << argv[i] << " accepted, but " << std::quoted(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")) << " is an invalid line ending option, skipping option" << std::endl;
                    }
                }   
            }
        } else if (isSwitch(argv[i], MAXIMUM_READ_SIZE_SWITCHES)) {
             if (maximumReadSize != -1) {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but maximumReadSize have already been set by another option (" << maximumReadSize << "), skipping option" << std::endl;
            } else if (argv[i+1]) {
                int tempSize{0};
                try {
                    tempSize = std::stoi(static_cast<std::string>(argv[i+1]));
                } catch (std::exception &e) {
                    std::cout << "WARNING: Switch " << argv[i] << " accepted, but " << std::quoted(argv[i+1]) << " is not a number, skipping option" << std::endl;
                }
                if (tempSize <= 0) {
                    std::cout << "WARNING: Switch " << argv[i] << " accepted, but " << std::quoted(argv[i+1]) << " is not a positive number, skipping option" << std::endl;
                } else {
                    maximumReadSize = tempSize;
                }
                i++;
            } else {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no size was specified after, skipping option" << std::endl;
            }
        } else if (isEqualsSwitch(argv[i], MAXIMUM_READ_SIZE_SWITCHES)) {
            if (maximumReadSize != -1) {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but maximumReadSize have already been set by another option (" << maximumReadSize << "), skipping option" << std::endl;
            } else {
                std::string copyString{static_cast<std::string>(argv[i])};
                size_t foundPosition{copyString.find("=")};
                size_t foundEnd{copyString.substr(foundPosition).find(" ")};
                if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                    std::cout << "WARNING: Switch " << argv[i] << " accepted, but no size were specified after, skipping option" << std::endl;
                } else {
                    int tempSize{0};
                    try {
                        tempSize = std::stoi(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\""));
                    } catch (std::exception &e) {
                        std::cout << "WARNING: Switch " << argv[i] << " accepted, but " << std::quoted(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")) << " is not a number, skipping option" << std::endl;
                    }
                    if (tempSize <= 0) {
                        std::cout << "WARNING: Switch " << argv[i] << " accepted, but " << std::quoted(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")) << " is not a positive number, skipping option" << std::endl;
                    } else {
                        maximumReadSize = tempSize;
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
        } else if (isSwitch(argv[i], READ_UNTIL_SWITCHES)) {
            if (argv[i+1]) {
                if (readUntil != "") {
                    std::cout << "WARNING: Switch " << argv[i] << " accepted, but --read-until switch has already been set to " << readUntil << ", skipping option" << std::endl; 
                } else {
                    readUntil = static_cast<std::string>(argv[i+1]);
                } 
            } else {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no read-until string was specified after, skipping option" << std::endl;
            }
            i++;
        } else if (isEqualsSwitch(argv[i], READ_UNTIL_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but no read-until string was specified after, skipping option" << std::endl;
            } else if (readUntil != "") {
                std::cout << "WARNING: Switch " << argv[i] << " accepted, but --read-until switch has already been set to " << readUntil << ", skipping option" << std::endl; 
            } else {
                readUntil = stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"");
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
        }  else {
            std::cout << "WARNING: Switch " << argv[i] << " is an invalid option, skipping" << std::endl;
        }
    }

    prettyPrinter->setForegroundColor(LIST_COLOR);
    prettyPrinter->setFontAttributes(COMMON_FONT_ATTRIBUTE);
    std::cout << "Using ClientHostName=";
    prettyPrinter->println(clientHostName);
    
    std::cout << "Using ClientPortNumber=";
    prettyPrinter->println(clientPortNumber);
    
    std::cout << "Using ServerPortNumber=";
    prettyPrinter->println(serverPortNumber);
    
    std::cout << "Using LineEndings=";
    prettyPrinter->println(getPrettyLineEndings(UDPDuplex::lineEndingToString(lineEndings)));

    if (readUntil != "") {
        std::cout << "Using ReadUntilString=";
        prettyPrinter->println(readUntil);
    }

    if (maximumReadSize != -1) {
        std::cout << "Using MaximumReadSize=";
        prettyPrinter->println(maximumReadSize);
    }

    int i{1};
    for (auto &it : scriptFiles) {
        std::cout << "Using ScriptFile=" << it << " (" << i++ << "/" << scriptFiles.size() << ")" << std::endl;
    }
    std::cout << std::endl;
    try {
        udpDuplex = std::make_shared<UDPDuplex>(clientHostName,
                                                std::stoi(clientPortNumber), 
                                                std::stoi(clientPortNumber));
        try {
            udpDuplex->setLineEnding(lineEndings);
            if (maximumReadSize > 0) {
                //udpDuplex->setMaximumReadSize(maximumReadSize);
            }
            udpDuplex->openPort();
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            return 1;
        }
        delayMilliseconds(500);
        udpDuplex->setTimeout(25);        
        long long int TIMEOUT_TIME{400};
        std::cout << "Successfully opened serial port ";
        prettyPrinter->println(udpDuplex->portName() + "\n");
        for (auto &it : scriptFiles) {
            scriptFileMap.emplace(it, std::make_unique<ScriptExecutor>(it));
        }
        i = 1;
        for (auto &it : scriptFileMap) {
            if (it.second->scriptReader()->commands()->empty()) {
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
        udpDuplex->flush();
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
                if (stripNonAsciiCharacters(stringToSend).find("[A") == 0) {
                    stringToSend = previousStringSent;
                } else {
                    stringToSend = stripNonAsciiCharacters(stringToSend);
                }
                //Strip stupid [B and [C control characters from Cygwin shell
                for (char i = 'B'; i < 'Z'; i++) {
                    stringToSend = stripAllFromString(stringToSend, (std::string{1, '['} + std::string{1, static_cast<char>(i)}));
                }
                sendUDPString(stringToSend);
            }
        } else if (receiveOnly) {
            std::cout << "Beginning ";
            prettyPrinter->print("receive-only");
            std::cout << " communication loop, enter desired string and press enter to send strings, or press CTRL+C to quit" << std::endl << std::endl;
            while (true) {
                returnString = doUDPReadString();
                if (returnString != "") {
                    printRxResult(returnString);
                }
            }   
        } else if (synchronousCommunication) {
            std::cout << "Beginning ";
            prettyPrinter->print("synchronous");
            std::cout << " communication loop, enter desired string and press enter to send strings, or press CTRL+C to quit" << std::endl << std::endl;
            while (true) {
                std::getline(std::cin, stringToSend);
                if (stripNonAsciiCharacters(stringToSend).find("[A") == 0) {
                    stringToSend = previousStringSent;
                } else {
                    stringToSend = stripNonAsciiCharacters(stringToSend);
                }
                //Strip stupid [B and [C control characters from Cygwin shell
                for (char i = 'B'; i < 'Z'; i++) {
                    stringToSend = stripAllFromString(stringToSend, (std::string{1, '['} + std::string{1, static_cast<char>(i)}));
                }
                if ((stringToSend != "") && (!isWhitespace(stringToSend))) {
                    sendUDPString(stringToSend);
                }
                returnString = doUDPReadString();
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
                    if (temp.find("[A") == 0) {
                        stringToSend = previousStringSent;
                    } else {
                        stringToSend = temp;
                    }
                    //Strip stupid [B and [C control characters from Cygwin shell
                    for (char i = 'B'; i < 'Z'; i++) {
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

void displayHelp() 
{
    std::cout << "Usage: " << PROGRAM_NAME << " [options][=][argument]" << std::endl << std::endl;
    std::cout << "Options: " << std::endl;
    std::cout << "    -n, --name, -port-name, --port-name: Specify serial port name" << std::endl;
    std::cout << "    -b, --b, -baud-rate, --baud-rate: Specify baud rate" << std::endl;
    std::cout << "    -p, --p, -parity, --parity: Specify parity" << std::endl;
    std::cout << "    -d, --d, -data-bits, --data-bits: Specify data bits" << std::endl;
    std::cout << "    -s, --s, -stop-bits, --stop-bits: Specify stop bits" << std::endl;
    std::cout << "    -c, --c, -script-file, --script-file: Specify script file to be run after serial port is opened" << std::endl;
    std::cout << "    -e, --e, -line-ending, --line-ending: Specify what type of line ending should be used" << std::endl;
    std::cout << "    -h, --h, -help, --help: Show this help text" << std::endl;
    std::cout << "    -v, --v, -version, --version: Display version" << std::endl;
    std::cout << "Example: " << std::endl;
    std::cout << "    Command line input: scomm --baud-rate=9600 --port-name=/dev/ttyUSB0 --line-ending=r" << std::endl;
    std::cout << "    Output:" << std::endl;
    std::cout << "        Using /dev/ttyUSB0 for serial port" << std::endl;
    std::cout << "        Using BaudRate=9600" << std::endl;
    std::cout << "        Using Parity=None" << std::endl;
    std::cout << "        Using StopBits=1" << std::endl;
    std::cout << "        Using DataBits=8" << std::endl;
    std::cout << "        Using LineEnding=\\r (Carriage Return)" << std::endl;
    std::cout << "        Successfully opened serial port /dev/ttyUSB0" << std::endl;
    std::cout << "        Received 10 bytes: 0123456789" << std::endl;
}

void displayVersion() 
{
    std::cout << PROGRAM_NAME << "(" << LONG_PROGRAM_NAME << "), v" << SOFTWARE_MAJOR_VERSION << "." << SOFTWARE_MINOR_VERSION << "." << SOFTWARE_PATCH_VERSION << std::endl;
    std::cout << "Written by " << AUTHOR_NAME << ", " << currentYear() << std::endl;
    std::cout << "Built with g++ v" << GCC_MAJOR_VERSION << "." << GCC_MINOR_VERSION << "." << GCC_PATCH_VERSION << ", " << dateStampMDY() << std::endl << std::endl;
}

void doAtExit()
{
    return interruptHandler(0);
}

void interruptHandler(int signalNumber) 
{
    std::cout << std::endl << "Exiting " << PROGRAM_NAME << std::endl;
    if (udpDuplex) {
        udpDuplex->closePort();
    }
    _Exit(signalNumber);
}

void startAsyncStdinTask(const std::function<std::string(void)> &func)
{
    asyncStdinTaskFuture = std::make_unique<std::shared_future<std::string>>(std::async(std::launch::async, func));
}

void startAsyncStdoutTask(const std::function<std::string(void)> &func)
{
    asyncStdoutTaskFuture = std::make_unique<std::shared_future<std::string>>(std::async(std::launch::async, func));
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
    if (readUntil == "") {
        std::string returnString{""};
        do {
            returnString += udpDuplex->readString();
        } while ((returnString.length() == 0) || (isWhitespace(returnString)));
        return returnString;
    } else {
        std::string returnString{""};
        do {
            returnString += udpDuplex->readString();
        } while ((returnString.length() == 0) || (isWhitespace(returnString)) && !GeneralUtilities::endsWith(returnString, readUntil));
        return returnString;
    }
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

std::string doUDPReadString()
{
    if (readUntil == "") {
        return udpDuplex->readString();
    } else {
        return udpDuplex->readStringUntil(readUntil);
    }
}

void sendUDPString(const std::string &str)
{
    udpDuplex->writeString(str);
    udpDuplex->flush();
    if ((str != "") && (!isWhitespace(str))) {
        previousStringSent = str;
    }
    std::cout << "\033[1A\r"; // Goes back up a line and clears the line
    printTxResult(str);
}

void printRxResult(const std::string &str)
{
    using namespace GeneralUtilities;
    std::unique_lock<std::mutex> ioLock{ioMutex};
    if (str == readUntil) {
        return;
    }
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