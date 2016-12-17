#include "scriptexecutor.h"

ScriptExecutor::ScriptExecutor(const std::string &scriptFilePath) :
    m_scriptReader{std::make_shared<ScriptReader>(scriptFilePath)}
{

}

void ScriptExecutor::setScriptFilePath(const std::string &scriptFilePath)
{
    this->m_scriptReader.reset();
    this->m_scriptReader = std::make_shared<ScriptReader>(scriptFilePath);
}

std::shared_ptr<ScriptReader> ScriptExecutor::scriptReader() const
{
    return this->m_scriptReader;
}

void ScriptExecutor::execute(std::shared_ptr<UDPDuplex> udpDuplex, 
                             const std::function<void(const std::string &)> &printRxResult, 
                             const std::function<void(const std::string &)> &printTxResult,
                             const std::function<void(DelayType, int)> &printDelayResult,
                             const std::function<void(FlushType)> &printFlushResult,
                             const std::function<void(LoopType, int, int)> &printLoopResult)
{
    using namespace UDPCommunicationStrings;
    using namespace GeneralUtilities;
    if (!udpDuplex) {
        throw std::runtime_error(NULL_UDP_DUPLEX_PASSED_TO_EXECUTE_STRING);
    }
    if (!udpDuplex->isOpen()) {
        try {
            udpDuplex->openPort();
        } catch (std::exception &e) {
            throw std::runtime_error(e.what());
        }
    }
    int loop {false};
    int loopCount{0};
    std::vector<UDPCommand> loopUDPCommands;
    this->m_scriptCommands = doUnrollLoopCommands(*this->m_scriptReader->commands());
    for (auto &it : this->m_scriptCommands) {
        try {
            if (it.commandType() == UDPCommandType::WRITE) {
                udpDuplex->writeString(it.commandArgument());
                printTxResult(it.commandArgument());
            } else if (it.commandType() == UDPCommandType::READ) {
                printRxResult(udpDuplex->readString());
            } else if (it.commandType() == UDPCommandType::DELAY_SECONDS) {
                printDelayResult(DelayType::SECONDS, std::stoi(it.commandArgument()));
                delaySeconds(std::stoi(it.commandArgument()));
            } else if (it.commandType() == UDPCommandType::DELAY_MILLISECONDS) {
                printDelayResult(DelayType::MILLISECONDS, std::stoi(it.commandArgument()));
                delayMilliseconds(std::stoi(it.commandArgument()));
            } else if (it.commandType() == UDPCommandType::DELAY_MICROSECONDS) {
                printDelayResult(DelayType::MICROSECONDS, std::stoi(it.commandArgument()));
                delayMilliseconds(std::stoi(it.commandArgument()));
            } else if (it.commandType() == UDPCommandType::FLUSH_RX) {
                printFlushResult(FlushType::RX);
                udpDuplex->flushRX();
            } else if (it.commandType() == UDPCommandType::FLUSH_TX) {
                printFlushResult(FlushType::TX);
                udpDuplex->flushTX();
            } else if (it.commandType() == UDPCommandType::FLUSH_RX_TX) {
                printFlushResult(FlushType::RX_TX);
                udpDuplex->flushRXTX();
            } else {
                throw std::runtime_error(UDP_COMMAND_TYPE_NOT_IMPLEMENTED_STRING + it.commandArgument());
            }
        } catch (std::exception &e) {
            throw std::runtime_error(e.what());
        }
    }
}

std::vector<UDPCommand> ScriptExecutor::doUnrollLoopCommands(const std::vector<UDPCommand> &udpCommands)
{
    using namespace UDPCommunicationStrings;
    std::vector<UDPCommand> returnCommands;
    for (auto iter = udpCommands.begin(); iter != udpCommands.end(); iter++) {
        try {
            std::cout << "iter = " << iter->commandArgument() << std::endl;
            if (iter->commandType() == UDPCommandType::WRITE) {
                returnCommands.push_back(*iter);
            } else if (iter->commandType() == UDPCommandType::READ) {
                returnCommands.push_back(*iter);
            } else if (iter->commandType() == UDPCommandType::DELAY_SECONDS) {
                returnCommands.push_back(*iter);
            } else if (iter->commandType() == UDPCommandType::DELAY_MILLISECONDS) {
                returnCommands.push_back(*iter);
            } else if (iter->commandType() == UDPCommandType::DELAY_MICROSECONDS) {
                returnCommands.push_back(*iter);
            } else if (iter->commandType() == UDPCommandType::FLUSH_RX) {
                returnCommands.push_back(*iter);
            } else if (iter->commandType() == UDPCommandType::FLUSH_TX) {
                returnCommands.push_back(*iter);
            } else if (iter->commandType() == UDPCommandType::FLUSH_RX_TX) {
                returnCommands.push_back(*iter);
            } else if (iter->commandType() == UDPCommandType::LOOP_START) {
                /*Move past loop_start*/
                iter++;
                int loopCount{std::stoi(iter->commandArgument())};
                std::vector<UDPCommand> loopCommands;
                int otherLoopStartCount{0};
                while ((iter->commandType() != UDPCommandType::LOOP_END) && (otherLoopStartCount == 0)) {
                    if (iter->commandType() == UDPCommandType::LOOP_START) {
                        otherLoopStartCount++;
                    }
                    loopCommands.push_back(*iter);
                    iter++;
                }
                /*move past loop_end*/
                iter++;
                /*Put the commands inside the loop into the return commands x number of times, where x is the loop count*/
                for (int i = 0; i < loopCount; i++) {
                    for (auto &innerIt : loopCommands) {
                        returnCommands.push_back(innerIt);
                    }
                }
            } else {
                throw std::runtime_error(UDP_COMMAND_TYPE_NOT_IMPLEMENTED_STRING + iter->commandArgument());
            }
        } catch (std::exception &e) {
            throw std::runtime_error(e.what());
        }
    }
    return returnCommands;
}

bool containsLoopStart(const std::vector<UDPCommand> &commands) 
{
    for (auto &it : commands) {
        if (it.commandType() == UDPCommandType::LOOP_START) {
            return true;
        }
    }
    return false;
}
