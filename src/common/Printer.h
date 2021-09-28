/*********************************************************************
 Author: Antti Hyvarinen <antti.hyvarinen@gmail.com>

 OpenSMT2 -- Copyright (C) 2012 - 2016, Antti Hyvarinen
                           2008 - 2012, Roberto Bruttomesso

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/

#ifndef PRINTER_H
#define PRINTER_H
#include <string.h>
#include <ostream>
#include <iostream>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "Timer.h"

namespace opensmt {

namespace Color
{
    enum Code {
        FG_RED      = 31,
        FG_GREEN    = 32,
        FG_BLUE     = 34,
        FG_DEFAULT  = 39,
        BG_RED      = 41,
        BG_GREEN    = 42,
        BG_BLUE     = 44,
        BG_DEFAULT  = 49
    };
}

class synced_stream  {
    public:
        /**
         * @brief Construct a new synced stream.
         *
         * @param _out_stream The output stream to print to. The default value is std::cout.
         */
        synced_stream(std::ostream &_out_stream = std::cout)
                : out_stream(_out_stream) {};

        /**
         * @brief Print any number of items into the thread-safe output stream.
         */
        template <typename... T>
        void print(Color::Code colorCode, const T &...items)
        {
            const std::scoped_lock lock(stream_mutex);
            if (colorCode != Color::FG_DEFAULT)
                out_stream << "\033[" << colorCode << "m";
            (out_stream << ... << items);
            if (colorCode != Color::FG_DEFAULT)
                out_stream << "\033[" << Color::FG_DEFAULT << "m";
        }

        /**
         * @brief Print any number of items into the thread-safe output stream, seperated by a newline.
         */
        template <typename... T>
        void println(Color::Code colorCode , const T &...items)
        {
            print(colorCode, items..., '\n');
        }

    private:
        /**
         * @brief A mutex to synchronize printing.
         */
        mutable std::mutex stream_mutex = {};

        std::ostream &out_stream;

    };

class PrintStopWatch {
    protected:
        char* name;
        StoppableWatch timer;
        synced_stream& ss;
        Color::Code colorCode;

    public:
        PrintStopWatch(const char* _name, synced_stream& _ss, Color::Code cc = Color::Code::FG_DEFAULT)
        : ss(_ss), colorCode(cc)
        {
            timer.start();
            int size = strlen(_name);
            name = (char*)malloc(size+1);
            strcpy(name, _name);
        }

        ~PrintStopWatch()
        {
            ss.println(colorCode, ";", name, " ", timer.elapsed_time_milliseconds());
            free(name);
        }
};

}
#endif
