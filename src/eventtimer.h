/***********************************************************************
*    eventtimer.h:                                                     *
*    Class EventTimer, for timing events and getting calculated times  *
*    Copyright (c) 2017 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.com/Pinguinsan/tjlutils                            *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the declarations of an EventTimer template class  *
*    EventTimer is used to time events that need to be able to pause   *
*    The class also allows access to calculated out hours/minutes/etc, *
*    suitable for a running game timer (eg 00:01.295)                  *
*    The default clock type is std::chrono::steady_clock, but this can *
*    be changed during instantiation via the template parameter to any *
*    clock specified in the std::chrono namespace (ie system_clock)    *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#ifndef TJLUTILS_EVENTTIMER_H
#define TJLUTILS_EVENTTIMER_H

#include <chrono>
#include <string>
#include <thread>
#include <future>
#include <sstream>
#include <list>
#include <functional>

struct TimePoint
{
    unsigned long hours;
    unsigned long minutes;
    unsigned long seconds;
    unsigned long milliseconds;
};

template <typename ClockType = std::chrono::steady_clock>
class EventTimer
{
    using platform_clock_t = ClockType;

public:
    EventTimer<ClockType>() :
        m_startTime{platform_clock_t::now()},
        m_endTime{platform_clock_t::now()},
        m_totalTime{0},
        m_hours{0},
        m_minutes{0},
        m_seconds{0},
        m_milliseconds{0},
        m_isPaused{false}
    {

    }

    EventTimer<ClockType>(const EventTimer<ClockType> &other) :
        m_startTime{other.m_startTime},
        m_endTime{other.m_endTime},
        m_totalTime{other.m_totalTime},
        m_hours{other.m_hours},
        m_minutes{other.m_minutes},
        m_seconds{other.m_seconds},
        m_milliseconds{other.m_milliseconds},
        m_isPaused{other.m_isPaused}
    {

    }

    template <typename T> static inline std::string toStdString(const T &t) {
        return dynamic_cast<std::stringstream &>(std::stringstream{} << t).str();
    }

    void start()
    {
        this->m_totalTime = 0;
        this->m_hours = 0;
        this->m_minutes = 0;
        this->m_seconds = 0;
        this->m_milliseconds = 0;
        this->m_startTime = platform_clock_t::now();
        this->m_endTime = this->m_startTime;
        this->m_isPaused = false;
    }

    void restart()
    {
        return this->start();
    }

    void pause()
    {
        this->stop();
    }

    void stop()
    {
        this->m_isPaused = true;
    }

    void resume()
    {
        this->m_isPaused = false;
    }

    void update()
    {
        if (this->m_isPaused)  {
            this->m_startTime = platform_clock_t::now() - this->m_rawTime;
        } else {
            this->m_endTime = platform_clock_t::now();
            this->m_totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(this->m_endTime-this->m_startTime).count();
            this->m_rawTime = std::chrono::duration_cast<std::chrono::milliseconds>(this->m_endTime - this->m_startTime);
            this->m_hours = (this->m_totalTime/MILLISECONDS_PER_HOUR);
            this->m_minutes = (this->m_totalTime - (this->m_hours * MILLISECONDS_PER_HOUR)) / MILLISECONDS_PER_MINUTE;
            this->m_seconds = (this->m_totalTime - (this->m_hours * MILLISECONDS_PER_HOUR) - (this->m_minutes * MILLISECONDS_PER_MINUTE)) / MILLISECONDS_PER_SECOND;
            this->m_milliseconds = (this->m_totalTime - (this->m_hours * MILLISECONDS_PER_HOUR) - (this->m_minutes * MILLISECONDS_PER_MINUTE) - (this->m_seconds * MILLISECONDS_PER_SECOND));
        }
    }

    long long int totalMicroseconds()
    {
        this->update();
        return static_cast<long long int>(static_cast<double>(this->m_totalTime) * MICROSECONDS_PER_MILLISECOND);
    }

    TimePoint timePoint()
    {
        this->update();
        TimePoint returnPoint;
        returnPoint.hours = this->m_hours;
        returnPoint.minutes = this->m_minutes;
        returnPoint.seconds = this->m_seconds;
        returnPoint.milliseconds = this->m_milliseconds;
        return returnPoint;
    }

    long long int totalMilliseconds()
    {
        this->update();
        return this->m_totalTime;
    }

    long long int totalSeconds()
    {
        this->update();
        return static_cast<long long int>(static_cast<double>(this->m_totalTime) / MILLISECONDS_PER_SECOND);
    }

    long long int totalMinutes()
    {
        this->update();
        return static_cast<long long int>(static_cast<double>(this->m_totalTime) / MILLISECONDS_PER_MINUTE);
    }

    long long int totalHours()
    {
        this->update();
        return static_cast<long long int>(static_cast<double>(this->m_totalTime) / MILLISECONDS_PER_HOUR);
    }

    long long int totalTime()
    {
        this->update();
        return this->m_totalTime;
    }

    std::string toString(uint8_t millisecondDigits = 3)
    {
        if (!this->m_isPaused) {
            this->update();
        }
        std::string returnString{""};
        if (this->m_hours != 0) {
            returnString = toStdString(this->m_hours) + ':';
        }
        returnString += toStdString(this->m_minutes)
                        + ':'
                        + toStdString(this->m_seconds)
                        + '.';
        long long int millisecond{this->m_milliseconds};
        std::string millisecondsString{""};
        if (millisecond < 10) {
            millisecondsString = "00" + toStdString(millisecond);
        } else if (millisecond < 100) {
            millisecondsString = "0" + toStdString(millisecond);
        } else {
            millisecondsString = toStdString(millisecond);
        }
        returnString += millisecondsString.substr(0, millisecondDigits);

        return returnString;
    }

    long long int hours()
    {
        return this->m_hours;
    }

    long long int minutes()
    {
        return this->m_minutes;
    }

    long long int seconds()
    {
        return this->m_seconds;
    }

    long long int milliseconds()
    {
        return this->m_milliseconds;
    }

    inline bool isPaused() const
    {
        return this->m_isPaused;
    }

    inline bool isRunning() const
    {
        return (!this->isPaused());
    }

private:
    std::chrono::time_point<platform_clock_t> m_startTime;
    std::chrono::time_point<platform_clock_t> m_endTime;
    std::chrono::milliseconds m_rawTime;
    long long int m_totalTime;
    long long int m_hours;
    long long int m_minutes;
    long long int m_seconds;
    long long int m_milliseconds;
    bool m_isPaused;
    bool m_isStopped;

    static const int INVALIDATE_CACHE_TIMEOUT{100};

    static const long long int constexpr NANOSECONDS_PER_MICROSECOND{1000};
    static const long long int constexpr NANOSECONDS_PER_MILLISECOND{1000000};
    static const long long int constexpr NANOSECONDS_PER_SECOND{1000000000};
    static const long long int constexpr NANOSECONDS_PER_MINUTE{60000000000};
    static const long long int constexpr NANOSECONDS_PER_HOUR{3600000000000};
    static const long long int constexpr NANOSECONDS_PER_DAY {86400000000000};

    static const long long int constexpr MICROSECONDS_PER_SECOND{1000000};
    static const long long int constexpr MICROSECONDS_PER_MINUTE{60000000};
    static const long long int constexpr MICROSECONDS_PER_HOUR{3600000000};
    static const long long int constexpr MICROSECONDS_PER_DAY{86400000000};
    static const long long int constexpr MILLISECONDS_PER_SECOND{1000};
    static const long long int constexpr MICROSECONDS_PER_MILLISECOND{1000};
    static const long long int constexpr MILLISECONDS_PER_MINUTE{60000};
    static const long long int constexpr MILLISECONDS_PER_HOUR{3600000};
    static const long long int constexpr MILLISECONDS_PER_DAY{86400000};

    static const long long int constexpr SECONDS_PER_MINUTE{60};
    static const long long int constexpr SECONDS_PER_HOUR{3600};

    static const long long int constexpr MINUTES_PER_HOUR{60};
    static const long long int constexpr MINUTES_PER_DAY{1440};

    static const long long int constexpr HOURS_PER_DAY{24};

};

typedef EventTimer<std::chrono::steady_clock> SteadyEventTimer;
typedef EventTimer<std::chrono::high_resolution_clock> HighResolutionEventTimer;
typedef EventTimer<std::chrono::system_clock> SystemEventTimer;

#endif //TJLUTILS_EVENTTIMER_H
