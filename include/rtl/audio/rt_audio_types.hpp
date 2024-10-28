#if ! defined RTL66_AUDIO_TYPES_HPP
#define RTL66_AUDIO_TYPES_HPP

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
 * \file          rt_audio_types.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2023-03-17
 * \license       See above.
 *
 *  The types in here are simple.  See the audio_support module for more
 *  extensive, structural types.
 */

#include <vector>                           /* std::vector container        */

namespace rtl
{

/**
 *  Support for signed integers and floats.  Audio data fed to/from an
 *  rtaudio stream is assumed to ALWAYS be in host byte order.  The
 *  internal routines will automatically take care of any necessary
 *  byte-swapping between the host format and the soundcard.  Thus,
 *  endian-ness is not a concern in the following format definitions.
 *
 */

enum class stream_format : unsigned long
{
    none    = 0x00,             /**< 8-bit unsigned, default/unuseable      */
    sint8   = 0x01,             /**< 8-bit signed integer                   */
    sint16  = 0x02,             /**< 16-bit signed integer                  */
    sint24  = 0x04,             /**< 24-bit signed integer                  */
    sint32  = 0x08,             /**< 32-bit signed integer                  */
    float32 = 0x10,             /**< Normalized between plus/minus 1.0      */
    float64 = 0x20              /**< Normalized between plus/minus 1.0      */
};

/**
 *  Provides a way to encode multiple stream_format values.
 */

using stream_formats = unsigned;

inline stream_formats
stream_format_none ()
{
    return static_cast<stream_formats>(stream_format::none);
}

inline stream_formats
add (stream_formats current, stream_format f)
{
    stream_formats result = current | static_cast<unsigned>(f);
    return result;
}

inline bool
test (stream_formats current, stream_format f)
{
    return (current & static_cast<unsigned>(f)) != 0;
}

/**
 *  Rtaudio stream option flags.  The following flags can be OR'ed together to
 *  allow a client to make changes to the default stream behavior:
 *
 *  - noninterleaved:    Use non-interleaved buffers (default = interleaved).
 *  - minimize_latency:  Attempt to set stream parameters for the lowest
 *                       possible latency.
 *  - hog_device:        Attempt grab device for exclusive use.
 *  - alsa_use_default:  Use the "default" PCM device (ALSA only).
 *  - jack_dont_connect: Do not automatically connect ports (JACK only).
 *
 *  By default, rtaudio streams pass and receive audio data from the client in
 *  an interleaved Format.  By passing the noninterleaved flag to the
 *  openStream() function, audio data will instead be presented in
 *  non-interleaved buffers.  In this case, each buffer argument in the
 *  rtaudio callback function will point to a single array of data, with
 *  nframes samples for each channel concatenated back-to-back.  For example,
 *  the first sample of data for the second channel would be located at index
 *  nFrames (assuming the \c buffer pointer was recast to the correct data type
 *  for the stream).
 *
 *  Certain audio APIs offer a number of parameters that influence the I/O
 *  latency of a stream.  By default, rtaudio will attempt to set these
 *  parameters internally for robust (glitch-free) performance (though some
 *  APIs, like Windows DirectSound, make this difficult).  By passing the
 *  minimize_latency flag to the openStream() function, internal stream
 *  settings will be influenced in an attempt to minimize stream latency, though
 *  possibly at the expense of stream performance.
 *
 *  If the hog_device flag is set, rtaudio will attempt to open the
 *  input and/or output stream device(s) for exclusive use.  Note that this is
 *  not possible with all supported audio APIs.
 *
 *  If the schedule_realtime flag is set, rtaudio will attempt to select
 *  realtime scheduling (round-robin) for the callback thread.
 *
 *  If the alsa_use_default flag is set, rtaudio will attempt to open
 *  the "default" PCM device when using the ALSA API. Note that this will
 *  override any specified input or output device id.
 *
 *  If the jack_dont_connecT flag is set, rtaudio will not attempt to
 *  automatically connect the ports of the client to the audio device.
 */

enum class stream_flags : unsigned
{
    none                = 0x00,
    noninterleaved      = 0x01,
    minimize_latency    = 0x02,
    hog_device          = 0x04,
    schedule_realtime   = 0x08,
    alsa_use_default    = 0x10,
    jack_dont_connect   = 0x20
};

/**
 *  rtaudio stream status (over/underflow) flags.
 *  Notification of a stream over- or underflow is indicated by a
 *  non-zero stream \c status argument in the rtaudioCallback function.
 *  The stream status can be one of the following two options,
 *  depending on whether the stream is open for output and/or input:
 *
 *  - input_overflow:   Input data was discarded because of an overflow
 *                      condition at the driver.
 *  - output_underflow: The output buffer ran low, likely producing a break
 *                      in the output sound.
 */

enum class stream_status : int   // unsigned long!?
{
    input_overflow      = 0x01,
    output_underflow    = 0x02
};

enum class stream_disposition
{
    failure,
    success
};

enum class stream_state
{
    stopped,
    stopping,
    running,
    closed = -50
};

enum class stream_mode
{
    output,
    input,
    duplex,
    uninitialized = -75
};

enum audio_mode
{
    audio_mode_playback = 0,
    audio_mode_record   = 1
};

enum class callback_result
{
    normal,
    stop,
    abort
};

/**
 *  All rtaudio clients must create a function of type rtaudioCallback
 *  to read and/or write data from/to the audio stream.  When the
 *  underlying audio system is ready for new input or output data, this
 *  function will be invoked.
 *
 * \param outputBuffer
 *      For output (or duplex) streams, the client
 *      should write \c nFrames of audio sample frames into this
 *      buffer.  This argument should be recast to the datatype
 *      specified when the stream was opened.  For input-only
 *      streams, this argument will be NULL.
 *
 * \param inputbuffer
 *      For input (or duplex) streams, this buffer will
 *      hold \c nFrames of input audio sample frames.  This
 *      argument should be recast to the datatype specified when the
 *      stream was opened.  For output-only streams, this argument
 *      will be NULL.
 *
 * \param nframes
 *      The number of sample frames of input or output
 *      data in the buffers.  The actual buffer size in bytes is
 *      dependent on the data type and number of channels in use.
 *
 * \param streamtime
 *      The number of seconds that have elapsed since the stream was started.
 *
 * \param status
 *      If non-zero, this argument indicates a data overflow
 *      or underflow condition for the stream.  The particular
 *      condition can be determined by comparison with the
 *      rtaudioStreamStatus flags.
 *
 * \param userdata
 *      A pointer to optional data provided by the client
 *      when opening the stream (default = NULL).
 *
 * \return
 *      To continue normal stream operation, the rtaudio callback function
 *      should return a value of zero.  To stop the stream and drain the
 *      output buffer, the function should return a value of one.  To abort
 *      the stream immediately, the client should return a value of two.
 *
 *      callback_results: normal, stop, abort
 */

using callback_t = int (*)
(
    void * outputbuffer, void * inputbuffer,
    int nframes, double streamtime,
    stream_status status, void * userdata
);

}           // namespace rtl

#endif      // RTL66_AUDIO_TYPES_HPP

/*
 * rt_audio_types.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

