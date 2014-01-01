//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "voicebox/rtaudio_source.h"
#include "voicebox/audio_hardware.h"
#include "voicebox/audio_service.h"

namespace voicebox {

void rtaudio_source::set_enabled(bool enabling)
{
    logger::debug(TRACE_ENTRY) << __PRETTY_FUNCTION__ << " " << enabling;
    if (enabling and !is_enabled())
    {
        if (frame_size() <= 0 or sample_rate() <= 0.0)
        {
            logger::warning() << "Bad frame size " << frame_size()
                << " or sample rate " << sample_rate();
            return;
        }

        bool wasempty = audio_hardware::add_instream(this);
        super::set_enabled(true);
        if (wasempty or audio_hardware::get_sample_rate() < sample_rate()) {
            audio_hardware::reopen(); // (re-)open at suitable rate
        }
    }
    else if (!enabling and is_enabled())
    {
        bool isempty = audio_hardware::remove_instream(this);
        super::set_enabled(false);
        if (isempty) {
            audio_hardware::reopen(); // close or reopen without input
        }
    }
}

} // voicebox namespace
