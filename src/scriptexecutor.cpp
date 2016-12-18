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
    std::vector<UDPCommand> copyCommands{udpCommands};
    if (!ScriptExecutor::containsLoopStart(copyCommands)) {
        return copyCommands;
    }
    while (ScriptExecutor::containsLoopStart(copyCommands)) {
        std::pair<int, int> innerLoopPositions = findInnerLoopIndexes(copyCommands);
        std::vector<UDPCommand> toUnroll{};
        std::vector<UDPCommand> temp{};
        int numberOfLoops{std::stoi(copyCommands.at(innerLoopPositions.first).commandArgument())};
        for (int loops = 0; loops < numberOfLoops; loops++) {
            for (int i = innerLoopPositions.first+1; i < innerLoopPositions.second; i++) {
                toUnroll.emplace_back(copyCommands.at(i));
            }
        }
        for (int i = 0; i < innerLoopPositions.first; i++) {
            temp.emplace_back(copyCommands.at(i));
        }
        for (auto &it : toUnroll) {
            temp.emplace_back(it);
        }
        for (int i = innerLoopPositions.second+1; i < copyCommands.size(); i++) {
            temp.emplace_back(copyCommands.at(i));
        }
        copyCommands = temp; 
    }
    return copyCommands;
}
std::pair<int, int> ScriptExecutor::findInnerLoopIndexes(const std::vector<UDPCommand> udpCommands)
{
    int lastLoopStartPosition{0};
    int lastLoopEndPosition{0};
    for (int i = 0; i < udpCommands.size(); i++) {
        if (udpCommands.at(i).commandType() == UDPCommandType::LOOP_START) {
            lastLoopStartPosition = i;
        }
    }

    for (int i = lastLoopStartPosition; i < udpCommands.size(); i++) {
        if (udpCommands.at(i).commandType() == UDPCommandType::LOOP_END) {
            if (lastLoopStartPosition == 0) {
                lastLoopEndPosition = lastLoopStartPosition + i;
            } else {
                lastLoopEndPosition = lastLoopStartPosition + i - 1;
            }
            break;
        }
    }
    return std::make_pair(lastLoopStartPosition, lastLoopEndPosition);
}

bool ScriptExecutor::containsLoopStart(const std::vector<UDPCommand> &commands) 
{
    for (auto &it : commands) {
        if (it.commandType() == UDPCommandType::LOOP_START) {
            return true;
        }
    }
    return false;
}
