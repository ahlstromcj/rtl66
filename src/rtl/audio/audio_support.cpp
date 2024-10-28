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
 * \file          audio_support.cpp
 *
 *    Provides a number of audio support classes and structures in the
 *    rtl::audio namespace.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-10
 * \updates       2024-01-16
 * \license       See above.
 *
 */

#include <cmath>                        /* std::llround()                   */

#include "c_macros.h"                   /* not_nullptr() and friends        */
#include "rtl/audio/audio_support.hpp"  /* rtl::audio_support definitions   */

namespace rtl
{

/*------------------------------------------------------------------------
 * stream_options
 *------------------------------------------------------------------------*/

stream_options::stream_options () :
    m_flags             (stream_flags::none),
    m_numberofbuffers   (0),
    m_streamname        (),
    m_priority          (0)
{
    // No code
}

/*------------------------------------------------------------------------
 * S24 data type
 *------------------------------------------------------------------------*/

S24::S24 () : m_c3()
{
    // no code
}

S24 &
S24::operator = (const int & i)
{
    m_c3[0] = (unsigned char)(i & 0x000000ff);
    m_c3[1] = (unsigned char)((i & 0x0000ff00) >> 8);
    m_c3[2] = (unsigned char)((i & 0x00ff0000) >> 16);
    return *this;
}

int
S24::as_int () const
{
    int i = m_c3[0] | (m_c3[1] << 8) | (m_c3[2] << 16);
    if (i & 0x800000)
        i |= ~0xffffff;

    return i;
}

/*------------------------------------------------------------------------
 * callback_info
 *------------------------------------------------------------------------*/

callback_info::callback_info () :
    m_object                (nullptr),
//  m_thread                (), // ThreadHandle
    m_callback              (nullptr),
    m_userdata              (nullptr),
    m_errorcallback         (nullptr),
    m_apiinfo               (nullptr),
    m_isrunning             (false),
    m_dorealtime            (false),
    m_priority              (0),
    m_devicedisconnected    (false)
{
    // Any code needed?
}

callback_info::~callback_info ()
{
    // Any code needed to delete the various void pointers?
}

void
callback_info::clear ()
{
    // Any code needed to delete the various void pointers?

    callback(nullptr);
    userdata(nullptr);
    isrunning(false);
    devicedisconnected(false);
}

void
callback_info::set_callbacks
(
    void * callback,
    void * userdata,
    void * errorcallback
)
{
    m_callback = callback;
    m_userdata = userdata;
    m_errorcallback = errorcallback;
}

/*------------------------------------------------------------------------
 * convert_info
 *------------------------------------------------------------------------*/

convert_info::convert_info () :
    m_channels  (0),
    m_injump    (0),
    m_outjump   (0),
    m_informat  (stream_format::none),
    m_outformat (stream_format::none),
    m_inoffset  (),
    m_outoffset ()
{
    // Any code needed?
}

void
convert_info::clear ()
{
    m_channels = 0;
    m_injump = 0;
    m_outjump = 0;
    m_informat = stream_format::none;
    m_outformat = stream_format::none;
    m_inoffset.clear();
    m_outoffset.clear();
}

void
convert_info::set_convert_info_jump
(
    unsigned injump, unsigned outjump,
    stream_format informat, stream_format outformat
)
{
    m_injump = injump;
    m_outjump = outjump;
    m_informat = informat;
    m_outformat = outformat;
}

void
convert_info::set_convert_jump ()
{
    if (m_injump < m_outjump)
        m_channels = m_injump;
    else
        m_channels = m_outjump;
}

/**
 *  A helper function for efficiency in api_stream::set_convert_interleaved().
 *
 * \param buffersize
 *      Provides the api_stream's buffer size.
 *
 * \param input
 *      If true, the k * buffersize push-count is applied to input, otherwise it
 *      is applied to output.
 */

void
convert_info::set_deinterleave_offsets (unsigned buffersize, bool input)
{
    for (int k = 0; k < m_channels; ++k)
    {
        if (input)
        {
            m_inoffset.push_back(k * buffersize);
            m_outoffset.push_back(k);
            m_injump = 1;
        }
        else
        {
            m_inoffset.push_back(k);
            m_outoffset.push_back(k * buffersize);
            m_outjump = 1;
        }
    }
}

/**
 *  Another helper function for api_stream::set_convert_interleaved().
 *
 * \param buffersize
 *      Provides the api_stream's buffer size.
 *
 * \param userinterleaved
 *      If true, the k * buffersize push-count is not applied, otherwise it
 *      is applied to input and output.
 */

void
convert_info::set_no_interleaved_offsets
(
    unsigned buffersize, bool userinterleaved
)
{
    for (int k = 0; k < m_channels; ++k)
    {
        if (userinterleaved)
        {
            m_inoffset.push_back(k);
            m_outoffset.push_back(k);
        }
        else
        {
            m_inoffset.push_back(k * buffersize);
            m_outoffset.push_back(k * buffersize);
            m_injump = 1;
            m_outjump = 1;
        }
    }
}

void
convert_info::add_channel_offsets (stream_mode mode, unsigned amount)
{
    for (int k = 0; k < m_channels; ++k)
    {
        if (mode == stream_mode::output)
            m_outoffset[k] += amount;
        else
            m_inoffset[k] += amount;
    }
}

/**
 *  Performs the given conversion, if applicable to this convert_info object.
 *  The input buffer is converted and placed into the output buffer.
 *
 * \param buffsize
 *      Provides the api_stream::buffersize() value.
 *
 * \param outbuffer
 *      The output buffer. The convert_info format should be
 *      stream_format::float64, but this is \a not tested.
 *      For speed, this pointer is assumed to be valid.
 *
 * \param inbuffer
 *      The input buffer. The convert_info format should be
 *      stream_format::sint8. This a\ is tested for applicability.
 *      For speed, this pointer is assumed to be valid.
 *
 * \return
 *      If true, the conversion was applicable and was performed.
 *      Otherwise, move on to try the next conversion.
 */

bool
convert_info::float64_from_sint8
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint8;
    if (result)
    {
        Float64 * out = reinterpret_cast<Float64 *>(outbuffer);
        char * in = inbuffer;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float64>(in[m_inoffset[j]]) / 128.0;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float64_from_sint16
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint16;
    if (result)
    {
        Float64 * out = reinterpret_cast<Float64 *>(outbuffer);
        Int16 * in = reinterpret_cast<Int16 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float64>(in[m_inoffset[j]]) / 32768.0;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float64_from_sint24
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint24;
    if (result)
    {
        Float64 * out = reinterpret_cast<Float64 *>(outbuffer);
        Int24 * in = reinterpret_cast<Int24 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float64>(in[m_inoffset[j]]) / 8388608.0;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float64_from_sint32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint32;
    if (result)
    {
        Float64 * out = reinterpret_cast<Float64 *>(outbuffer);
        Int32 * in = reinterpret_cast<Int32 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float64>(in[m_inoffset[j]]) / 2147483648.0;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float64_from_float32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float32;
    if (result)
    {
        Float64 * out = reinterpret_cast<Float64 *>(outbuffer);
        Float32 * in = reinterpret_cast<Float32 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float64>(in[m_inoffset[j]]);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::float64_from_float64
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float64;
    if (result)
    {
        Float64 * out = reinterpret_cast<Float64 *>(outbuffer);
        Float64 * in = reinterpret_cast<Float64 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
                out[m_outoffset[j]] = in[m_inoffset[j]];

            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float32_from_sint8
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint8;
    if (result)
    {
        Float32 * out = reinterpret_cast<Float32 *>(outbuffer);
        signed char * in = reinterpret_cast<signed char *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float32>(in[m_inoffset[j]]) / 128.0;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float32_from_sint16
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint16;
    if (result)
    {
        Float32 * out = reinterpret_cast<Float32 *>(outbuffer);
        Int16 * in = reinterpret_cast<Int16 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float32>(in[m_inoffset[j]]) / 32768.0;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float32_from_sint24
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint24;
    if (result)
    {
        Float32 * out = reinterpret_cast<Float32 *>(outbuffer);
        Int24 * in = reinterpret_cast<Int24 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float32>(in[m_inoffset[j]]) / 8388608.0;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float32_from_sint32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint32;
    if (result)
    {
        Float32 * out = reinterpret_cast<Float32 *>(outbuffer);
        Int32 * in = reinterpret_cast<Int32 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Float32>(in[m_inoffset[j]]) / 2147483648.0;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::float32_from_float32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float32;
    if (result)
    {
        Float32 * out = reinterpret_cast<Float32 *>(outbuffer);
        Float32 * in = reinterpret_cast<Float32 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
                out[m_outoffset[j]] = in[m_inoffset[j]];

            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::float32_from_float64
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float64;
    if (result)
    {
        Float32 * out = reinterpret_cast<Float32 *>(outbuffer);
        Float64 * in = reinterpret_cast<Float64 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
                out[m_outoffset[j]] =
                    static_cast<Float32>(in[m_inoffset[j]]);

            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint32_from_sint8
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint8;
    if (result)
    {
        Int32 * out = reinterpret_cast<Int32 *>(outbuffer);
        char * in = inbuffer;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] = static_cast<Int32>(in[m_inoffset[j]]);
                out[m_outoffset[j]] <<= 24;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint32_from_sint16
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint16;
    if (result)
    {
        Int32 * out = reinterpret_cast<Int32 *>(outbuffer);
        Int16 * in = reinterpret_cast<Int16 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] = static_cast<Int32>(in[m_inoffset[j]]);
                out[m_outoffset[j]] <<= 16;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint32_from_sint24
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint24;
    if (result)
    {
        Int32 * out = reinterpret_cast<Int32 *>(outbuffer);
        Int24 * in = reinterpret_cast<Int24 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] = static_cast<Int32>(in[m_inoffset[j]]);
                out[m_outoffset[j]] <<= 8;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::sint32_from_sint32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint32;
    if (result)
    {
        Int32 * out = reinterpret_cast<Int32 *>(outbuffer);
        Int32 * in = reinterpret_cast<Int32 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
                out[m_outoffset[j]] = in[m_inoffset[j]];

            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function uses llround() which returns "long long", which is
 *  guaranteed to be at least 64 bits.
 */

bool
convert_info::sint32_from_float32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float32;
    if (result)
    {
        Int32 * out = reinterpret_cast<Int32 *>(outbuffer);
        Float32 * in = reinterpret_cast<Float32 *>(inbuffer);
        const long long cmax = 2147483647LL;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                Float32 invalue = in[m_inoffset[j]] * 2147483648.0F;
                long long outvalue = std::min(std::llround(invalue), cmax);
                out[m_outoffset[j]] = static_cast<Int32>(outvalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint32_from_float64
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float64;
    if (result)
    {
        Int32 * out = reinterpret_cast<Int32 *>(outbuffer);
        Float64 * in = reinterpret_cast<Float64 *>(inbuffer);
        const long long cmax = 2147483647LL;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                Float64 invalue = in[m_inoffset[j]] * 2147483648.0F;
                long long outvalue = std::min(std::llround(invalue), cmax);
                out[m_outoffset[j]] = static_cast<Int32>(outvalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

//----------------------------------------------

bool
convert_info::sint24_from_sint8
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint8;
    if (result)
    {
        Int24 * out = reinterpret_cast<Int24 *>(outbuffer);
        char * in = inbuffer;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                int outvalue = in[m_inoffset[j]];
                out[m_outoffset[j]] = outvalue << 16;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint24_from_sint16
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint16;
    if (result)
    {
        Int24 * out = reinterpret_cast<Int24 *>(outbuffer);
        Int16 * in = reinterpret_cast<Int16 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                int outvalue = in[m_inoffset[j]];
                out[m_outoffset[j]] = outvalue << 8;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::sint24_from_sint24
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint24;
    if (result)
    {
        Int24 * out = reinterpret_cast<Int24 *>(outbuffer);
        Int24 * in = reinterpret_cast<Int24 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
                out[m_outoffset[j]] = in[m_inoffset[j]];

            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint24_from_sint32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint32;
    if (result)
    {
        Int24 * out = reinterpret_cast<Int24 *>(outbuffer);
        Int32 * in = reinterpret_cast<Int32 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                int outvalue = in[m_inoffset[j]];
                out[m_outoffset[j]] = outvalue >> 8;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function uses llround() which returns "long long", which is
 *  guaranteed to be at least 64 bits.
 */

bool
convert_info::sint24_from_float32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float32;
    if (result)
    {
        Int24 * out = reinterpret_cast<Int24 *>(outbuffer);
        Float32 * in = reinterpret_cast<Float32 *>(inbuffer);
        const long long cmax = 8388607LL;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                Float32 invalue = in[m_inoffset[j]] * 8388608.0F;
                long long outvalue = std::min(std::llround(invalue), cmax);
                out[m_outoffset[j]] = outvalue;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint24_from_float64
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float64;
    if (result)
    {
        Int24 * out = reinterpret_cast<Int24 *>(outbuffer);
        Float64 * in = reinterpret_cast<Float64 *>(inbuffer);
        const long long cmax = 8388607LL;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                Float64 invalue = in[m_inoffset[j]] * 8388608.0F;
                long long outvalue = std::min(std::llround(invalue), cmax);
                out[m_outoffset[j]] = outvalue;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

//----------------------------------------------

bool
convert_info::sint16_from_sint8
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint8;
    if (result)
    {
        Int16 * out = reinterpret_cast<Int16 *>(outbuffer);
        signed char * in = reinterpret_cast<signed char *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] = static_cast<Int16>(in[m_inoffset[j]]);
                out[m_outoffset[j]] <<= 8;
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::sint16_from_sint16
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint16;
    if (result)
    {
        Int16 * out = reinterpret_cast<Int16 *>(outbuffer);
        Int16 * in = reinterpret_cast<Int16 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
                out[m_outoffset[j]] = in[m_inoffset[j]];

            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::sint16_from_sint24
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint16;
    if (result)
    {
        Int16 * out = reinterpret_cast<Int16 *>(outbuffer);
        Int24 * in = reinterpret_cast<Int24 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                /*Int24*/ int invalue = int(in[m_inoffset[j]]) >> 8;
                out[m_outoffset[j]] = static_cast<Int16>(invalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint16_from_sint32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint32;
    if (result)
    {
        Int16 * out = reinterpret_cast<Int16 *>(outbuffer);
        Int32 * in = reinterpret_cast<Int32 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<Int16>(in[m_inoffset[j]] >> 16 & 0x0000ffff);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function uses llround() which returns "long long", which is
 *  guaranteed to be at least 64 bits.
 */

bool
convert_info::sint16_from_float32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float32;
    if (result)
    {
        Int16 * out = reinterpret_cast<Int16 *>(outbuffer);
        Float32 * in = reinterpret_cast<Float32 *>(inbuffer);
        const long long cmax = 32767LL;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                Float32 invalue = in[m_inoffset[j]] * 32768.0F;
                long long outvalue = std::min(std::llround(invalue), cmax);
                out[m_outoffset[j]] = static_cast<Int16>(outvalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint16_from_float64
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float64;
    if (result)
    {
        Int16 * out = reinterpret_cast<Int16 *>(outbuffer);
        Float64 * in = reinterpret_cast<Float64 *>(inbuffer);
        const long long cmax = 32767LL;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                Float64 invalue = in[m_inoffset[j]] * 32768.0F;
                long long outvalue = std::min(std::llround(invalue), cmax);
                out[m_outoffset[j]] = static_cast<Int16>(outvalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

//----------------------------------------------

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::sint8_from_sint8
(
    unsigned buffsize, char * outbuffer, char * /* inbuffer */
)
{
    bool result = m_informat == stream_format::sint8;
    if (result)
    {
        signed char * out = reinterpret_cast<signed char *>(outbuffer);
        /*
         * TODO
         *
         * char * in = inbuffer;
         */
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
                out[m_outoffset[j]] = out[m_outoffset[j]];

            /*
             * TODO
             *
             * in += m_injump;
             */

            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::sint8_from_sint16
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint16;
    if (result)
    {
        signed char * out = reinterpret_cast<signed char *>(outbuffer);
        Int16 * in = reinterpret_cast<Int16 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                out[m_outoffset[j]] =
                    static_cast<unsigned char>
                    (
                        (in[m_inoffset[j]] >> 8) & 0xff
                    );
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function is use for channel compensation and/or (de)interleaving only.
 */

bool
convert_info::sint8_from_sint24
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint16;
    if (result)
    {
        char * out = outbuffer;;
        Int24 * in = reinterpret_cast<Int24 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                int invalue = (int(in[m_inoffset[j]]) >> 16) & 0xFF;
                out[m_outoffset[j]] = static_cast<char>(invalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint8_from_sint32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::sint32;
    if (result)
    {
        char * out = outbuffer;
        Int32 * in = reinterpret_cast<Int32 *>(inbuffer);
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                int invalue = (int(in[m_inoffset[j]]) >> 24) & 0xFF;
                out[m_outoffset[j]] = static_cast<char>(invalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/**
 *  This function uses llround() which returns "long long", which is
 *  guaranteed to be at least 64 bits.
 */

bool
convert_info::sint8_from_float32
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float32;
    if (result)
    {
        char * out = outbuffer;
        Float32 * in = reinterpret_cast<Float32 *>(inbuffer);
        const long long cmax = 127LL;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                Float32 invalue = in[m_inoffset[j]] * 128.0F;
                long long outvalue = std::min(std::llround(invalue), cmax);
                out[m_outoffset[j]] = static_cast<char>(outvalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

bool
convert_info::sint8_from_float64
(
    unsigned buffsize, char * outbuffer, char * inbuffer
)
{
    bool result = m_informat == stream_format::float64;
    if (result)
    {
        char * out = outbuffer;
        Float64 * in = reinterpret_cast<Float64 *>(inbuffer);
        const long long cmax = 127LL;
        for (unsigned i = 0; i < buffsize; ++i)
        {
            for (int j = 0; j < m_channels; ++j)
            {
                Float64 invalue = in[m_inoffset[j]] * 128.0F;
                long long outvalue = std::min(std::llround(invalue), cmax);
                out[m_outoffset[j]] = static_cast<char>(outvalue);
            }
            in += m_injump;
            out += m_outjump;
        }
    }
    return result;
}

/*---------------------------------------------------------------------------
 *  device_info
 *--------------------------------------------------------------------------*/

/**
 *  Default constructor
 */

device_info::device_info () :
    m_probed                (false),
    m_ID                    (invalid_id),
    m_name                  (),
    m_output_channels       (0),
    m_input_channels        (0),
    m_duplex_channels       (0),
    m_is_default_output     (false),
    m_is_default_input      (false),
    m_sample_rates          (),             // vector
    m_preferred_sample_rate (0),
    m_native_formats        (stream_format_none())
{
    // no other code
}

/*------------------------------------------------------------------------
 * api_stream
 *------------------------------------------------------------------------*/

api_stream::api_stream () :
    m_deviceid          (),             /* playback/record unsigned array   */
    m_apihandle         (nullptr),
    m_mode              (stream_mode::uninitialized),
    m_state             (stream_state::closed),
    m_userbuffer        (),             /* playback/record char * array     */
    m_devicebuffer      (nullptr),
    m_doconvertbuffer   (),             /* playback/record boolean array    */
    m_userinterleaved   (false),
    m_deviceinterleaved (),             /* playback/record boolean array    */
    m_dobyteswap        (),             /* playback/record boolean array    */
    m_samplerate        (0),
    m_buffersize        (0),
    m_nbuffers          (0),
    m_nuserchannels     (),             /* playback/record unsigned array   */
    m_ndevicechannels   (),             /* playback/record unsigned array   */
    m_channeloffset     (),             /* playback/record unsigned array   */
    m_latency           (),             /* playback/record UL array         */
    m_userformat        (stream_format::none),
    m_deviceformat      (),             /* playback/record audio_format[2]  */
    m_mutex             (),             /* handles its own init()/destroy() */
    m_callbackinfo      (),
    m_convertinfo       (),             /* playback/record array            */
    m_streamtime        (0.0)
{
    m_deviceid[0] = m_deviceid[1] = 11111;
    m_userbuffer[0] = m_userbuffer[1] = 0;
}

api_stream::~api_stream ()
{
    // _mutex destroys its own mutex handle
}

void
api_stream::clear ()
{
    m_mode = stream_mode::uninitialized;
    m_state = stream_state::closed;
    m_samplerate = m_buffersize = m_nbuffers = 0;
    m_userformat = stream_format::none;
    m_userinterleaved = true;
    m_streamtime = 0.0;
    m_apihandle = nullptr;
    m_devicebuffer = nullptr;
    m_callbackinfo.clear();
    for (int i = 0; i < 2; ++i)
    {
        m_deviceid[i] = 11111;
        m_doconvertbuffer[i] = false;
        m_deviceinterleaved[i] = true;
        m_dobyteswap[i] = false;
        m_nuserchannels[i] = m_ndevicechannels[i] = m_channeloffset[i] = 0;
        m_deviceformat[i] = stream_format::none;
        m_latency[i] = 0;
        m_userbuffer[i] = nullptr;
        m_convertinfo[i].clear();
    }
}

void
api_stream::set_convert_jump (stream_mode mode)
{
    convert_info & destination = convertinfo(mode);
    destination.set_convert_jump();
}

void
api_stream::set_convert_info_jump (stream_mode mode)
{
    convert_info & cinfo = convertinfo(mode);
    if (mode == stream_mode::input)     /* convert device to user buffer    */
    {
        cinfo.set_convert_info_jump
        (
            ndevicechannels(mode),      /* mode == 1 (input), in-jump       */
            nuserchannels(mode),        /* out-jump     */
            deviceformat(mode),         /* in-format    */
            userformat()                /* out-format   */
        );
    }
    else                                /* convert user to device buffer    */
    {
        cinfo.set_convert_info_jump
        (
            nuserchannels(mode),        /* mode == 0 (output), in-jump      */
            ndevicechannels(mode),      /* out-jump     */
            userformat(),               /* in-format    */
            deviceformat(mode)          /* out-format   */
        );
    }
}

/**
 *  Sets up the interleave/deinterleave or no-(de)interleave offsets.
 */

void
api_stream::set_deinterleaved_offsets (stream_mode mode)
{
    convert_info & cinfo = convertinfo(mode);
    unsigned smode = static_cast<unsigned>(mode);
    if (m_deviceinterleaved[smode] != m_userinterleaved)
    {
        bool doinput =
            (mode == stream_mode::output && m_deviceinterleaved[smode]) ||
            (mode == stream_mode::input && m_userinterleaved) ;

        if (doinput)
            cinfo.set_deinterleave_offsets(buffersize(), true);  // input
        else
            cinfo.set_deinterleave_offsets(buffersize(), false); // output
    }
    else
        cinfo.set_no_interleaved_offsets(buffersize(), m_userinterleaved);
}

void
api_stream::add_channel_offsets (stream_mode mode, unsigned firstchan)
{
    convert_info & cinfo = convertinfo(mode);
    unsigned offset = deviceinterleaved(mode) ?
        firstchan : firstchan * buffersize();

    cinfo.add_channel_offsets(mode, offset);

#if 0
    if (deviceinterleaved(mode))
    {
        if (mode == stream_mode::output)
        {
            for (int k = 0; k < cinfo.channels; ++k)
                cinfo.outOffset[k] += firstchan;
        }
        else
        {
            for (int k = 0; k < cinfo.channels; ++k)
                cinfo.inOffset[k] += firstchan;
        }
    }
    else
    {
        if (mode == stream_mode::output)
        {
            for (int k = 0; k < cinfo.channels; ++k)
                cinfo.outOffset[k] += (firstchan * buffersize());
        }
        else
        {
            for (int k = 0; k < cinfo.channels; ++k)
                cinfo.inOffset[k] += (firstchan * buffersize());
        }
    }
#endif
}

}           // namespace rtl

/*
 * audio_support.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

