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
 * \file          midi_web_midi.cpp
 *
 *    An implementation of the Web MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-03-06
 * \license       See above.
 *
 *  Written primarily by Atsushi Eno, February 2020.
 */

#include "rtl/midi/webmidi/midi_web_midi.hpp"   /* rtl::midi_web_midi class */

#if defined RTL66_BUILD_WEB_MIDI

/**
 *  Requires (in Debian/Ubuntu) the emscripten package to be installed.
 *  From https://emscripten.org/:
 *
 *      "Compile your existing projects written in C or C++ — or any
 *      language that uses LLVM — to browsers, Node.js, or wasm
 *      runtimes."
 */

#include <emscripten.h>

namespace rtl
{

/**
 *  WebMidi detection function. TODO!
 */

bool
detect_web_midi ()
{
    return false;
}

/*------------------------------------------------------------------------
 * web_midi_shim
 *------------------------------------------------------------------------*/

std::unique_ptr<web_midi_shim> shim{nullptr};

static void
ensure_shim()
{
    if (shim)                           // if (shim.get() != nullptr )
        return;

    shim.reset(new web_midi_shim());
}

static bool
is_web_midi_available()
{
    ensure_shim();
    return MAIN_THREAD_EM_ASM_INT
    ({
        if (typeof window._rtmidi_internals_waiting === "undefined")
        {
            console.log("Using Web MIDI API without opening it");
            return false;
        }
        if (window._rtmidi_internals_waiting)
        {
            console.log("Using Web MIDI API while querying it");
            return false;
        }
        if (_rtmidi_internals_midi_access == null)
        {
            console.log("Using Web MIDI API while it is unavailable");
            return false;
        }
        return true;
    });
}

web_midi_shim::web_midi_shim ()
{
    MAIN_THREAD_ASYNC_EM_ASM
    ({
        if (typeof window._rtmidi_internals_midi_access !== "undefined")
            return;

        if (typeof window._rtmidi_internals_waiting !== "undefined")
        {
            console.log ("MIDI Access requested while request in progress");
            return;
        }

        // define functions

        window._rtmidi_internals_get_port_by_number =

        function(portnumber, isinput)
        {
            var midi = window._rtmidi_internals_midi_access;
            var devices = isinput ? midi.inputs : midi.outputs;
            var i = 0;
            for (var device of devices.values())
            {
                if (i == portnumber )
                    return device;

                i++;
            }
            console.log
            (
                "MIDI " + (isinput ? "input" : "output") +
                    " device of portnumber " + portnumber +
                    " is not found."
            );
            return null;
        };

        window._rtmidi_internals_waiting = true;
        window.navigator.requestMIDIAccess
        ({"sysex": true} ).then( (midiAccess) =>
        {
            window._rtmidi_internals_midi_access = midiAccess;
            window._rtmidi_internals_latest_message_timestamp = 0.0;
            window._rtmidi_internals_waiting = false;
            if (midiAccess == null)
            {
                console.log ("Could not get access to MIDI API");
            }
        });
    });
}

web_midi_shim::~web_midi_shim ()
{
    // No code needed
}

std::string
web_midi_shim::get_port_name (int portnumber, bool isinput)
{
    if (! is_web_midi_available())
        return "";

    char * ret = (char *) MAIN_THREAD_EM_ASM_INT
    ({
        var port = window._rtmidi_internals_get_port_by_number($0, $1);
        if (port == null)
            return null;

        var length = lengthBytesUTF8(port.name) + 1;
        var ret = _malloc(length);
        stringToUTF8(port.name, ret, length);
        return ret;
        }, portnumber, isinput, &ret
    );
    if (ret == nullptr)
        return "";

    std::string s = ret;
    free(ret);
    return s;
}

/*------------------------------------------------------------------------
 * midi_web_in
 *------------------------------------------------------------------------*/

midi_web_in::midi_web_in
(
    const std::string & clientname, unsigned queuesizelimit
) :
    midi_in_api   (queuesizelimit)
{
    initialize(clientname);
}

midi_web_in::~midi_web_in ()
{
    close_port();
}

extern "C" void EMSCRIPTEN_KEEPALIVE rtmidi_on_midi_message_proc
(
    midi_in_api::rtmidi_in_data * data,
    uint8_t * inputBytes,
    int32_t length,
    double domHighResTimeStamp
)
{
    auto & message = data->message;
    message.bytes.resize(message.bytes.size() + length);
    memcpy(message.bytes.data(), inputBytes, length);

    // FIXME: handle timestamp

    if (data->usingCallback)
    {
        rtmidi_in_data::callback_t cb = data->user_callback();
        cb(message.timeStamp, &message.bytes, data->userData);
    }
}

bool
midi_web_in::open_port (int portnumber, const std::string & portname)
{
    if (! is_web_midi_available() )
        return false;

    if (open_port_number >= 0)
        return false;

    MAIN_THREAD_EM_ASM
    ({
        // In Web MIDI API world, there is no step to open a port,
        // but we have to register the input callback instead.

        var input = window._rtmidi_internals_get_port_by_number($0, true);
        input.onmidimessage = function(e)
        {
            // In rtmidi world, timestamps are delta time from previous message,
            // while in Web MIDI world timestamps are relative to window creation
            // time (i.e. kind of absolute time with window "epoch" time).

            var rtmidiTimestamp =
                window._rtmidi_internals_latest_message_timestamp == 0.0 ?
                    0.0 :
                    e.timeStamp - window._rtmidi_internals_latest_message_timestamp
                    ;
            window._rtmidi_internals_latest_message_timestamp = e.timeStamp;
            Module.ccall
            (
                'rtmidi_on_midi_message_proc', 'void',
                ['number', 'array', 'number', 'number'],
                [$1, e.data, e.data.length, rtmidiTimestamp]
            );
        };
    }, portnumber, &m_input_data);
    open_port_number = portnumber;
    return true;
}

bool
midi_web_in::open_virtual_port (const std::string &portname )
{

  m_error_string = "midi_web_in::open_virtual_port: unimplemented in Web MIDI API";
  error(rterror::kind::warning, m_error_string);
  return true;
}

bool
midi_web_in::close_port ()
{
    if (open_port_number < 0)
        return false;

    MAIN_THREAD_EM_ASM
    ({
        var input = _rtmidi_internals_get_port_by_number($0, true);
        if (input == null)
        {
            console.log ("Port #" + $0 + " could not be found.");
            return false;
        }
        input.onmidimessage = null; // unregister event handler
        }, open_port_number
    );
    open_port_number = -1;
    return true;
}

bool
midi_web_in::set_client_name (const std::string & clientname)
{
    client_name = clientname;
    return true;
}

bool
midi_web_in::set_port_name (const std::string & portname)
{
    m_error_string = "midi_web_in::set_port_name: not implemented in Web MIDI API";
    error(rterror::kind::warning, m_error_string);
    return true;
}

int
midi_web_in::get_port_count ()
{
    if (! is_web_midi_available())
        return 0;

    return MAIN_THREAD_EM_ASM_INT
    ({
        return _rtmidi_internals_midi_access.inputs.size;
    });
}

std::string
midi_web_in::get_port_name (int portnumber)
{
    if (! is_web_midi_available())
        return "";

    return shim->get_port_name(portnumber, true);
}

bool
midi_web_in::initialize (const std::string & clientname)
{
    ensure_shim();
    set_client_name(clientname);
    return true;
}

/*------------------------------------------------------------------------
 * midi_web_out
 *------------------------------------------------------------------------*/

midi_web_out::midi_web_out (const std::string & clientname)
{
    initialize (clientname);
}

midi_web_out::~midi_web_out ()
{
    (void) close_port();
}

/**
 *  In the Web MIDI API world, there is no step to open a port.
 */

bool
midi_web_out::open_port (int portnumber, const std::string & portname)
{
    if (! is_web_midi_available())
        return false;

    if (open_port_number >= 0)
        return false;


    open_port_number = portnumber;
    return true;
}

bool
midi_web_out::open_virtual_port (const std::string &portname )
{
    m_error_string =
        "midi_web_out::open_virtual_port: not implemented in Web MIDI API";

    error(rterror::kind::warning, m_error_string);
    return true;
}

/**
 *  There is really nothing to do for output at JS side.
 */

bool
midi_web_out::close_port ()
{
    open_port_number = -1;
    return true;
}

bool
midi_web_out::set_client_name (const std::string & clientname)
{
    client_name = clientname;
    return true;
}

bool
midi_web_out::set_port_name (const std::string & portname)
{
    m_error_string = "midi_web_out::set_port_name: unimplemented in Web MIDI API";
    error(rterror::kind::warning, m_error_string);
    return true;
}

int
midi_web_out::get_port_count ()
{
    if (! is_web_midi_available())
        return 0;

    return MAIN_THREAD_EM_ASM_INT
    ({
        return _rtmidi_internals_midi_access.outputs.size;
    });
}

std::string
midi_web_out::get_port_name (int portnumber)
{
    if (! is_web_midi_available())
        return "";

    return shim->get_port_name (portnumber, false);
}

bool
midi_web_out::send_message (const midi::byte * message, size_t sz)
{
    if (open_port_number < 0)
        return false;

    MAIN_THREAD_EM_ASM
    ({
        var output = _rtmidi_internals_get_port_by_number ($0, false);
        if (output == null)
        {
            console.log ("Port #" + $0 + " could not be found.");
            return;
        }
        var buf = new ArrayBuffer($2);
        var msg = new Uint8Array(buf);
        msg.set(new Uint8Array(Module.HEAPU8.buffer.slice( $1, $1 + $2)));
        output.send(msg);
        }, open_port_number, message, sz
    );
    return true;
}

bool
midi_web_out::initialize (const std::string & clientname )
{
    if (shim)                           // if (shim.get() != nullptr )
        return;

    shim.reset(new web_midi_shim());
    set_client_name(clientname);
    return true;
}

}           // namespace rtl

#endif      // defined RTL66_BUILD_WEB_MIDI

/*
 * midi_web_midi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

