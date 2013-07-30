//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace logger { // logger::debug()

/**
 * Binary dump a container to the log binary file.
 * Usually only for byte_arrays.
 * @todo Add timestamps for replaying.
 */
class file_dump
{
    static std::mutex m;
public:
    template <typename T>
    file_dump(const T& data) {
        m.lock();
        std::ofstream out("dump.bin", std::ios::out|std::ios::app|std::ios::binary);
        boost::archive::binary_oarchive oa(out, boost::archive::no_header);
        char t = 'T';
        char r = 'R';
        char e = 'E';
        char k = 'K'; // isn't it a bit... ridiculous?
        size_t size = data.size();
        oa << t << r << e << k << size << data;
    }
    ~file_dump() { m.unlock(); }
};

/**
 * Base class for logging output.
 *
 * @todo: to control different output of different debug levels, consider replacing the
 * streambuf on std::clog depending on logging levels, e.g.
 * class debug would set clog wrbuf to either some output buf or null buf.
 */
class logging
{
    static std::mutex m;
protected:
    logging() {
        m.lock();
	}
    ~logging() { std::clog << std::endl; m.unlock(); }

public:
    template <typename T>
    std::ostream& operator << (const T& v) { std::clog << v; return std::clog; }
};

class debug : public logging
{
public:
    debug() : logging() {
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        std::clog << "[DEBUG] " << boost::posix_time::to_iso_extended_string(now) << " T#" << std::this_thread::get_id() << ' ';
    }
};

class info : public logging
{
public:
    info() : logging() {
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        std::clog << "[INFO ] " << boost::posix_time::to_iso_extended_string(now) << " T#" << std::this_thread::get_id() << ' ';
    }
};

class warning : public logging
{
public:
    warning() : logging() {
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        std::clog << "[WARN ] " << boost::posix_time::to_iso_extended_string(now) << " T#" << std::this_thread::get_id() << ' ';
    }
};

class fatal : public logging
{
public:
    fatal() : logging() {
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        std::clog << "[FATAL] " << boost::posix_time::to_iso_extended_string(now) << " T#" << std::this_thread::get_id() << ' ';
    }
    ~fatal() { std::abort(); }
};

} // namespace logger

/**
 * Helper to output a hexadecimal value with formatting to an iostream.
 * Usage: io << hex(value, 8, true, false)
 */
struct hex_output
{
    int ch;
    int width;
    bool fill;
    bool base;

    hex_output(int c, int w, bool f, bool b) : ch(c), width(w), fill(f), base(b) {}
};

inline std::ostream& operator<<(std::ostream& o, const hex_output& hs)
{
    return (o << std::setw(hs.width) << std::setfill(hs.fill ? '0' : ' ') << std::hex << (hs.base ? std::showbase : std::noshowbase) << hs.ch);
}

inline hex_output hex(int c, int w = 2, bool f = true, bool b = false)
{
    return hex_output(c,w,f,b);
}

