//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

// Set to 1 if you want to generate gnuplot file of delays.
#define DELAY_PLOT 0

/**
 * Record timing information into a gnuplot-compatible file.
 */
class plotfile
{
#if DELAY_PLOT
    mutex m;
    ofstream out_;
public:
    plotfile()
        : out_("timeplot.dat", ios::out|ios::trunc|ios::binary)
    {
        out_ << "# gnuplot data for packet timing\r\n"
             << "# ts\tlocal_ts\tdifference\r\n";
    }

    ~plotfile()
    {
        out_ << "\r\n\r\n"; // gnuplot end marker
        out_.close();
    }

    void dump(int64_t ts, int64_t local_ts)
    {
        lock_guard<mutex> guard(m);
        out_ << ts << '\t' << local_ts << '\t' << fabs(local_ts - ts) << "\r\n";
    }
#endif
};
