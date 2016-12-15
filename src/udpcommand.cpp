#include "udpcommand.h"

UDPCommand::UDPCommand(UDPCommandType commandType, const std::string &commandArgument) :
    m_commandType{commandType},
    m_commandArgument{commandArgument}
{

}

UDPCommandType UDPCommand::commandType() const
{
    return this->m_commandType;
}

std::string UDPCommand::commandArgument() const
{
    return this->m_commandArgument;
}

void UDPCommand::setCommandType(UDPCommandType commandType)
{
    this->m_commandType = commandType;
}

void UDPCommand::setCommandArgument(const std::string &commandArgument)
{
    this->m_commandArgument = commandArgument;
}