#if ! defined RTL66_AUDIO_SUPPORT_HPP
#define RTL66_AUDIO_SUPPORT_HPP

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
 * \file          audio_support.hpp
 *
 *      This module puts all the various audio-related structure in one place
 *      in the rtl::audio namespace.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-10
 * \updates       2024-01-16
 * \license       See above.
 *
 * Declarations:
 *
 *      -   class stream_parameters
 *      -   class stream_options
 *      -   class S24
 *      -   class callback_info
 *      -   class convert_info
 *      -   class device_info
 *      -   class api_stream
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#include <array>                        /* std::array class                 */
#include <string>                       /* std::string class                */
#include <thread>                       /* std::thread class                */

#include "rtl/audio/rt_audio_types.hpp" /* rtl::audio simple types          */
#include "xpc/recmutex.hpp"             /* xpc::recmutex class              */

namespace rtl
{

/**
 *  RtAudio type aliases.  Also see the Int24 alias defined after the definition
 *  of the S24 class.
 */

using Int16 = short;
using Int32 = int;
using Float32 = float;
using Float64 = double;

/*------------------------------------------------------------------------
 * S24 data type
 *------------------------------------------------------------------------*/

#pragma pack(push, 1)

class RTL66_DLL_PUBLIC S24
{

protected:

    unsigned char m_c3[3];

public:

    S24 ();
    S24 & operator = (const int & i);

    S24 (const double & d)
    {
        *this = int(d);
    }

    S24 (const float & f)
    {
        *this = int(f);
    }

    S24 (short s)
    {
        *this = int(s);
    }

    S24 (char c)
    {
        *this = int(c);
    }

    /*
     * Cast operators. Convert S24 to a Float64 or Float32.
     */

    explicit operator Float64 ()
    {
        return as_float64();
    }

    explicit operator Float32 ()
    {
        return as_float32();
    }

    explicit operator Int32 ()
    {
        return as_int32();
    }

private:

    int as_int () const;

    Float64 as_float64 () const
    {
        return static_cast<Float64>(as_int());
    }

    Float32 as_float32 () const
    {
        return static_cast<Float64>(as_int());
    }

    Int32 as_int32 () const
    {
        return static_cast<Int32>(as_int());
    }

};          // class S24

/**
 *  Alias the class above as a 24-bit integer.
 */

using Int24 = S24;

/*------------------------------------------------------------------------
 * Moved from class rtaudio to here.
 *------------------------------------------------------------------------*/

/**
 *  The structure for specifying input or output stream parameters.
 */

class stream_parameters
{
private:

    unsigned m_deviceid;        /**< Device index (0 to device count - 1).  */
    unsigned m_nchannels;       /**< Number of channels.                    */
    unsigned m_firstchannel;    /**< First channel index on device [0].     */

public:

    stream_parameters () = default;

    unsigned deviceid () const
    {
        return m_deviceid;
    }

    unsigned nchannels () const
    {
        return m_nchannels;
    }

    unsigned firstchannel () const
    {
        return m_firstchannel;
    }

};

/**
 *  The structure for specifying input or output stream options.
 */

class stream_options
{
private:

    stream_flags m_flags;       /**< See definition in rt_audio_types.hpp   */
    unsigned m_numberofbuffers;
    std::string m_streamname;   /**< Stream name (used only in JACK).       */
    int m_priority;             /**< Realtime scheduling cb priority.       */

public:

    stream_options ();

    stream_flags flags () const
    {
        return m_flags;
    }

    unsigned numberofbuffers () const
    {
        return m_numberofbuffers;
    }

    void numberofbuffers (unsigned nb)
    {
        m_numberofbuffers = nb;
    }

    const std::string & streamname () const
    {
        return m_streamname;
    }

    int priority () const
    {
        return m_priority;
    }
};

/*------------------------------------------------------------------------
 * callback_info
 *------------------------------------------------------------------------*/

/**
 *  This global structure type is used to pass callback information
 *  between the private audio_stream structure and global callback
 *  handling functions.
 */

class callback_info
{

// friends....

private:

    void * m_object;     // Used as a "this" pointer.

    /*
     * Uncopyable/unmovable and why do we need it hear?
     *
     * std::thread m_thread;
     */

    void * m_callback;
    void * m_userdata;
    void * m_errorcallback;
    void * m_apiinfo;    // void pointer for API specific callback information
    bool m_isrunning;    // {false};
    bool m_dorealtime;   // {false};
    int m_priority;
    bool m_devicedisconnected;

public:

    callback_info ();
    callback_info (callback_info &&) = delete;
    callback_info (const callback_info &);
    callback_info & operator = (callback_info &&) = delete;
    callback_info & operator = (const callback_info &);
    ~callback_info ();

    void clear ();
    void set_callbacks (void * callback, void * userdata, void * errorcb);

    void * callback ()
    {
        return m_callback;
    }

    void callback (void * cb)
    {
        m_callback = cb;
    }

    void * userdata ()
    {
        return m_userdata;
    }

    void userdata (void * cb)
    {
        m_userdata = cb;
    }

    bool isrunning () const
    {
        return m_isrunning;
    }

    void devicedisconnected (bool flag)
    {
        m_devicedisconnected = flag;
    }

    bool devicedisconnected () const
    {
        return m_devicedisconnected;
    }

    void isrunning (bool flag)
    {
        m_isrunning = flag;
    }
};          // class callback_info

#pragma pack(pop)

/*------------------------------------------------------------------------
 * convert_info
 *------------------------------------------------------------------------*/

/**
 *  A structure used for buffer conversion.
 *
 */

class convert_info
{

private:

    int m_channels;
    int m_injump;
    int m_outjump;
    stream_format m_informat;
    stream_format m_outformat;
    std::vector<int> m_inoffset;
    std::vector<int> m_outoffset;

public:

    convert_info ();
    ~convert_info () = default;

    int channels () const
    {
        return m_channels;
    }

    int injump () const
    {
        return m_injump;
    }

    int outjump () const
    {
        return m_outjump;
    }

    stream_format informat () const
    {
        return m_informat;
    }

    stream_format outformat () const
    {
        return m_outformat;
    }

    const std::vector<int> & inoffset () const
    {
        return m_inoffset;
    }

    const std::vector<int> & outoffset () const
    {
        return m_outoffset;
    }

    void clear ();
    void set_convert_info_jump
    (
        unsigned injump, unsigned outjump,
        stream_format informat, stream_format outformat
    );
    void set_convert_jump ();
    void set_deinterleave_offsets (unsigned buffersize, bool input);
    void set_no_interleaved_offsets (unsigned buffersize, bool userinterleaved);
    void add_channel_offsets (stream_mode mode, unsigned amount);

    bool float64_from_sint8 (unsigned sz, char * out, char * in);
    bool float64_from_sint16 (unsigned sz, char * out, char * in);
    bool float64_from_sint24 (unsigned sz, char * out, char * in);
    bool float64_from_sint32 (unsigned sz, char * out, char * in);
    bool float64_from_float32 (unsigned sz, char * out, char * in);
    bool float64_from_float64 (unsigned sz, char * out, char * in);

    bool float32_from_sint8 (unsigned sz, char * out, char * in);
    bool float32_from_sint16 (unsigned sz, char * out, char * in);
    bool float32_from_sint24 (unsigned sz, char * out, char * in);
    bool float32_from_sint32 (unsigned sz, char * out, char * in);
    bool float32_from_float32 (unsigned sz, char * out, char * in);
    bool float32_from_float64 (unsigned sz, char * out, char * in);

    bool sint32_from_sint8 (unsigned sz, char * out, char * in);
    bool sint32_from_sint16 (unsigned sz, char * out, char * in);
    bool sint32_from_sint24 (unsigned sz, char * out, char * in);
    bool sint32_from_sint32 (unsigned sz, char * out, char * in);
    bool sint32_from_float32 (unsigned sz, char * out, char * in);
    bool sint32_from_float64 (unsigned sz, char * out, char * in);

    bool sint24_from_sint8 (unsigned sz, char * out, char * in);
    bool sint24_from_sint16 (unsigned sz, char * out, char * in);
    bool sint24_from_sint24 (unsigned sz, char * out, char * in);
    bool sint24_from_sint32 (unsigned sz, char * out, char * in);
    bool sint24_from_float32 (unsigned sz, char * out, char * in);
    bool sint24_from_float64 (unsigned sz, char * out, char * in);

    bool sint16_from_sint8 (unsigned sz, char * out, char * in);
    bool sint16_from_sint16 (unsigned sz, char * out, char * in);
    bool sint16_from_sint24 (unsigned sz, char * out, char * in);
    bool sint16_from_sint32 (unsigned sz, char * out, char * in);
    bool sint16_from_float32 (unsigned sz, char * out, char * in);
    bool sint16_from_float64 (unsigned sz, char * out, char * in);

    bool sint8_from_sint8 (unsigned sz, char * out, char * in);
    bool sint8_from_sint16 (unsigned sz, char * out, char * in);
    bool sint8_from_sint24 (unsigned sz, char * out, char * in);
    bool sint8_from_sint32 (unsigned sz, char * out, char * in);
    bool sint8_from_float32 (unsigned sz, char * out, char * in);
    bool sint8_from_float64 (unsigned sz, char * out, char * in);

};          // class convert_info

/*------------------------------------------------------------------------
 * device_info
 *------------------------------------------------------------------------*/

/**
 *  The public device information structure for returning queried values.
 */

class device_info
{

public:

    static const unsigned invalid_id = 0;

private:

    /**
     *  true if the device capabilities were successfully probed.
     */

    bool m_probed;

    /**
     *  Device ID.  If 0, this is an invalid device ID.
     */

    unsigned m_ID;

    /**
     *  Character string device identifier.
     */

    std::string m_name;

    /**
     *  Maximum output channels supported by device.
     */

    unsigned m_output_channels;

    /**
     *  Maximum input channels supported by device.
     */

    unsigned m_input_channels;

    /**
     *  Maximum simultaneous input/output channels supported by device
     */

    unsigned m_duplex_channels;

    /**
     *  true if this is the default output device.
     */

    bool m_is_default_output;

    /**
     *  true if this is the default input device.
     */

    bool m_is_default_input;

    /**
     *  Supported sample rates (queried from list of standard rates).
     */

    std::vector<unsigned> m_sample_rates;

    /**
     *  Preferred sample rate, e.g. for WASAPI the system sample rate.
     */

    unsigned m_preferred_sample_rate;

    /**
     *  An unsigned it mask of supported data formats.
     */

    stream_formats m_native_formats;

public:

    device_info ();
    ~device_info () = default;

    bool probed () const
    {
        return m_probed;
    }

    unsigned ID () const
    {
        return m_ID;
    }

    bool invalid () const
    {
        return m_ID == 0;
    }

    std::string name () const
    {
        return m_name;
    }

    unsigned output_channels () const
    {
        return m_output_channels;
    }

    void output_channels (unsigned channels)
    {
        m_output_channels = channels;
    }

    unsigned input_channels () const
    {
        return m_input_channels;
    }

    void input_channels (unsigned channels)
    {
        m_input_channels = channels;
    }

    unsigned duplex_channels () const
    {
        return m_duplex_channels;
    }

    bool is_default_output () const
    {
        return m_is_default_output;
    }

    void is_default_output (bool flag)
    {
        m_is_default_output = flag;
    }

    bool is_default_input () const
    {
        return m_is_default_input;
    }

    void is_default_input (bool flag)
    {
        m_is_default_input = flag;
    }

    std::vector<unsigned> & sample_rates ()
    {
        return m_sample_rates;
    }

    const std::vector<unsigned> & sample_rates () const
    {
        return m_sample_rates;
    }

    unsigned preferred_sample_rate ()
    {
        return m_preferred_sample_rate;
    }

    stream_formats native_formats () const
    {
        return m_native_formats;
    }

    void clear_native_formats ()
    {
        m_native_formats = stream_format_none();
    }

    stream_formats add_format (stream_format f)
    {
        return add(m_native_formats, f);
    }

    bool test_format (stream_format f)
    {
        return test(m_native_formats, f);
    }

};          // class device_info

/**
 *  A protected structure for audio streams.
 */

class api_stream
{

public:

    static const int playback   = 0;
    static const int record     = 1;

private:

    std::array<unsigned, 2> m_deviceid;         /* playback, record         */
    void * m_apihandle;                         /* API's stream information */
    stream_mode m_mode;                         /* output, input, duplex    */
    stream_state m_state;                       /* stopped, running, closed */
    std::array<char *, 2> m_userbuffer;         /* playback/record          */
    char * m_devicebuffer;
    std::array<bool, 2> m_doconvertbuffer;      /* playback/record          */
    bool m_userinterleaved;
    std::array<bool, 2> m_deviceinterleaved;    /* playback/record          */
    std::array<bool, 2> m_dobyteswap;           /* playback/record          */
    unsigned m_samplerate;
    unsigned m_buffersize;
    unsigned m_nbuffers;
    std::array<unsigned, 2> m_nuserchannels;    /* playback/record          */
    std::array<unsigned, 2> m_ndevicechannels;  /* playback/record channels */
    std::array<unsigned, 2> m_channeloffset;    /* playback/record          */
    std::array<unsigned long, 2> m_latency;     /* playback/record          */
    stream_format m_userformat;
    std::array<stream_format, 2> m_deviceformat; /* playback/record         */
    xpc::recmutex m_mutex;                      /* w/work-around copy c'tor */
    callback_info m_callbackinfo;
    std::array<convert_info, 2> m_convertinfo;  /* playback/record          */
    double m_streamtime;                        /* elapsed secs since start */

public:

    api_stream ();
    ~api_stream ();
    api_stream (api_stream &&) = delete;
    api_stream (const api_stream &) = default;
    api_stream & operator = (api_stream &&) = delete;
    api_stream & operator = (const api_stream &) = default;

    void clear();

    stream_mode mode () const
    {
        return m_mode;
    }

    void mode (stream_mode m)
    {
        m_mode = m;
    }

    stream_state state () const
    {
        return m_state;               // stopped, running, or closed
    }

    void state (stream_state ss)
    {
        m_state = ss;
    }

    unsigned nbuffers () const
    {
        return m_nbuffers;
    }

    double streamtime () const
    {
        return m_streamtime;
    }

    void streamtime (double t)
    {
        m_streamtime = t;
    }

    unsigned samplerate () const
    {
        return m_samplerate;
    }

    char * devicebuffer ()
    {
        return m_devicebuffer;
    }

    unsigned buffersize () const
    {
        return m_buffersize;
    }

    /**
     * UGH!
     */

    const std::array<unsigned long, 2> & latency () const
    {
        return m_latency;               /* reference to small array */
    }

    callback_info & callbackinfo ()
    {
        return m_callbackinfo;
    }

    convert_info & convertinfo (stream_mode strmode)
    {
        static convert_info s_dummy;
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_convertinfo[mode] : s_dummy ;
    }

    const convert_info & convertinfo (stream_mode strmode) const
    {
        static convert_info s_dummy;
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_convertinfo[mode] : s_dummy ;
    }

/*
    void set_convert_info
    (
        stream_mode mode,
        unsigned injump, unsigned outjump,
        stream_format informat, format outformat
    );
    */

    void set_convert_jump (stream_mode mode);
    void set_convert_info_jump (stream_mode mode);
    void set_deinterleaved_offsets (stream_mode mode);
    void add_channel_offsets (stream_mode mode, unsigned firstchan);

    unsigned deviceid (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_deviceid[mode] : device_info::invalid_id ;
    }

    char * userbuffer (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_userbuffer[mode] : nullptr ;
    }

    bool doconvertbuffer (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_doconvertbuffer[mode] : false ;
    }

    bool deviceinterleaved (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_deviceinterleaved[mode] : false ;
    }

    bool dobyteswap (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_dobyteswap[mode] : false ;
    }

    unsigned nuserchannels (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_nuserchannels[mode] : 0 ;
    }

    unsigned ndevicechannels (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_ndevicechannels[mode] : 0 ;
    }

    unsigned channeloffset (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_channeloffset[mode] : 0 ;
    }

    unsigned long latency (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_latency[mode] : 0 ;
    }

    stream_format deviceformat (stream_mode strmode)
    {
        int mode = static_cast<int>(strmode);
        return mode <= 1 ?  m_deviceformat[mode] : stream_format::none ;
    }

    stream_format userformat () const
    {
        return m_userformat;
    }

};          // class api_stream

}           // namespace rtl

#endif      // RTL66_AUDIO_SUPPORT_HPP

/*
 * audio_support.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

