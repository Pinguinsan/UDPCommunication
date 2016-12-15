#ifndef SERIALCOMMUNICATION_SCRIPTREADER_H
#define SERIALCOMMUNICATION_SCRIPTREADER_H

#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <memory>
#include <algorithm>

#include <fileutilities.h>
#include <generalutilities.h>
#include <mathutilities.h>

#include "udpcommand.h"
#include "udpcommunicationstrings.h"

class ScriptReader
{
public:
    ScriptReader(const std::string &scriptFilePath);
    std::shared_ptr<std::vector<UDPCommand>> commands() const;
private:
    std::string m_scriptFilePath;
    std::shared_ptr<std::vector<UDPCommand>> m_commands;

};

#endif //SERIALCOMMUNICATIONC_CONFIGURATIONFILEREADER_H
