/***********************************************************************
*    prettyprinter.h:                                                  *
*    PrettyPrinter class, for printing colored text to a POSIX terminal*
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.serial/tlewiscpp/tjlutils                         *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the declarations of a PrettyPrinter class         *
*    It is used to print colored text to a POSIX terminal              *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/


#ifndef TJUTILS_PRETTYPRINTER_H
#define TJUTILS_PRETTYPRINTER_H

#include <iostream>
#include <ostream>
#include <vector>

enum ForegroundColor {
    FG_DEFAULT       = 39,
    FG_BLACK         = 30,
    FG_RED           = 31,
    FG_GREEN         = 32, 
    FG_YELLOW        = 33, 
    FG_BLUE          = 34, 
    FG_MAGENTA       = 35, 
    FG_CYAN          = 36, 
    FG_LIGHT_GRAY    = 37,
    FG_DARK_GRAY     = 90, 
    FG_LIGHT_RED     = 91, 
    FG_LIGHT_GREEN   = 92, 
    FG_LIGHT_YELLOW  = 93, 
    FG_LIGHT_BLUE    = 94, 
    FG_LIGHT_MAGENTA = 95, 
    FG_LIGHT_CYAN    = 96, 
    FG_WHITE         = 97
};

enum BackgroundColor {
    BG_DEFAULT       = 49,
    BG_BLACK         = 40,
    BG_RED           = 41,
    BG_GREEN         = 42,
    BG_YELLOW        = 43,
    BG_BLUE          = 44,
    BG_MAGENTA       = 45,
    BG_CYAN          = 46,
    BG_LIGHT_GRAY    = 47,
    BG_DARK_GRAY     = 100,
    BG_LIGHT_RED     = 101,
    BG_LIGHT_GREEN   = 102,
    BG_LIGHT_YELLOW  = 103,
    BG_LIGHT_BLUE    = 104,
    BG_LIGHT_MAGENTA = 105,
    BG_LIGHT_CYAN    = 106,
    BG_WHITE         = 107
};

enum FontAttribute {
    FA_DEFAULT       = 0b00000000,
    FA_BOLD          = 0b00000001,
    FA_DIM           = 0b00000010,
    FA_UNDERLINED    = 0b00000100,
    FA_BLINK         = 0b00001000,
    FA_REVERSE       = 0b00010000,
    FA_HIDDEN        = 0b00100000
};

class PrettyPrinter 
{
public:
    PrettyPrinter();
    PrettyPrinter(std::ostream *outputStream);
    PrettyPrinter(ForegroundColor ForegroundColor);
    PrettyPrinter(BackgroundColor backgroundColor);
    PrettyPrinter(ForegroundColor ForegroundColor, BackgroundColor backgroundColor);
    PrettyPrinter(ForegroundColor ForegroundColor, std::ostream *outputStream);
    PrettyPrinter(BackgroundColor BackgroundColor, std::ostream *outputStream);
    PrettyPrinter(ForegroundColor ForegroundColor, BackgroundColor backgroundColor, std::ostream *outputStream);
    void setForegroundColor(ForegroundColor ForegroundColor);
    void setBackgroundColor(BackgroundColor backgroundColor);
    void setOutputStream(std::ostream *outputStream);
    void setFontAttributes(int fontAttributes);
    ForegroundColor foregroundColor() const;
    BackgroundColor backgroundColor() const;
    std::ostream *outputStream();
    int fontAttributes() const;

    void resetBackgroundColor();
    void resetForegroundColor();
    void resetFontAttributes();
    
    template<typename T>
    void operator<<(const T &toPrint)
    {
        return this->print(toPrint, *this->m_outputStream, this->m_foregroundColor, this->m_backgroundColor);
    }

    template <typename T>
    void print(const T& toPrint)
    {
        *this->m_outputStream << PrettyPrinter::s_DEFAULT_TERMINAL_FONT_ATTRIBUTES;
        *this->m_outputStream << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE << this->m_foregroundColor << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
        *this->m_outputStream << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE << this->m_backgroundColor << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
        for (auto &it : this->getFontAttributes(this->m_fontAttributes)) {
            *this->m_outputStream << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE << it << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
        }
        *this->m_outputStream << toPrint;
        this->resetStreamToDefault(this->m_outputStream);
    }

    template <typename T>
    void println(const T& toPrint)
    {
        *this->m_outputStream << PrettyPrinter::s_DEFAULT_TERMINAL_FONT_ATTRIBUTES;
        *this->m_outputStream << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE << this->m_foregroundColor << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
        *this->m_outputStream << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE << this->m_backgroundColor << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
        for (auto &it : this->getFontAttributes(this->m_fontAttributes)) {
            *this->m_outputStream << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE << it << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
        }
        *this->m_outputStream << toPrint;
        *this->m_outputStream << std::endl;
        this->resetStreamToDefault(this->m_outputStream);
    }

    void println()
    {
        return this->println("");
    }

    static const BackgroundColor COMMON_BACKGROUND_COLOR{BackgroundColor::BG_DEFAULT};
    static const int COMMON_FONT_ATTRIBUTE{(FontAttribute::FA_BOLD | FontAttribute::FA_UNDERLINED)};
    static const ForegroundColor TX_COLOR{ForegroundColor::FG_BLUE};
    static const ForegroundColor RX_COLOR{ForegroundColor::FG_RED};
    static const ForegroundColor DELAY_COLOR{ForegroundColor::FG_GREEN};
    static const ForegroundColor FLUSH_COLOR{ForegroundColor::FG_DARK_GRAY};
    static const ForegroundColor LOOP_COLOR{ForegroundColor::FG_CYAN};
    static const ForegroundColor LIST_COLOR{ForegroundColor::FG_YELLOW};

private:
    ForegroundColor m_foregroundColor;
    BackgroundColor m_backgroundColor;
    std::ostream *m_outputStream;
    int m_fontAttributes;

    void resetStreamToDefault(std::ostream *oStream);
    std::vector<int> getFontAttributes(int attr);

    static const char *s_TERMINAL_ESCAPE_SEQUENCE_BASE;
    static const char *s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
    static const char *s_DEFAULT_TERMINAL_FOREGROUND_COLOR;
    static const char *s_DEFAULT_TERMINAL_BACKGROUND_COLOR;
    static const char *s_DEFAULT_TERMINAL_FONT_ATTRIBUTES;

    static const std::vector<ForegroundColor> s_FOREGROUND_TERMINAL_COLORS_CONTAINER;
    static const std::vector<BackgroundColor> s_BACKGROUND_TERMINAL_COLORS_CONTAINER;

    static const int s_RESET_FONT_ATTRIBUTE_OFFSET;

    enum UnderlyingFontAttribute {
        UFA_DEFAULT       = 0,
        UFA_BOLD          = 1,
        UFA_DIM           = 2,
        UFA_UNDERLINED    = 4,
        UFA_BLINK         = 5,
        UFA_REVERSE       = 7,
        UFA_HIDDEN        = 8
    };
};

#endif //TJUTILS_PRETTYPRINTER_H