/***********************************************************************
*    ibytestream.h:                                                    *
*    IByteStream, base class for simple read and write operations      *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.serial/tlewiscpp/tjlutils                          *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the declarations of an base class IByteStream     *
*    It includes very basic IO operations like read and write          *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#ifndef TJLUTILS_IBYTESTREAM_H
#define TJLUTILS_IBYTESTREAM_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <functional>
#include <tuple>
#include <cstdlib>
#include <utility>

#if defined(_WIN32) && !defined(__CYGWIN__)
    //using ssize_t = long;
#endif

enum class IByteStreamCommandType { DELAY_SECONDS, DELAY_MILLISECONDS, DELAY_MICROSECONDS, WRITE, READ, FLUSH_RX, FLUSH_TX, FLUSH_RX_TX, LOOP_START, LOOP_END, COMMAND_UNSPECIFIED };
enum class DelayType { SECONDS, MILLISECONDS, MICROSECONDS };
enum class FlushType { RX, TX, RX_TX };
enum class LoopType { START, END };

class IByteStreamCommand
{
public:
    IByteStreamCommand(IByteStreamCommandType commandType, const std::string &commandArgument) :
        m_commandType{commandType},
        m_commandArgument{commandArgument} { }
    
    IByteStreamCommandType commandType() const { return this->m_commandType; }
    std::string commandArgument() const { return this->m_commandArgument; }
    void setCommandType(const IByteStreamCommandType &commandType) { this->m_commandType = commandType; }
    void setCommandArgument(const std::string &commandArgument) { this->m_commandArgument = commandArgument; }

private:
    IByteStreamCommandType m_commandType;
    std::string m_commandArgument;
};

class IByteStream
{
public:
    virtual ~IByteStream() { }

    virtual void setTimeout(long timeout) = 0;
    virtual long timeout() const = 0;
    virtual std::string lineEnding() const = 0;
    virtual void setLineEnding(const std::string &str) = 0;

    virtual ssize_t writeLine(const std::string &str) = 0;
    virtual ssize_t writeLine(const char *str) = 0;
    virtual ssize_t available() = 0;
    virtual bool isOpen() const = 0;
    virtual void openPort() = 0;
    virtual void closePort() = 0;

    virtual std::string portName() const = 0;
    virtual void flushRX() = 0;
    virtual void flushTX() = 0;
    virtual void flushRXTX() = 0;

    virtual std::string peek() = 0;
    virtual char peekByte() = 0;

    virtual void putBack(const std::string &str) = 0;
    virtual void putBack(const char *str) = 0;
    virtual void putBack(char back) = 0;

    virtual std::string readLine() = 0;
    virtual std::string readUntil(const std::string &until) = 0;
    virtual std::string readUntil(const char *until) = 0;
    virtual std::string readUntil(char until) = 0;

};

const char * const DELAY_IDENTIFIER{"delay"};
const char * const DELAY_SECONDS_IDENTIFIER{"delayseconds("};
const char * const DELAY_MILLISECONDS_IDENTIFIER{"delaymilliseconds("};
const char * const DELAY_MICROSECONDS_IDENTIFIER{"delaymicroseconds("};
const char * const WRITE_IDENTIFIER{"write("};
const char * const READ_IDENTIFIER{"read("};
const char * const LOOP_IDENTIFIER{"loop("};
const char * const FLUSH_IDENTIFIER{"flush"};
const char * const NO_CLOSING_PARENTHESIS_FOUND_STRING{"    No matching parenthesis found, ignoring option"};
const char * const NO_CLOSING_QUOTATION_MARKS_FOUND_STRING{"    No matching quotation marks found, ingoring option"};
const char * const NO_PARAMETER_SEPARATING_COMMA_STRING{"    No parameter separating comma found, ignoring option"};
const char * const NO_LOOP_COUNT_SPECIFIED_STRING{"    Loop() parameter must include an integer parameter (the number of loops to execute), ignoring option"};
const char * const EXPECTED_HERE_STRING{"^---expected here"};
const char * const HERE_STRING{"^---here"};
const char * const WRITE_PARAMETER_MUST_BE_IN_QUOTATIONS_STRING{"    Write() parameter must be enclosed in parentheses, ignoring option"};
const char * const DELAY_SECONDS_PARAMETER_NOT_AN_INTEGER_STRING{"    DelaySeconds() parameter is not an integer, ignoring option"};
const char * const DELAY_MILLISECONDS_PARAMETER_NOT_AN_INTEGER_STRING{"    DelayMilliseconds() parameter is not an integer, ignoring option"};
const char * const DELAY_MICROSECONDS_PARAMETER_NOT_AN_INTEGER_STRING{"    DelayMicroseconds() parameter is not an integer, ignoring option"};
const char * const GENERIC_CONFIG_WARNING_BASE_STRING{"WARNING: line "};
const char * const GENERIC_CONFIG_WARNING_TAIL_STRING{" of configuration file:"};
const char * const CONFIG_EXPRESSION_MALFORMED_STRING{"    expression is malformed/has invalid syntax, ignoring option"};
const char * const EXCEPTION_IN_CONSTRUCTOR_STRING{"WARNING: Standard exception caught in IByteStreamScriptReader constructor: "};
const char * const COMMAND_TYPE_NOT_IMPLEMENTED_STRING{"WARNING: Command type not implemented, skipping command: "};
const char * const NULL_IO_STREAM_PASSED_TO_EXECUTE_STRING{"WARNING: Null IByteStream passed to IByteStreamScriptExecutor::execute(std::shared_ptr<IByteStream>), skipping script execution"};
const char * const SCRIPT_FILE_DOES_NOT_EXISTS_STRING{"WARNING: Script file does not exist, skipping script: "};
const char * const UNABLE_TO_OPEN_SCRIPT_FILE_STRING{"WARNING: Unable to open script file, skipping script: "};
const char * const LOOP_COUNT_PARAMETER_NOT_AN_INTEGER_STRING{"LoopCount() parameter is not an integer, ignoring option"};
const char * const UNTERMINATED_LOOP_STRING{"WARNING: The script contains an unterminated loop,  skipping script execution"};
const char * const UNEXPECTED_LOOP_CLOSING_STRING{"WARNING: A loop closure was found, but no loop was currently being populated, ignoring option"};
const char * const CLOSING_LOOP_IDENTIFIER{"}"};


class IByteStreamScriptReader
{
public:
    IByteStreamScriptReader(const std::string &scriptFilePath);
    std::string scriptFilePath() const;
    std::shared_ptr<std::vector<IByteStreamCommand>> commands() const;
private:
    std::string m_scriptFilePath;
    std::shared_ptr<std::vector<IByteStreamCommand>> m_commands;
    bool fileExists(const std::string &fileToCheck);
    bool fileExists(const char *fileToCheck);
    template <typename T> static inline std::string toStdString(const T &t) { 
        return dynamic_cast<std::stringstream &>(std::stringstream{} << t).str(); 
    }
    template <typename T> static inline std::string tQuoted(const T &t) {
        return "\"" + toStdString(t) + "\"";
    }
};

class IByteStreamScriptExecutor
{
private:
    static inline void delayMilliseconds(unsigned long long howLong) { std::this_thread::sleep_for(std::chrono::milliseconds(howLong)); }
    static inline void delaySeconds(unsigned long long howLong) { std::this_thread::sleep_for(std::chrono::seconds(howLong)); }
    template <typename T> static inline std::string toStdString(const T &t) { 
        return dynamic_cast<std::stringstream &>(std::stringstream{} << t).str(); 
    }
    template <typename T> static inline std::string tQuoted(const T &t) {
        return "\"" + toStdString(t) + "\"";
    }
public:
    IByteStreamScriptExecutor(const std::string &scriptFilePath);
    void setScriptFilePath(const std::string &scriptFilePath);
    std::string scriptFilePath() const;
    bool hasCommands() const;
    size_t numberOfCommands() const;
    
    template <typename ... RxArgs, typename ... TxArgs, typename ... DelayArgs, typename ... FlushArgs, typename ... LoopArgs>
    void execute(std::shared_ptr<IByteStream> ioStream, 
                 const std::function<void(RxArgs...)> &printRxResult, 
                 const std::function<void(TxArgs...)> &printTxResult,
                 const std::function<void(DelayArgs...)> &printDelayResult,
                 const std::function<void(FlushArgs...)> &printFlushResult,
                 const std::function<void(LoopArgs...)> &printLoopResult)
    {
        if (!ioStream) {
            throw std::runtime_error(NULL_IO_STREAM_PASSED_TO_EXECUTE_STRING);
        }
        if (!ioStream->isOpen()) {
            try {
                ioStream->openPort();
            } catch (std::exception &e) {
                throw std::runtime_error(e.what());
            }
        }
        int loop {false};
        int loopCount{0};
        this->m_iByteStreamScriptCommands = doUnrollLoopCommands(*this->m_iByteStreamScriptReader->commands());
        for (auto &it : this->m_iByteStreamScriptCommands) {
            try {
                if (it.commandType() == IByteStreamCommandType::WRITE) {
                    ioStream->writeLine(it.commandArgument());
                    printTxResult(it.commandArgument());
                } else if (it.commandType() == IByteStreamCommandType::READ) {
                    printRxResult(ioStream->readLine());
                } else if (it.commandType() == IByteStreamCommandType::DELAY_SECONDS) {
                    printDelayResult(DelayType::SECONDS, std::stoi(it.commandArgument()));
                    delaySeconds(std::stoi(it.commandArgument()));
                } else if (it.commandType() == IByteStreamCommandType::DELAY_MILLISECONDS) {
                    printDelayResult(DelayType::MILLISECONDS, std::stoi(it.commandArgument()));
                    delayMilliseconds(std::stoi(it.commandArgument()));
                } else if (it.commandType() == IByteStreamCommandType::DELAY_MICROSECONDS) {
                    printDelayResult(DelayType::MICROSECONDS, std::stoi(it.commandArgument()));
                    delayMilliseconds(std::stoi(it.commandArgument()));
                } else if (it.commandType() == IByteStreamCommandType::FLUSH_RX) {
                    printFlushResult(FlushType::RX);
                    ioStream->flushRX();
                } else if (it.commandType() == IByteStreamCommandType::FLUSH_TX) {
                    printFlushResult(FlushType::TX);
                    ioStream->flushTX();
                } else if (it.commandType() == IByteStreamCommandType::FLUSH_RX_TX) {
                    printFlushResult(FlushType::RX_TX);
                    ioStream->flushRXTX();
                } else {
                    throw std::runtime_error(COMMAND_TYPE_NOT_IMPLEMENTED_STRING + it.commandArgument());
                }
            } catch (std::exception &e) {
                throw std::runtime_error(e.what());
            }
        }
    }


    template <typename InstanceArg, typename ... RxArgs, typename ... TxArgs, typename ... DelayArgs, typename ... FlushArgs, typename ... LoopArgs>
    void execute(InstanceArg *instanceArg,
                 std::shared_ptr<IByteStream> ioStream, 
                 const std::function<void(InstanceArg *, RxArgs...)> &printRxResult, 
                 const std::function<void(InstanceArg *, TxArgs...)> &printTxResult,
                 const std::function<void(InstanceArg *, DelayArgs...)> &printDelayResult,
                 const std::function<void(InstanceArg *, FlushArgs...)> &printFlushResult,
                 const std::function<void(InstanceArg *, LoopArgs...)> &printLoopResult)
    {
        (void)printLoopResult;
        if (!ioStream) {
            throw std::runtime_error(NULL_IO_STREAM_PASSED_TO_EXECUTE_STRING);
        }
        if (!ioStream->isOpen()) {
            try {
                ioStream->openPort();
            } catch (std::exception &e) {
                throw std::runtime_error(e.what());
            }
        }
        int loop {false};
        int loopCount{0};
        this->m_iByteStreamScriptCommands = doUnrollLoopCommands(*this->m_iByteStreamScriptReader->commands());
        (void)loop;
        (void)loopCount;
        for (auto &it : this->m_iByteStreamScriptCommands) {
            try {
                if (it.commandType() == IByteStreamCommandType::WRITE) {
                    ioStream->writeLine(it.commandArgument());
                    printTxResult(instanceArg, it.commandArgument());
                } else if (it.commandType() == IByteStreamCommandType::READ) {
                    printRxResult(instanceArg, ioStream->readLine());
                } else if (it.commandType() == IByteStreamCommandType::DELAY_SECONDS) {
                    printDelayResult(instanceArg, DelayType::SECONDS, std::stoi(it.commandArgument()));
                    delaySeconds(std::stoi(it.commandArgument()));
                } else if (it.commandType() == IByteStreamCommandType::DELAY_MILLISECONDS) {
                    printDelayResult(instanceArg, DelayType::MILLISECONDS, std::stoi(it.commandArgument()));
                    delayMilliseconds(std::stoi(it.commandArgument()));
                } else if (it.commandType() == IByteStreamCommandType::DELAY_MICROSECONDS) {
                    printDelayResult(instanceArg, DelayType::MICROSECONDS, std::stoi(it.commandArgument()));
                    delayMilliseconds(std::stoi(it.commandArgument()));
                } else if (it.commandType() == IByteStreamCommandType::FLUSH_RX) {
                    printFlushResult(instanceArg, FlushType::RX);
                    ioStream->flushRX();
                } else if (it.commandType() == IByteStreamCommandType::FLUSH_TX) {
                    printFlushResult(instanceArg, FlushType::TX);
                    ioStream->flushTX();
                } else if (it.commandType() == IByteStreamCommandType::FLUSH_RX_TX) {
                    printFlushResult(instanceArg, FlushType::RX_TX);
                    ioStream->flushRXTX();
                } else {
                    throw std::runtime_error(COMMAND_TYPE_NOT_IMPLEMENTED_STRING + it.commandArgument());
                }
            } catch (std::exception &e) {
                throw std::runtime_error(e.what());
            }
        }
    }
private:
    std::shared_ptr<IByteStreamScriptReader> m_iByteStreamScriptReader;
    std::vector<IByteStreamCommand> m_iByteStreamScriptCommands;

    std::vector<IByteStreamCommand> doUnrollLoopCommands(const std::vector<IByteStreamCommand> &iByteStreamCommands);
    bool containsLoopStart(const std::vector<IByteStreamCommand> &commands);
    std::pair<int, int> findInnerLoopIndexes(const std::vector<IByteStreamCommand> &iByteStreamCommands);
};


std::vector<std::string> trimWhitespaceFromBeginning (const std::vector<std::string> &vec);
std::vector<std::string> trimWhitespaceFromEnd (const std::vector<std::string> &vec);
std::vector<std::string> trimWhitespace (const std::vector<std::string> &vec);
std::string trimWhitespace(const std::string &str);
std::string trimWhitespaceFromBeginning(const std::string &str);
std::string trimWhitespaceFromEnd(const std::string &str);
bool isWhitespace(const std::string &stringToCheck);
bool isWhitespace(char charToCheck);
std::string getBetween(const std::string &beginning, const std::string &ending, const std::string &findString);
std::vector<std::string> getAllBetween(const std::string &beginning, const std::string &ending, const std::string &findString);
std::string stripBetween(const std::string &beginning, const std::string &ending, const std::string &findString);
std::string stripAllBetween(const std::string &beginning, const std::string &ending, const std::string &findString);
std::string stripFromString(const std::string &stringToStrip, const std::string &whatToStrip);
std::string stripFromString(const std::string &stringToStrip, char whatToStrip);
std::string stripAllFromString(const std::string &stringToStrip, const std::string &whatToStrip);
std::string stripAllFromString(const std::string &stringToStrip, char whatToStrip);
std::string tWhitespace(int howMuch);

#endif //TJLUTILS_IBYTESTREAM_H
