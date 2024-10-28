/*
 *  This file is part of rtl66.
 *
 *  rtl66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  rtl66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with rtl66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          audio_api.cpp
 *
 *    Provides the base class for the Audio I/O implementations.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-07
 * \updates       2023-03-17
 * \license       See above.
 *
 */

#include <cstring>                      /* std::memset()                    */

#include "c_macros.h"                   /* not_nullptr() and friends        */
#include "rtl/audio/rtaudio.hpp"        /* rtl::audio::rtaudio class        */
#include "rtl/audio/audio_api.hpp"      /* rtl::audio::audio_api class      */

namespace rtl
{

/*------------------------------------------------------------------------
 * audio_api static definitions
 *------------------------------------------------------------------------*/

const unsigned audio_api::sc_max_sample_rates = 14;
const unsigned audio_api::sc_sample_rates [] =
{
    4000, 5512, 8000, 9600, 11025, 16000, 22050,
    32000, 44100, 48000, 88200, 96000, 176400, 192000
};

/*------------------------------------------------------------------------
 * audio_api basic functions
 *------------------------------------------------------------------------*/

audio_api::audio_api () :
    api_base            (),
    m_device_list       (),
    m_currentdeviceid   (129),
    m_stream            (),
    m_show_warnings     (true)
{
    clear_stream_info();
}

bool
audio_api::open_stream
(
    stream_parameters * outparameters,
    stream_parameters * inparameters,
    stream_format sformat,
    unsigned samplerate,
    unsigned int bufferframes,
    callback_t cb,
    void * userdata,
    stream_options * options,
    rterror::callback_t errorcb
)
{
    if (m_stream.state() != stream_state::closed)
    {
        std::string msg = "audio_api::open_stream: already open";
        error(rterror::kind::invalid_use, msg);
        return false;
    }
    clear_stream_info();
    if (not_nullptr(outparameters) && outparameters->nchannels() < 1)
    {
        std::string msg = "open_stream: output stream_parameters nchannels < 1";
        error(rterror::kind::invalid_use, msg);
        return false;
    }
    if (not_nullptr(inparameters) && inparameters->nchannels() < 1)
    {
        std::string msg = "open_stream: input stream_parameters nchannels < 1";
        error(rterror::kind::invalid_use, msg);
        return false;
    }
    if (is_nullptr_2(outparameters, inparameters))
    {
        std::string msg = "open_stream: input/output stream_parameters both null";
        error(rterror::kind::invalid_use, msg);
        return false;
    }
    if (format_bytes(sformat) == 0)
    {
        std::string msg = "open_stream: format parameter undefined";
        error(rterror::kind::invalid_use, msg);
        return false;
    }

    unsigned ndevices = get_device_count();
    unsigned ochannels = 0;
    unsigned int ichannels = 0;
    if (not_nullptr(outparameters))
    {
        ochannels = outparameters->nchannels();
        if (outparameters->deviceid() >= ndevices)
        {
            std::string msg = "open_stream: output device parameter invalid";
            error(rterror::kind::invalid_use, msg);
            return false;
        }
    }
    if (not_nullptr(inparameters))
    {
        ichannels = inparameters->nchannels();
        if (inparameters->deviceid() >= ndevices)
        {
            std::string msg = "open_stream: input device parameter invalid";
            error(rterror::kind::invalid_use, msg);
            return false;
        }
    }

    bool result;
    if (ochannels > 0)
    {
        result = probe_device_open
        (
            outparameters->deviceid(), stream_mode::output, ochannels,
            outparameters->firstchannel(),
            samplerate, sformat, &bufferframes, options
        );
        if (! result)
        {
            std::string msg = "probe_device_open: failed for output";
            error(rterror::kind::system_error, msg);
            return false;
        }
    }
    if (ichannels > 0)
    {
        result = probe_device_open
        (
            inparameters->deviceid(), stream_mode::input, ichannels,
            inparameters->firstchannel(),
            samplerate, sformat, &bufferframes, options
        );
        if (! result)
        {
            if (ochannels > 0)
                close_stream();

            std::string msg = "probe_device_open: failed for output";
            error(rterror::kind::system_error, msg);
            return false;
        }
    }
    m_stream.callbackinfo().set_callbacks
    (
        reinterpret_cast<void *>(cb),
        reinterpret_cast<void *>(userdata),
        reinterpret_cast<void *>(errorcb)
    );
    if (not_nullptr(options))
        options->numberofbuffers(m_stream.nbuffers());

    m_stream.state(stream_state::stopped);
    return true;
}

/**
 *  This function MUST be implemented in all subclasses! Within each API, this
 *  function will be used to:
 *
 *   - enumerate the devices and fill or update our
 *     std::vector< RtAudio::DeviceInfo> deviceList_ class variable
 *   - store corresponding (usually API-specific) identifiers that
 *     are needed to open each device
 *   - make sure that the default devices are properly identified
 *     within the deviceList_ (unless API-specific functions are
 *     available for this purpose).
 *
 *   The function should not reprobe devices that have already been found. The
 *   function must properly handle devices that are removed or added.
 *
 *   Ideally, we would also configure callback functions to be invoked when
 *   devices are added or removed (which could be used to inform clients about
 *   changes). However, none of the APIs currently support notification of _new_
 *   devices and I don't see the usefulness of having this work only for device
 *   removal.
 */

bool
audio_api::probe_devices ()
{
    return false;
}

/**
 *  A public function to query the number of audio devices.
 *  This function performs a system query of available devices each
 *  time it is called, thus supporting devices (dis)connected \e after
 *  instantiation. If a system error occurs during processing, a
 *  warning will be issued.
 */

unsigned
audio_api::get_device_count ()
{
    unsigned result = 0;
    if (probe_devices())
        result = unsigned(device_list().size());

    return result;
}

/**
 *  Return an RtAudio::DeviceInfo structure for a specified device ID.
 *  Any device ID returned by get_device_ids() is valid, unless it has
 *  been removed between the call to get_device_ids() and this
 *  function.
 *
 *  If an invalid argument is provided, an
 *  rterror::kind::invalid_use will be passed to the user-provided
 *  error-callback function (or otherwise printed to stderr) and all
 *  members of the returned audio::device_info structure will be
 *  initialized to default, invalid values (ID = 0, empty name, ...).
 *
 *  If the specified device is the current default input or output
 *  device, the corresponding "is_default" member will have a value of
 *  "true".
 */

device_info
audio_api::get_device_info (unsigned deviceid)
{
    bool ok = deviceid > 0;
    if (ok && device_list().empty())
        ok = probe_devices();

    if (ok)
    {
        for (unsigned m = 0; m < unsigned(device_list().size()); ++m)
        {
            if (device_list()[m].ID() == deviceid)
                return device_list()[m];
        }
        return device_info();
    }
    else
    {
        std::string msg = "audio::get_device_info: ID no found";
        error(rterror::kind::invalid_parameter, msg);
        return device_info();
    }
}

/**
 *  Functions that return the ID of the default input/output devices.
 *  If the underlying audio API does not provide a "default device",
 *  the first probed input device ID will be returned. If no devices
 *  are available, the return value will be 0 (which is an invalid
 *  device identifier).
 *
 *  If not found, find the first device with input channels, set it
 *  as the default, and return the ID.
 *
 *  Should be reimplemented in subclasses if necessary.
 */

unsigned
audio_api::get_default_input_device ()
{
    bool ok = true;
    if (device_list().empty())
        ok = probe_devices();

    if (ok)
    {
        for (unsigned m = 0; m < unsigned(device_list().size()); ++m)
        {
            if (device_list()[m].is_default_input())
                return device_list()[m].ID();
        }
        for (unsigned m = 0; m < unsigned(device_list().size()); ++m)
        {
            if (device_list()[m].input_channels() > 0)
            {
                device_list()[m].is_default_input(true);
                return device_list()[m].ID();
            }
        }
    }
    return device_info::invalid_id;
}

unsigned
audio_api::get_default_output_device ()
{
    bool ok = true;
    if (device_list().empty())
        ok = probe_devices();

    if (ok)
    {
        for (unsigned m = 0; m < unsigned(device_list().size()); ++m)
        {
            if (device_list()[m].is_default_output())
                return device_list()[m].ID();
        }
        for (unsigned m = 0; m < unsigned(device_list().size()); ++m)
        {
            if (device_list()[m].output_channels() > 0)
            {
                device_list()[m].is_default_output(true);
                return device_list()[m].ID();
            }
        }
    }
    return device_info::invalid_id;
}

long
audio_api::get_stream_latency ()
{
    long result = 0;
    if (stream_mode_is_output())
        result = m_stream.latency()[api_stream::playback];

    if (stream_mode_is_input())
        result += m_stream.latency()[api_stream::record];

    return result;
}

void
audio_api::byte_swap_buffer
(
    char * buffer, unsigned samples, stream_format sformat
)
{
    char val;
    char * ptr = buffer;
    if (sformat == stream_format::sint16)
    {
        for (unsigned i = 0; i < samples; ++i)
        {
            val = *ptr;             // swap 1st and 2nd bytes.
            *ptr = *(ptr + 1);
            *(ptr + 1) = val;
            ptr += 2;               // increment 2 bytes.
        }
    }
    else if (sformat == stream_format::sint32 || sformat == stream_format::float32)
    {
        for (unsigned i = 0; i < samples; ++i)
        {
            val = *ptr;             // swap 1st and 4th bytes.
            *ptr = *(ptr + 3);
            *(ptr + 3) = val;

            ++ptr;                  // swap 2nd and 3rd bytes.
            val = *ptr;
            *ptr = *(ptr + 1);
            *(ptr + 1) = val;
            ptr += 3;               // increment 3 more bytes.
        }
    }
    else if (sformat == stream_format::sint24)
    {
        for (unsigned i = 0; i < samples; ++i)
        {
            val = *ptr;             // swap 1st and 3rd bytes.
            *ptr = *(ptr + 2);
            *(ptr + 2) = val;
            ptr += 2;               // increment 2 more bytes.
        }
    }
    else if (sformat == stream_format::float64)
    {
        for (unsigned i = 0; i < samples; ++i)
        {
            val = *ptr;             // swap 1st and 8th bytes
            *ptr = *(ptr + 7);
            *(ptr + 7) = val;

            ++ptr;                  // swap 2nd and 7th bytes
            val = *ptr;
            *ptr = *(ptr + 5);
            *(ptr + 5) = val;

            ++ptr;                  // swap 3rd and 6th bytes
            val = *ptr;
            *ptr = *(ptr + 3);
            *(ptr + 3) = val;

            ++ptr;                  // swap 4th and 5th bytes
            val = *ptr;
            *ptr = *(ptr + 1);
            *(ptr + 1) = val;
            ptr += 5;               // increment 5 more bytes.
        }
    }
}

unsigned
audio_api::format_bytes (stream_format sformat)
{
    unsigned result = 0;
    switch (sformat)
    {
        case stream_format::sint16:

            result = 2;
            break;

        case stream_format::sint32:
        case stream_format::float32:

            result = 4;
            break;

        case stream_format::float64:

            result = 8;
            break;

        case stream_format::sint24:

            result = 3;
            break;

        case stream_format::sint8:

            result = 1;
            break;

        default:

            break;
    }
    if (result == 0)
    {
        std::string msg = "audio_api::format_bytes: undefined format";
        error(rterror::kind::warning, msg);
    }
    return result;
}

void
audio_api::set_convert_info (stream_mode mode, unsigned firstchan)
{
    /*
     * Convert device to user buffer or vice versa.
     * Then set the convert_info channels to either the in-jump or out-jump.
     */

    m_stream.set_convert_info_jump(mode);
    m_stream.set_convert_jump(mode);

    /*
     * Set up the interleave/deinterleave offsets or set up for no
     * (de)interleaving..
     */

    m_stream.set_deinterleaved_offsets(mode);
    m_stream.add_channel_offsets(mode, firstchan);  /* Add channel offsets  */
}

/**
 *  This function does format conversion, input/output channel compensation, and
 *  data interleaving/deinterleaving.  24-bit integers are assumed to occupy
 *  the lower three bytes of a 32-bit integer.
 *
 *  The steps:
 *
 *  -   Clear the duplex device output buffer if there are more device outputs
 *      than user outputs.
 */

void
audio_api::convert_buffer
(
    char * outbuffer,
    char * inbuffer,
    convert_info & cinfo
)
{
    if
    (
        outbuffer == m_stream.devicebuffer() &&
        m_stream.mode() == stream_mode::duplex &&
        cinfo.outjump() > cinfo.injump()
    )
    {
        size_t bytecount = m_stream.buffersize() * cinfo.outjump() *
            format_bytes(cinfo.outformat());

        std::memset(outbuffer, 0, bytecount);
    }

    unsigned buffsize = m_stream.buffersize();
    if (cinfo.outformat() == stream_format::float64)
    {
        if (cinfo.float64_from_sint8(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float64_from_sint16(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float64_from_sint24(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float64_from_sint32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float64_from_float32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float64_from_float64(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
    }
    else if (cinfo.outformat() == stream_format::float32)
    {
        if (cinfo.float32_from_sint8(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float32_from_sint16(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float32_from_sint24(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float32_from_sint32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float32_from_float32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.float32_from_float64(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
    }
    else if (cinfo.outformat() == stream_format::sint32)
    {
        if (cinfo.sint32_from_sint8(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint32_from_sint16(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint32_from_sint24(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint32_from_sint32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint32_from_float32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint32_from_float64(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
    }
    else if (cinfo.outformat() == stream_format::sint24)
    {
        if (cinfo.sint24_from_sint8(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint24_from_sint16(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint24_from_sint24(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint24_from_sint32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint24_from_float32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint24_from_float64(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
    }
    else if (cinfo.outformat() == stream_format::sint16)
    {
        if (cinfo.sint16_from_sint8(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint16_from_sint16(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint16_from_sint24(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint16_from_sint32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint16_from_float32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint16_from_float64(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
    }
    else if (cinfo.outformat() == stream_format::sint8)
    {
        if (cinfo.sint8_from_sint8(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint8_from_sint16(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint8_from_sint24(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint8_from_sint32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint8_from_float32(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
        else if (cinfo.sint8_from_float64(buffsize, outbuffer, inbuffer))
        {
            // it was converted
        }
    }
}

/**
 *  Not implemented in RtAudio:
 *
 *      void verify_stream ();
 */

/**
 *  Subclasses that do not provide their own implementation of
 *  get_stream_time() should call this function once per buffer I/O to
 *  provide basic stream time support.
 */

void
audio_api::tick_stream_time ()
{
    double strmtime = m_stream.streamtime();
    strmtime += m_stream.buffersize() * 1.0 / m_stream.samplerate();
    m_stream.streamtime(strmtime);

  /*
#if defined( HAVE_GETTIMEOFDAY )
    gettimeofday( &stream_.lastTickTimestamp, NULL );
#endif
  */
}

}           // namespace rtl

/*
 * audio_api.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

