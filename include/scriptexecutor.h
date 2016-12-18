#ifndef UDPCOMMUNICATION_SCRIPTEXECUTOR_H
#define UDPCOMMUNICATION_SCRIPTEXECUTOR_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <functional>
#include <cstdlib>
#include <utility>
#include <udpduplex.h>
#include <generalutilities.h>

#include "udpcommunicationstrings.h"
#include "scriptreader.h"

class ScriptExecutor
{
public:
    ScriptExecutor(const std::string &scriptFilePath);
    void setScriptFilePath(const std::string &scriptFilePath);
    std::shared_ptr<ScriptReader> scriptReader() const;
    void execute(std::shared_ptr<UDPDuplex> udpDuplex, 
                 const std::function<void(const std::string &)> &printRxResult, 
                 const std::function<void(const std::string &)> &printTxResult,
                 const std::function<void(DelayType, int)> &printDelayResult,
                 const std::function<void(FlushType)> &printFlushResult,
                 const std::function<void(LoopType, int, int)> &printLoopResult);
private:
    std::shared_ptr<ScriptReader> m_scriptReader;
    std::vector<UDPCommand> m_scriptCommands;

    std::vector<UDPCommand> doUnrollLoopCommands(const std::vector<UDPCommand> &UDPCommands);
    bool containsLoopStart(const std::vector<UDPCommand> &commands);
    std::pair<int, int> findInnerLoopIndexes(const std::vector<UDPCommand> udpCommands);


};


#endif //UDPCOMMUNICATION_SCRIPTEXECUTOR_H
