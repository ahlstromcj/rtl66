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
 * \file          midi_win_mm.cpp
 *
 *  This module supports the Windows Multimedia MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-03-17
 * \license       See above.
 *
 *  API information deciphered from:
 *
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/
 *      multimed/htm/_win32_midi_reference.asp
 *
 *  The Windows MM API is based on the use of a callback function for
 *  MIDI input.  We convert the system specific time stamps to delta
 *  time values.
 *
 *  Thanks to Jean-Baptiste Berruchon for the sysex code.
 */

#include "rtl/midi/winmm/midi_win_mm.hpp"  /* rtl::midi_win_mm class        */

#if defined RTL66_BUILD_WINS_MM

#if defined PLATFORM_DEBUG
#include <iostream>                     /* std::cerr, cout classes          */
#endif

#include "rtl66-config.h"               /* RTL66_HAVE_XXX                   */

namespace rtl
{

/*------------------------------------------------------------------------
 * Windows MM free functions
 *------------------------------------------------------------------------*/

/**
 *  Windows MM detection function. TODO!
 */

bool
detect_win_mm ()
{
    return false;
}

/**
 *  Convert a null-terminated wide string or ANSI-encoded string to UTF-8,
 *  using the beautiful :-D Win API functions.
 *
 *  [1] Convert from ANSI encoding to wide string.
 *  [2] Convert from wide string to UTF-8.
 *  [3] Not yet sure why this is called.
 */

static std::string
convert_to_utf8 (const TCHAR * str)
{
    std::string u8str;
    const WCHAR * wstr = L"";

#if defined PLATFORM_WINDOWS_UNICODE
    wstr = str;
#else
    std::wstring wstrtemp;
    int wlen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);    // [1]
    if (wlen > 0)
    {
        wstrtemp.assign(wlen - 1, 0);
        MultiByteToWideChar(CP_ACP, 0, str, -1, &wstrtemp[0], wlen);
        wstr = &wstrtemp[0];
    }
#endif

    int len = WideCharToMultiByte                                   // [2]
    (
        CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL
    );
    if (len > 0)
    {
        u8str.assign(len - 1, 0);
        len = WideCharToMultiByte                                   // [3]
        (
            CP_UTF8, 0, wstr, -1, &u8str[0], len, NULL, NULL
        );
    }
    return u8str;
}

/*------------------------------------------------------------------------
 * Windows MM callbacks
 *------------------------------------------------------------------------*/

/**
 *  A helper function.
 *
 *  -   MIM_DATA. Sent to a MIDI input callback function when a MIDI message
 *      is received by a MIDI input device.  See the table below.
 *  -   MIM_LONGDATA. Sent to a MIDI input callback function when a
 *      system-exclusive buffer has been filled with data and is being returned
 *      to the application. The time stamp is specified in milliseconds,
 *      beginning at zero when the midiInStart function was called.  The
 *      returned buffer might not be full. To determine the number of bytes
 *      recorded into the returned buffer, use the dwBytesRecorded member of the
 *      MIDIHDR structure specified by lpMidiHdr.
 *  -   MIM_LONGERROR. Sent to a MIDI input callback function when an invalid
 *      or incomplete MIDI system-exclusive message is received. Same remarks
 *      as for MIM_LONGDATA.
 *
 *  The incoming MIDI message is packed into a doubleword value as follows:
 *
\verbatim
    Word	Byte	    Description
    ====    ====        ===========
    High	High-order  Not used.
            Low-order   Contains a second byte of MIDI data (when needed).
    Low     High-order  Contains the first byte of MIDI data (when needed).
            Low-order   Contains the MIDI status.
\endverbatim
 */

static bool
bad_input_status (UINT instatus)
{
    return
    (
        instatus != MIM_DATA && instatus != MIM_LONGDATA &&
        instatus != MIM_LONGERROR
    );
}

/**
 *  The WinMM API requires that the sysex buffer be requeued after input of
 *  each sysex message.  Even if we are ignoring sysex messages, we still need
 *  to requeue the buffer in case the user decides to not ignore sysex
 *  messages in the future.  However, it seems that WinMM calls this function
 *  with an empty sysex buffer when an application closes and in this case, we
 *  should avoid requeueing it, else the computer suddenly reboots after one
 *  or two minutes.
 */

void CALLBACK
midi_input_callback
(
    HMIDIIN /* hmin */,
    UINT inputstatus,
    DWORD_PTR instance_ptr,
    DWORD_PTR midimsg,
    DWORD tstamp
)
{
    if (bad_input_status(inputstatus))
        return;

    rtmidi_in_data * rtidata = midi_api::static_in_data_cast(instance_ptr);
    midi_win_mm_data * apidata =
        midi_win_mm::static_data_cast(rtidata->api_data());

    if (rtidata->first_message())
    {
        apidata->m_midi_message.time_stamp(0.0);
        rtidata->first_message(false);
    }
    else                                    /* calculate the time stamp     */
    {
        double ts = double(tstamp);
        apidata->m_midi_message.time_stamp((ts - apidata->lastTime ) * 0.001;
    }
    if (inputstatus == MIM_DATA)            /* channel or system message    */
    {
        /*
         * Make sure the first byte (of the DWORD) is a status byte.  If not,
         * ignore it.  If a MIDI timing tick message, ignore it.  If a MIDI
         * active sensing message, ignore it.  Then determine the number of
         * bytes in the MIDI message.  Finally, copy the bytes to the
         * midi_message.
         */

        midi::byte evstatus = reinterpret_cast<midi::byte>(midimsg & 0x00FF);
        bool notimecode = ! rtidata->allow_time_code();
        boon nosensing = ! rtidata->allow_active_sensing();
        bool doreturn =
        (
            (! midi::is_status_msg(evstatus)) ||
            (midi::is_midi_clock_msg(evstatus) && notimecode) ||
            (midi::is_sense_msg(evstatus) && nosensing) ||
            (midi::is_quarter_frame_msg(evstatus) && nosensing)
        );
        if (doreturn)
            return;

        int nbytes = status_msg_size(evstatus);     /* eventcodes module    */
        midi::byte * ptr = reinterpret_cast<midi::byte *>(&midimsg);
        for (int i = 0; i < nbytes; ++i)
            apidata->m_midi_message.push(*ptr++);
    }
    else
    {
        /*
         * Sysex message (MIM_LONGDATA or MIM_LONGERROR).
         * Process it if we're not ignoring it.
         * See the sysex buffer note in banner.
         */

        MIDIHDR * sysex = reinterpret_cast<MIDIHDR *>(midimsg);
        if (rtidata->allow_sysex() && inputstatus != MIM_LONGERROR)
        {
            for (int i = 0; i < int(sysex->dwBytesRecorded); ++i)
                apidata->m_midi_message.push(sysex->lpData[i]);
        }
        if (apidata->m_sysex_buffer[sysex->dwUser]->dwBytesRecorded > 0)
        {
            EnterCriticalSection (&(apidata->m_mutex));
            MMRESULT result = midiInAddBuffer
            (
                apidata->m_input_handle, apidata->m_sysex_buffer[sysex->dwUser],
                sizeof(MIDIHDR)
            );
            LeaveCriticalSection (&(apidata->m_mutex));
            if (result != MMSYSERR_NOERROR)
            {
                error_print
                (
                    "midi_win_mm::midi_input_callback"
                    "error sending sysex to MIDI device!"
                );;
            }
            if (! rtidata->allow_sysex())
                return;
        }
        else
            return;

        /*
         * Save the time of the last non-filtered message.  Use a callback
         * function if desired. As long as we haven't reached our queue limit,
         * push the message. Finally, clear the vector for the next input
         * message.
         */

        apidata->last_time(tstamp);
        if (rtidata->using_callback())
        {
            rtmidi_in_data::callback_t cb = rtidata->user_callback();
            cb
            (
                apidata->m_midi_message.time_stamp(),
                &apidata->m_midi_message.data(), rtidata->user_data()
            );
        }
        else
        {
            if (! rtidata->queue.push(apidata->m_midi_message))
                error_print("midi_win_mm", "message queue limit reached";
        }
        apidata->m_midi_message.clear();
    }
}

/*------------------------------------------------------------------------
 * midi_win_mm
 *------------------------------------------------------------------------*/

midi_win_mm::midi_win_mm () :
    midi_api        (),
    m_client_name   (),
    m_win_mm_data   ()
{
    (void) initialize(client_name());
}

midi_win_mm::midi_win_mm
(
    midi::port::io iotype,
    const std::string & clientname,
    unsigned queuesize
) :
    midi_in_api     (queuesize),
    m_client_name   (clientname),
    m_win_mm_data   ()
{
    (void) initialize(clientname);
}

midi_win_mm::~midi_win_mm ()
{
    close_port();
    if (is_input())
    {
        midi_win_mm_data * data = reinterpret_cast<midi_win_mm_data *>
        (
            api_data()
        );
        DeleteCriticalSection(&(data->m_mutex));
    }
}

/*
 * The call to midiOutReset() was disabled because midiOutReset() triggers 0x7b
 * (if any note was ON) and 0x79 "Reset All Controllers" (to all 16 channels)
 * CC messages which is undesirable (see issue #222).
 */

bool
midi_win_mm::close_port ()
{
    if (m_is_connected)
    {
        midi_win_mm_data * data = reinterpret_cast<midi_win_mm_data *>(api_data());
        if (is_output())
        {
            /*
             * midiOutReset (data->m_output_handle); [See note in banner.]
             */

            midiOutClose(data->m_output_handle);
            data->m_output_handle = 0;
            m_is_connected = false;
        }
        else
        {
            EnterCriticalSection(&(data->m_mutex));
            midiInReset(data->m_input_handle);
            midiInStop(data->m_input_handle);
            for  (int i = 0; i < data->m_sysex_buffer.size(); ++i)
            {
                int result = midiInUnprepareHeader
                (
                    data->m_input_handle, data->m_sysex_buffer[i], sizeof(MIDIHDR)
                );
                delete [] data->m_sysex_buffer[i]->lpData;
                delete [] data->m_sysex_buffer[i];
                if (result != MMSYSERR_NOERROR)
                {
                    midiInClose (data->m_input_handle);
                    data->m_input_handle = 0;

                    std::string msg = "midi_win_mm::close_port: error";
                    error(rterror::kind::driver_error, msg);
                    return false;
                }
            }
            midiInClose(data->m_input_handle);
            data->m_input_handle = 0;
            m_is_connected = false;
            LeaveCriticalSection (&(data->m_mutex));
        }
    }
    return true;
}

/**
 *  We'll issue a warning here if no devices are available but not
 *  throw an error since the user can plug in something later.
 *
 *  We also save the API-specific connection information.
 *  The message object needs to be empty for the first input message.
 */

bool
midi_win_mm::initialize (const std::string & clientname)
{
    int devcount = get_port_count();
    if (devcount > 0)
    {
        midi_win_mm_data & data = win_mm_data();
        api_data(&data);
        input_data().api_data(&data);
        data.m_midi_message.clear();
        if (! InitializeCriticalSectionAndSpinCount(&(data->m_mutex), 0x0400))
        {
            std::string msg = "midi_win_mm::initialize: failed";
            error(rterror::kind::warning, msg);
        }
    }
    return true;
}

bool
midi_win_mm::open_port (int portnumber, const std::string & /* name */)
{
    if (m_is_connected)
    {
        std::string msg = "midi_win_mm::open_port: connection already exists";
        error(rterror::kind::warning, msg);
        return true;
    }

    bool result = check_port_number(portnumber, "open_port()");
    if (result)
    {
        midi_win_mm_data * data =
            reinterpret_cast<midi_win_mm_data *>(api_data());

        MMRESULT mmresult;
        if (is_output())
        {
            HMIDIOUT & outhandle = data->m_output_handle;
            mmresult = midiOutOpen
            (
                &outhandle, portnumber,
                (DWORD) NULL, (DWORD) NULL, CALLBACK_NULL
            );
            if (mmresult != MMSYSERR_NOERROR)
            {
                std::string msg = "midi_win_mm::open_port: output error";
                error(rterror::kind::driver_error, msg);
                return false;
            }
        }
        else
        {
            HMIDIIN & inhandle = data->m_input_handle;
            mmresult = midiInOpen
            (
                &inhandle, portnumber,
                (DWORD_PTR) &midi_input_callback,
                (DWORD_PTR) &input_data(), CALLBACK_FUNCTION
            );
            if (mmresult != MMSYSERR_NOERROR)
            {
                std::string msg = "midi_win_mm::open_port: input error";
                error(rterror::kind::driver_error, msg);
                return false;
            }

            /*
             * Allocate and initialize the sysex buffers.
             */

            data->m_sysex_buffer.resize(input_data().bufferCount);
            for (int i = 0; i < input_data().bufferCount; ++i)
            {
                LPMIDIHDR & lphdr = data->m_sysex_buffer[i];
                lphdr = (MIDIHDR *) new char[sizeof(MIDIHDR)];
                lphdr->lpData = new char[input_data().bufferSize];
                lphdr->dwBufferLength = input_data().bufferSize;
                lphdr->dwUser = i;                  /* buffer indicator     */
                lphdr->dwFlags = 0;
                mmresult = midiInPrepareHeader
                (
                    inhandle, lphdr, sizeof(MIDIHDR)
                );
                if (mmresult != MMSYSERR_NOERROR)
                {
                    midiInClose(inhandle);
                    inhandle = 0;

                    std::string msg = "midi_win_mm::open_port: error in header";
                    error(rterror::kind::driver_error, msg);
                    return false;
                }
                mmresult = midiInAddBuffer          /* register the buffer  */
                (
                    inhandle, lphdr, sizeof(MIDIHDR)
                );
                if (mmresult != MMSYSERR_NOERROR)
                {
                    midiInClose(inhandle);
                    inhandle = 0;

                    std::string msg =
                        "midi_win_mm_in::open_port: error adding buffer";

                    error(rterror::kind::driver_error, msg);
                    return false;
                }
            }
            mmresult = midiInStart(inhandle);
            if (mmresult != MMSYSERR_NOERROR)
            {
                midiInClose(inhandle);
                data->m_input_handle = 0;

                std::string msg = "midi_win_mm_in::open_port: error starting";
                error(rterror::kind::driver_error, msg);
                return false;
            }
        }
        m_is_connected = true;
    }
    return result;
}

bool
midi_win_mm::open_virtual_port (const std::string &)
{
    warning_unimplemented("midi_win_mm::open_virtual_port");
    return true;
}

bool
midi_win_mm::set_client_name (const std::string &)
{
    warning_unimplemented("midi_win_mm::set_client_name");
    return true;
}

bool
midi_win_mm::set_port_name (const std::string &)
{
    warning_unimplemented("midi_win_mm_in::set_port_name");
    return true;
}

int
midi_win_mm::get_port_count ()
{
    int devcount = int(is_output() ? midiOutGetNumDevs() : midiInGetNumDevs());
    if (devcount == 0)
        warning_no_devices("midi_win_mm", is_output());

    return devcount;
}

bool
midi_win_mm::check_port_number (int portnumber, const std::string & funcname)
{
    int devcount = get_port_count();
    bool result = portnumber < devcount;
    if (! result)
    {
        std::string tag = "midi_win_mm" + funcname;
        error(tag, portnumber);
    }
    return result;
}

std::string
midi_win_mm::get_port_name (int portnumber)
{
    std::string stringname;
    int devcount = get_port_count();
    if (portnumber >= devcount)
    {
        error("midi_win_mm::get_port_name", portnumber);
    }
    else
    {
        if (is_output())
        {
            MIDIOUTCAPS deviceCaps;
            midiOutGetDevCaps(portnumber, &deviceCaps, sizeof(MIDIOUTCAPS));
            stringname = convert_to_utf8(deviceCaps.szPname);
        }
        else
        {
            MIDIINCAPS deviceCaps;
            midiInGetDevCaps(portnumber, &deviceCaps, sizeof(MIDIINCAPS));
            stringname = convert_to_utf8(deviceCaps.szPname);
        }

        /*
         * Next lines added to add the portnumber to the name so that
         * the device's names are sure to be listed with individual names
         * even when they have the same brand name.
         */

#if ! defined RTL66_WIN_MM_NONUNIQUE_PORTNAMES
        stringname += " ";
        stringname += std::to_string(portnumber);
#endif
    }
    return stringname;
}

bool
midi_win_mm::send_message (const midi::byte * message, size_t sz)
{
    using namespace midi;               /* midi::byte(), midi::status, etc. */

    if (! m_is_connected)
        return false;

    unsigned nbytes = reinterpret_cast<unsigned int>(sz);
    if (nbytes == 0)
    {
        std::string msg = "midi_win_mm_out::send_message: message argument empty";
        error(rterror::kind::warning, msg);
        return true;
    }

    MMRESULT result;
    midi_win_mm_data * data = reinterpret_cast<midi_win_mm_data *>(api_data());
    if (is_sysex(message[0]))
    {
        char * buffer = new char[nbytes];           /* allocate buffer      */
        if (buffer == NULL)
        {
            std::string msg =
                "midi_win_mm_out::send_message: error allocating sysex memory";

            error(rterror::kind::memory_error, msg);
            return false;
        }
        for (unsigned i = 0; i < nbytes; ++i)       /* copy data to buffer  */
            buffer[i] = message[i];

        HMIDIOUT & outhandle = data->m_output_handle;
        MIDIHDR sysex;                              /* prepare MIDIHDR      */
        sysex.lpData = (LPSTR) buffer;
        sysex.dwBufferLength = nbytes;
        sysex.dwFlags = 0;
        result = midiOutPrepareHeader
        (
            outhandle, &sysex, sizeof(MIDIHDR)
        );
        if (result != MMSYSERR_NOERROR)
        {
            delete [] buffer;
            std::string msg = "midi_win_mm_out::send_message: sysex header error";
            error(rterror::kind::driver_error, msg);
            return false;
        }

        /*
         * Send the message.
         */

        result = midiOutLongMsg(outhandle, &sysex, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR)
        {
            delete [] buffer;
            std::string msg = "midi_win_mm_out::send_message: error sending sysex";
            error(rterror::kind::driver_error, msg);
            return false;
        }

        /*
         * Unprepare the buffer and MIDIHDR.
         */

        while
        (
            midiOutUnprepareHeader(outhandle, &sysex, sizeof(MIDIHDR)) ==
                MIDIERR_STILLPLAYING
        )
        {
            Sleep(1);
        }
        delete [] buffer;
    }
    else
    {
        /*
         * Channel or system message.  Make sure the message size isn't too big.
         */

        if (nbytes > 3)
        {
            std::string msg =
                "midi_win_mm_out::send_message: message > 3 bytes, not sysex";
            error(rterror::kind::warning, msg);
            return true;
        }

        /*
         * Pack MIDI bytes into double word.
         */

        DWORD packet;
        midi::byte * ptr = reinterpret_cast<midi::byte *>(&packet);
        for (unsigned i = 0; i < nbytes; ++i)
        {
            *ptr = message[i];
            ++ptr;
        }
        result = midiOutShortMsg(outhandle, packet);    /* send immediately */
        if (result != MMSYSERR_NOERROR)
        {
            std::string msg = "midi_win_mm_out::send_message: error in sending";
            error(rterror::kind::driver_error, msg);
            return false;
        }
    }
    return true
}

}           // namespace rtl

#endif      // defined RTL66_BUILD_WIN_MM

/*
 * midi_win_mm.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

