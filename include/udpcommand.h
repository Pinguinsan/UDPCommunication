#ifndef UDPCOMMUNICATION_UDPCOMMAND_H
#define UDPCOMMUNICATION_UDPCOMMAND_H

#include <string>

enum class UDPCommandType { DELAY_SECONDS, DELAY_MILLISECONDS, DELAY_MICROSECONDS, WRITE, READ, FLUSH_RX, FLUSH_TX, FLUSH_RX_TX, LOOP_START, LOOP_END, COMMAND_UNSPECIFIED };
enum class DelayType { SECONDS, MILLISECONDS, MICROSECONDS };
enum class FlushType { RX, TX, RX_TX };
enum class LoopType { START, END };

class UDPCommand
{
public:
    UDPCommand(UDPCommandType commandType, const std::string &commandArgument);
    UDPCommandType commandType() const;
    std::string commandArgument() const;
    void setCommandType(UDPCommandType commandType);
    void setCommandArgument(const std::string &commandArgument);

private:
    UDPCommandType m_commandType;
    std::string m_commandArgument;
};

#endif //UDPCOMMUNICATION_UDPCOMMAND_H