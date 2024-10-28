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
 * \file          rtmidi_in.cpp
 *
 *    A reworking of RtMidi.cpp, with the same functionality but different
 *    coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-06-07
 * \license       See above.
 *
 */

#include "midi/ports.hpp"                   /* midi::ports class            */
#include "rtl/midi/find_midi_api.hpp"       /* rtl::try_open_midi_api()     */
#include "rtl/midi/rtmidi_in.hpp"           /* rtl::rtmidi_in class, etc.   */

namespace rtl
{

/**
 *  Default constructor that allows an optional API, client name and queue
 *  size.
 *
 *  An exception will be thrown if a MIDI system initialization error occurs.
 *  The queue size defines the maximum number of messages that can be held in
 *  the MIDI queue (when not using a callback function).  If the queue size
 *  limit is reached, incoming messages will be ignored.
 *
 *  If no API argument is specified and multiple API support has been
 *  compiled, the default order of use is ALSA, JACK (Linux) and CORE, JACK
 *  (OS-X).
 *
 *  It should be impossible to not find an API because the preprocessor
 *  definition RTL66_BUILD_DUMMY is automatically defined if no API-specific
 *  definitions are passed to the compiler. But just in case something weird
 *  happens, we'll throw an error.
 *
 *  \param rapi
 *      An optional API id can be specified.  The default is
 *      rtl::rtmidi::api::unspecified.
 *
 *  \param clientname
 *      An optional client name can be specified. This will be used to group
 *      the ports that are created by the application.
 *
 *  \param qsizelimit
 *      An optional size of the MIDI input queue can be specified. The default
 *      is 100.
 */

RTL66_DLL_PUBLIC
rtmidi_in::rtmidi_in
(
    rtmidi::api rapi,
    const std::string & clientname,
    unsigned qsize
) :
    rtmidi  ()
{
    if (qsize == 0)
        qsize = RTL66_DEFAULT_Q_SIZE;

    rapi = ctor_common_setup(rapi, clientname);
    if (is_midiapi_valid(rapi))
    {
        if (open_midi_api(rapi, clientname, qsize))
            rtmidi::selected_api(rapi);
    }
}

/**
 *  If a MIDI connection is still open, it will be closed by the destructor.
 */

rtmidi_in::~rtmidi_in () noexcept
{
    // No code needed
}

/**
 *  This function accesses only the supported rtl::rtmidi::api values, and
 *  creates a new "midi_in_xxx" object only if a match is found. Note that, under
 *  Linux, JACK is tried first.
 *
 *  For Linux, there is a fallback process.  If there is no API specified,
 *  then the APIs are tried in the following order:
 *
 *      -   PipeWire (TODO! Currently it is a dummy implementation.)
 *      -   JACK
 *      -   ALSA
 */

bool
rtmidi_in::open_midi_api
(
    rtmidi::api rapi,
    const std::string & clientname,
    unsigned qsize
)
{
    bool result = rapi != rtmidi::api::max;
    delete_rt_api_ptr();                    /* remove and nullify pointer   */
    if (result)
    {
        rt_api_ptr
        (
            try_open_midi_api(rapi, midi::port::io::input, clientname, qsize)
        );
        result = not_nullptr(rt_api_ptr());
    }
    return result;
}

/**
 *  Open a MIDI input connection given by enumeration number.
 *
 * \param portnumber
 *      An optional port number greater than 0 can be specified.
 *      Otherwise, the default or first port found is opened.
 *
 * \param portname
 *      An optional name for the application port that is used to connect
 *      to portId can be specified.
 */

bool
rtmidi_in::open_port (int portnumber, const std::string & portname)
{
    std::string pname = portname;
    if (pname.empty())
        pname = "rtl66 midi in";

    return rtmidi::open_port(portnumber, pname);
}

/**
 *  Create a virtual input port, with optional name, to allow software
 *  connections (OS X, JACK and ALSA only).  This function creates a virtual
 *  MIDI input port to which other software applications can connect.  This
 *  type of functionality is currently only supported by the Macintosh OS-X,
 *  any JACK, and Linux ALSA APIs (the function returns an error for the
 *  other APIs).
 *
 *  TODO:  Add an overload containing a port (index) number.
 *
 * \param portname
 *      An optional name for the application port that is used to connect to
 *      portId can be specified.
 */

bool
rtmidi_in::open_virtual_port (const std::string & portname)
{
    std::string pname = portname;
    if (pname.empty())
        pname = "rtl66 midi vin";

    return rtmidi::open_virtual_port(pname);
}

/**
 *  Set a callback function to be invoked for incoming MIDI messages.  The
 *  callback function will be called whenever an incoming MIDI message is
 *  received.  While not absolutely necessary, it is best to set the callback
 *  function before opening a MIDI port to avoid leaving some messages in the
 *  queue.
 *
 *  Note that this function is not virtual.
 *
 * \param cb
 *      A callback function must be given.
 *
 * \param userdata
 *      Optionally, a pointer to additional data can be passed to the
 *      callback function whenever it is called.
 */

void
rtmidi_in::set_input_callback (rtmidi_in_data::callback_t cb, void * userdata)
{
    if (not_nullptr(rt_api_ptr()))
        rt_api_ptr()->set_input_callback(cb, userdata);
}

/**
 *  Cancel use of the current callback function (if one exists).
 *  Subsequent incoming MIDI messages will be written to the queue
 *  and can be retrieved with the \e get_message function.
 */

void
rtmidi_in::cancel_input_callback ()
{
    if (not_nullptr(rt_api_ptr()))
        rt_api_ptr()->cancel_input_callback();
}

/**
 *  Specify whether certain MIDI message types should be queued or ignored
 *  during input.
 *
 *  By default, MIDI timing and active sensing messages are ignored
 *  during message input because of their relative high data rates.  MIDI sysex
 *  messages are ignored by default as well.  Variable values of "true" imply
 *  that the respective message type will be ignored.
 */

void
rtmidi_in::ignore_midi_types (bool midisysex, bool miditime, bool midisense)
{
    rt_api_ptr()->ignore_midi_types(midisysex, miditime, midisense);
}

/**
 *  Fill the user-provided vector with the data bytes for the next available
 *  MIDI message in the input queue and return the event delta-time in seconds.
 *
 *  This function returns immediately whether a new message is available or
 *  not.  A valid message is indicated by a non-zero vector size.  An exception
 *  is thrown if an error occurs during message retrieval or an input
 *  connection was not previously established.
 */

double
rtmidi_in::get_message (midi::message & message)
{
    return static_cast<midi_api *>(rt_api_ptr())->get_message(message);
}

/**
 *  Set maximum expected incoming message size.
 *
 *  For APIs that require manual buffer management, it can be useful to set the
 *  buffer size and buffer count when expecting to receive large SysEx
 *  messages.  Note that currently this function has no effect when called
 *  after open_port().  The default buffer size is 1024 with a count of 4
 *  buffers, which should be sufficient for most cases; as mentioned, this does
 *  not affect all API backends, since most either support dynamically scalable
 *  buffers or take care of buffer handling themselves.  It is principally
 *  intended for users of the Windows MM backend who must support receiving
 *  especially large messages.
 */

void
rtmidi_in::set_buffer_size (size_t sz, int count)
{
    rt_api_ptr()->set_buffer_size(sz, count);
}

}           // namespace rtl

/*
 * rtmidi_in.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

