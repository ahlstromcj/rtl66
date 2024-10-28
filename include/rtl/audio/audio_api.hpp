#if ! defined RTL66_AUDIO_API_HPP
#define RTL66_AUDIO_API_HPP

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
 * \file          audio_api.hpp
 *
 *      The base class for classes that due that actual audio work for the
 *      audio framework selected at run-time.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-07
 * \updates       2024-01-16
 * \license       See above.
 *
 *      This class is mostly similar to the original RtAudio MidiApi class,
 *      but adds some additional (and optional) features, plus a slightly
 *      simpler class hierarchy that can support both input and output in the
 *      same port.
 *
 *      Subclasses of audio_api contain all API- and OS-specific code
 *      necessary to fully implement the "rtaudio" API.
 *
 *      Note that audio_api is an abstract base class and cannot be explicitly
 *      instantiated.  The class audio_api will create an instance of an
 *      audio_api subclass (RtApiOss, RtApiAlsa, RtApiJack, RtApiCore,
 *      RtApiDs, or RtApiAsio).
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#include <string>                       /* std::string class                */
#include <thread>                       /* std::thread class                */

#if defined HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif

#include "rtl/api_base.hpp"             /* rtl::api_base class etc.         */
#include "rtl/audio/audio_support.hpp"  /* rtl::rtaudio::xxx_info classes   */
#include "rtl/audio/rtaudio.hpp"        /* rtl::rtaudio::api, etc.          */

namespace rtl
{

/**
 *  Subclasses of audio_api will contain all API- and OS-specific code
 *  necessary to fully implement the modified rtaudio API.
 *
 *  Note that audio_api is an abstract base class and cannot be explicitly
 *  instantiated.  rtaudio_in and rtaudio_out will create instances of the
 *  proper API-implementing subclass.
 */

class RTL66_DLL_PUBLIC audio_api : public api_base
{
    friend class rtaudio;

protected:

    using devicelist = std::vector<device_info>;

protected:

    static const unsigned sc_max_sample_rates;
    static const unsigned sc_sample_rates [];

private:

    devicelist m_device_list;

    unsigned m_currentdeviceid;

    api_stream m_stream;

    /*
     * Move into rterror???
     */

    bool m_show_warnings;

public:

    /*
     * The default constructor creates an output port.  The queuesize
     * parameter is meant only for input.
     */

    audio_api ();
    audio_api (const audio_api &) = default;                // delete;
    audio_api & operator = (audio_api &&) = delete;
    audio_api & operator = (const audio_api &) = default;   // delete;
    virtual ~audio_api () = default;

protected:

    devicelist & device_list ()
    {
        return m_device_list;
    }

    const devicelist & device_list () const
    {
        return m_device_list;
    }

    /**
     * Protected, API-specific methods that attempt to open a device
     * with the given parameters.  These functions must be implemented by
     * all subclasses.  If an error is encountered during the probe, a
     * "warning" message is reported and disposition::failure is returned. A
     * successful probe is indicated by a return value of disposition::success.
     */

    virtual bool probe_devices () = 0;
    virtual bool probe_device_open
    (
        unsigned device,
        stream_mode mode,                               /* I/O or duplex    */
        unsigned channels,
        unsigned firstchannel, unsigned samplerate,
        stream_format format, unsigned * buffersize,
        stream_options * options
    ) = 0;

    virtual bool open_stream
    (
        stream_parameters * outparameters,
        stream_parameters * inparameters,
        stream_format format,
        unsigned samplerate,
        unsigned int bufferframes,
        callback_t cb,
        void * userdata,
        stream_options * options,
        rterror::callback_t errorcb
    );

    void tick_stream_time ();           /* increments the stream time       */

    void clear_stream_info ()           /* clears api_stream structure      */
    {
        m_stream.clear();
    }

    /**
     *  Common method that throws rterror::invalid_use if a stream
     *  is not open. Turns out to be unimplemented in the RtAudio library.
     *
     *      void verify_stream ();
     */

    /**
     *  Performs format, channel number, or interleaving conversions between
     *  the user and device buffers.
     */

    void convert_buffer (char * outBuffer, char * inBuffer, convert_info & info);

    /**
     *  Common method used to perform byte-swapping on buffers.
     */

    void byte_swap_buffer (char * buff, unsigned samples, stream_format format);

    /**
     *  Common method that returns the number of bytes for a given format.
     */

    unsigned format_bytes (stream_format format);

    /**
     *  Common method that sets up the parameters for buffer conversion.
     */

    void set_convert_info (stream_mode mode, unsigned firstchannel);

    virtual rtaudio::api get_current_api ()      // must override
    {
        return rtaudio::api::unspecified;        // rtapi_->get_current_api();
    }

    virtual unsigned get_default_input_device ();
    virtual unsigned get_default_output_device ();

    /**
     *  A function that closes a stream and frees any associated stream
     *  memory.
     *
     * If a stream is not open, an rterror::kind::warning will be passed
     * to the user-provided error-callback function (or otherwise printed to
     * stderr). If an error occurs, the rterror::kind::error is used.
     *
     *  This function must be overridden.
     *
     *  The other "stream" functions operate similarly.
     */

    virtual bool close_stream () = 0;
    virtual bool start_stream () = 0;   /* implementations have common code */
    virtual bool stop_stream () = 0;    /* implementations have common code */
    virtual bool abort_stream () = 0;   /* implementations have common code */

    virtual double get_stream_time () const
    {
        return m_stream.streamtime();
    }

    virtual void set_stream_time (double t)
    {
        if (t >= 0.0)
            m_stream.streamtime(t);
    }

    /*
     * These functions don't need to be virtual as they call virtual
     * functions to do the work.
     */

    unsigned get_device_count ();
    device_info get_device_info (unsigned deviceid);

    bool is_stream_open () const
    {
        return m_stream.state() != stream_state::closed;
    }

    bool is_stream_running () const
    {
        return m_stream.state() == stream_state::running;
    }

    long get_stream_latency ();

    unsigned get_stream_sample_rate () const
    {
        return is_stream_open() ? m_stream.samplerate() : 0 ;
    }

private:

    bool stream_mode_is_input ()
    {
        return m_stream.mode() == stream_mode::input ||
            m_stream.mode() == stream_mode::duplex;
    }

    bool stream_mode_is_output ()
    {
        return m_stream.mode() == stream_mode::output ||
            m_stream.mode() == stream_mode::duplex;
    }

};          // class audio_api

}           // namespace rtl

#endif      // RTL66_AUDIO_API_HPP

/*
 * audio_api.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

