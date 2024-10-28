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
 * \file          rtmidi_c.cpp
 *
 *    A reworking of RtMidi.cpp, with the same functionality but different
 *    coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-06-01
 * \license       See above.
 *
 */

#include <string.h>
#include <stdlib.h>

#include "midi/message.hpp"             /* midi::message base class         */
#include "rtl/midi/rtmidi_c.h"          /* declare this module              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi base class           */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/midi/midi_api.hpp"        /* rtl::midi_api base class         */
#include "rtl/rterror.hpp"              /* rtl::rterror                     */

/**
 *  The RtMidiCCallback type is void (*) (double, cbytes, size_t, void *).
 */

class CallbackProxyUserData
{

private:

    /**
     *  This callback type uses a byte pointer and a size parameter.
     */

    RtMidiCCallback mc_callback;

    /**
     *  User data.  Still studying this one.
     */

    void * m_user_data;

public:

    CallbackProxyUserData
    (
        RtMidiCCallback cb,             /* rtl::rtmidi_in_data::callback_t  */
        void * userdata
    ) :
        mc_callback (cb),
        m_user_data (userdata)
    {
        // No code
    }

    RtMidiCCallback callback (void)
    {
        return mc_callback;
    }

    void * user_data (void)
    {
        return m_user_data;
    }

};

/*
 *  rtmidi API
 */

/**
 *  A new addition to the API, to encapsulate getting an rtmidi pointer.
 *  It throws an rterror exception to avoid throwing a generic, app-terminating
 *  exception.
 */

static rtl::rtmidi *
rt_device_ptr (RtMidiPtr device)
{
    rtl::rtmidi * result = nullptr;
    if (not_nullptr(device))
    {
        if (not_nullptr(device->ptr))
        {
            result = static_cast<rtl::rtmidi *>(device->ptr);
        }
        else
        {
            throw rtl::rterror
            (
                "RtMidiPtr not assigned", rtl::rterror::kind::invalid_device
            );
        }
    }
    else
    {
        throw rtl::rterror
        (
            "RtMidiPtr is null", rtl::rterror::kind::invalid_device
        );
    }
    return result;
}

static rtl::rtmidi_in *
rt_in_device_ptr (RtMidiPtr device)
{
    return dynamic_cast<rtl::rtmidi_in *>(rt_device_ptr(device));
}

/**
 *  Gets the "integers" associated with the APIs that were built at compile
 *  time.  Renamed from "rtmidi_get_compiled_api()" to better reflect what it
 *  does.
 *
 * \param [out] apis
 *      Provides an array to hold the values.  In test/api_names.cpp,
 *      this is a pre-allocated vector of RtMidiApi values.  If null, nothing
 *      is copied.
 *
 * \param apis_size
 *      The size of the output array of RtMidiApi values. Probably good
 *      to use size 8.  If zero, nothing is copied.
 *
 * \return
 *      Returns the number of API values actually copied, or the number
 *      actually found, if the parameters above are null.
 */

int
rtmidi_get_compiled_apis (RtMidiApi * apis, int apis_size)
{
    int num = rtl::rtmidi::api_count();
    if (not_nullptr(apis) && (apis_size > 0) && (num > 0))
    {
        if (num > apis_size)
            num = apis_size;

        for (int i = 0; i < num; ++i)
            apis[i] = static_cast<RtMidiApi>(rtl::rtmidi::api_by_index(i));
    }
    return num;
}

/**
 *  Similar to rtmidi_get_compiled_apis(), but for APIs that are actually
 *  "running" on the system.
 */

int
rtmidi_get_detected_apis (RtMidiApi * apis, int apis_size)
{
    rtl::rtmidi::api_list detected;
    rtl::rtmidi::get_detected_apis(detected);
    int num = int(detected.size());
    if (not_nullptr(apis) && (apis_size > 0) && num > 0)
    {
        if (num > apis_size)
            num = apis_size;

        int i = 0;
        for (auto a : detected)
        {
            apis[i] = static_cast<RtMidiApi>(a);
            ++i;
            if (i == num)
                break;
        }
        return i;
    }
    return num;
}

const char *
rtmidi_api_name (RtMidiApi rapi)
{
    static std::string result;
    rtl::rtmidi::api apicode = static_cast<rtl::rtmidi::api>(rapi);
    result = rtl::rtmidi::api_name(apicode).c_str();
    return result.c_str();
}

/**
 *  We need to be able to return a C string. The static string inside
 *  is initialized only once, so we have to then assign it later.
 *  It is unlikely that two threads will call this display function.
 *
 *  We hope.
 */

const char *
rtmidi_api_display_name (RtMidiApi rapi)
{
    static std::string s_result{"null"};                        /* \tricky  */
    rtl::rtmidi::api r = static_cast<rtl::rtmidi::api>(rapi);
    s_result = rtl::rtmidi::api_display_name(r);
    return s_result.c_str();
}

RtMidiApi
rtmidi_api_by_name (const char * name)
{
    return static_cast<RtMidiApi>(rtl::rtmidi::api_by_name(name));
}

RtMidiApi
rtmidi_api_by_index (int index)
{
    return static_cast<RtMidiApi>(rtl::rtmidi::api_by_index(index));
}

void
rtmidi_error
(
    MidiApi * mapi,                     /* rtl::midi_api * mapi */
    RtMidiErrorType type,
    const char * errstring
)
{
    std::string msg = errstring;
    rtl::rterror::kind ek = rtl::int_to_error_kind(static_cast<int>(type));
    reinterpret_cast<rtl::midi_api *>(mapi)->error(ek, msg);
}

/**
 *  Open a MIDI port.
 *
 * \param port
 *      Must be greater than 0
 *
 * \param portname
 *      Name for the application port.
 *
 * See rtmidi::open_port().
 */

void
rtmidi_open_port (RtMidiPtr device, int portnumber, const char * portname)
{
    std::string name = portname;
    try
    {
        rt_device_ptr(device)->open_port(portnumber, name);
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
    }
}

/**
 *  Creates a virtual MIDI port to which other software applications can connect.
 *
 *  See rtmidi::open_virtual_port().
 *
 * \param portname
 *      Name for the application port.
 */

void
rtmidi_open_virtual_port (RtMidiPtr device, const char * portname)
{
    std::string name = portname;
    try
    {
        rt_device_ptr(device)->open_virtual_port(name);
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
    }
}

/**
 *  Close a MIDI connection.
 *
 *  See rtmidi::close_port().
 */

void
rtmidi_close_port (RtMidiPtr device)
{
    try
    {
        rt_device_ptr(device)->close_port();
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
    }
}

/**
 *  Return the number of available MIDI ports.
 *
 *  See rtmidi::get_port_count().
 */

unsigned
rtmidi_get_port_count (RtMidiPtr device)
{
    try
    {
        return rt_device_ptr(device)->get_port_count();
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
        return 0;                       /* -1  */
    }
}

/**
 *  Access a string identifier for the specified MIDI input port number.
 *
 *  To prevent memory leaks a char buffer must be passed to this function.  NULL
 *  can be passed as bufOut parameter, and that will write the required buffer
 *  length in the bufLen.
 *
 * See rtmidi::get_port_name().
 */

int
rtmidi_get_port_name
(
    RtMidiPtr device,
    int portnumber,
    char * bufout, int * buflen
)
{
    if (both_are_nullptr(bufout, buflen))
    {
        return -1;
    }

    std::string name;
    try
    {
        name = rt_device_ptr(device)->get_port_name(portnumber);
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
        return -1;
    }
    if (is_nullptr(bufout))
    {
        *buflen = static_cast<int>(name.size()) + 1;
        return 0;
    }
    return snprintf(bufout, static_cast<size_t>(*buflen), "%s", name.c_str());
}

/*
 * rtmidi_in API
 */

/**
 *  Create a default RtMidiInPtr value, with no initialization.
 */

RtMidiInPtr
rtmidi_in_create_default (void)
{
    RtMidiWrapper * wrp = new RtMidiWrapper;
    try
    {
        rtl::rtmidi::api rapi = rtl::rtmidi::desired_api();
        rtl::rtmidi_in * rin = new rtl::rtmidi_in(rapi);
        wrp->ptr = (void *) rin;
        wrp->data = 0;
        wrp->ok  = true;
        wrp->msg = "";
    }
    catch (const rtl::rterror & err)
    {
        wrp->ptr = 0;
        wrp->data = 0;
        wrp->ok  = false;
        wrp->msg = err.what();
    }
    return wrp;
}

/**
 *  Create a  RtMidiInPtr value, with given rtmidi::api, clientname and
 *  queuesizelimit.
 *
 * \param rapi
 *      An optional MIDI API id can be specified.
 *
 * \param clientname
 *      An optional client name can be specified. This will be used to group the
 *      ports that are created by the application.
 *
 * \param queuesizelimit
 *      An optional size of the MIDI input queue can be specified.
 *
 * See rtmidi_in::rtmidi_in().
 */

RtMidiInPtr
rtmidi_in_create
(
    RtMidiApi rapi, const char * clientname, unsigned queuesizelimit
)
{
    std::string name = clientname;
    RtMidiWrapper * wrp = new RtMidiWrapper;
    try
    {
        rtl::rtmidi_in * rin = new rtl::rtmidi_in
        (
            static_cast<rtl::rtmidi::api>(rapi), name, queuesizelimit
        );
        wrp->ptr = (void *) rin;
        wrp->data = nullptr;
        wrp->ok  = true;
        wrp->msg = "";

    }
    catch (const rtl::rterror & err)
    {
        wrp->ptr = nullptr;
        wrp->data = nullptr;
        wrp->ok  = false;
        wrp->msg = err.what();
    }
    return wrp;
}

/**
 *  Free the given RtMidiInPtr.
 */

void
rtmidi_in_free (RtMidiInPtr device)
{
    if (not_nullptr(device))
    {
        if (not_nullptr(device->data))
            delete static_cast<CallbackProxyUserData *>(device->data);

        try
        {
            rtl::rtmidi_in * rmi = rt_in_device_ptr(device);
            if (not_nullptr(rmi))
                delete rmi;
        }
        catch (const rtl::rterror & /*err*/)
        {
            // no code
        }
        delete device;
    }
}

/**
 *  Returns the MIDI API specifier for the given instance of rtmidi_in.
 *
 *  See \ref rtmidi_in::get_current_api().
 */

RtMidiApi
rtmidi_in_get_current_api (RtMidiPtr device)
{
    try
    {
        rtl::rtmidi_in * rmi = rt_in_device_ptr(device);
        return static_cast<RtMidiApi>(rmi->get_current_api());
    }
    catch (const rtl::rterror & err)
    {
        device->ok = false;
        device->msg = err.what();
        return RTMIDI_API_UNSPECIFIED;
    }
}

/**
 *  Internal function to call the callback.  Note that it is an
 *  rtmidi_in_data::callback_t() function, with parameters of double,
 *  midi_message, and void pointer.
 *
 *  Reminder: RtMidiCCallback mc_callback;
 */

static void
callback_proxy
(
    double tstamp,
    midi::message * message,
    void * userdata
)
{
    CallbackProxyUserData * d =
        reinterpret_cast<CallbackProxyUserData *>(userdata);

    d->callback()
    (
        tstamp, message->data_ptr(), message->size(), d->user_data()
    );
}

/**
 *  Set a callback function to be invoked for incoming MIDI messages.
 *
 *  See rtmidi_in::set_input_callback().  Here, the callback_proxy() function
 *  converts the midi_message parameter to its pointer and size value.
 */

void
rtmidi_in_set_callback
(
    RtMidiInPtr device,
    RtMidiCCallback cb,                 /* rtl::rtmidi_in_data::callback_t  */
    void * userdata
)
{
    device->data = (void *) new CallbackProxyUserData(cb, userdata);
    try
    {
        static_cast<rtl::rtmidi_in *>(device->ptr)->
            set_input_callback(callback_proxy, device->data);
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
        delete (CallbackProxyUserData *) device->data;
        device->data = 0;
    }
}

/**
 *  Cancel use of the current callback function (if one exists).
 *
 *  See \ref rtmidi_in::cancel_input_callback().
 */

void
rtmidi_in_cancel_callback (RtMidiInPtr device)
{
    try
    {
        rt_in_device_ptr(device)->cancel_input_callback();
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
    }
    if (not_nullptr(device->data))
    {
        delete reinterpret_cast<CallbackProxyUserData *>(device->data);
        device->data = nullptr;
    }
}

/**
 *  Specify whether certain MIDI message types should be queued or ignored during
 *  input.
 *
 *  See \ref rtmidi_in::ignore_midi_types().
 */

void
rtmidi_in_ignore_types
(
    RtMidiInPtr device, bool midisysex, bool miditime, bool midisense
)
{
    try
    {
        rtl::rtmidi_in * rmi = rt_in_device_ptr(device);
        rmi->ignore_midi_types(midisysex, miditime, midisense);
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
    }
}

/**
 *  Fill the user-provided array with the data bytes for the next available
 *  MIDI message in the input queue and return the event delta-time in
 *  seconds.
 *
 *  See rtmidi_in::get_message().  This function is the same in name and
 *  functionality as in the original RtMidi module rtmidi_c.  However,
 *  even in that project it is used only in a sample Go file, rtmidi.go.
 *
 * \param message
 *      Must point to a char* that is already allocated.  SYSEX messages
 *      maximum size being 1024, a statically allocated array could be
 *      sufficient.
 *
 * \param size
 *      Is used to return the size of the message obtained.  Must be set to
 *      the size of \ref message when calling.
 *
 * \return
 *      Returns the double timestamp of the message.
 */

double
rtmidi_in_get_message
(
    RtMidiInPtr device,
    midi::byte * msg,
    size_t * psz
)
{
    try
    {
        // FIXME: use allocator to achieve efficient buffering

#if defined USE_OLD_CODE
        midi::bytes v;
        double ret = static_cast<rtl::rtmidi_in *>(device->ptr)->get_message(&v);
        if (v.size () > 0 && v.size() <= *psz)
            memcpy(msg, v.data(), v.size());

        *psz = v.size();
#else
        midi::message m;
        double ret = static_cast<rtl::rtmidi_in *>(device->ptr)->get_message(m);
        if (m.size () > 0 && m.size() <= *psz)
            memcpy(msg, m.data_ptr(), m.size());

        *psz = m.size();
#endif
        return ret;
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
        return -1;
    }
    catch (...)
    {
        device->ok  = false;
        device->msg = "Unknown error";
        return -1;
    }
}

/*
 *  rtmidi_out API
 */

/**
 *  Create a default RtMidiInPtr value, with no initialization.
 */

RtMidiOutPtr
rtmidi_out_create_default (void)
{
    RtMidiWrapper * wrp = new RtMidiWrapper;
    try
    {
        rtl::rtmidi::api rapi = rtl::rtmidi::desired_api();
        rtl::rtmidi_out * rout = new rtl::rtmidi_out(rapi);
        wrp->ptr = (void*) rout;
        wrp->data = nullptr;
        wrp->ok  = true;
        wrp->msg = "";

    }
    catch (const rtl::rterror & err)
    {
        wrp->ptr = nullptr;
        wrp->data = nullptr;
        wrp->ok  = false;
        wrp->msg = err.what();
    }
    return wrp;
}

/**
 *  Create a RtMidiOutPtr value, with given and clientname.
 *
 *  \param rapi
 *      An optional API id can be specified.
 *
 *  \param clientname
 *      An optional client name can be specified. This will be used to group
 *      the ports that are created by the application.
 *
 * See rtmidi_out::rtmidi_out().
 */

RtMidiOutPtr
rtmidi_out_create (RtMidiApi rapi, const char * clientname)
{
    RtMidiWrapper * wrp = new RtMidiWrapper;
    std::string name = clientname;
    try
    {
        rtl::rtmidi::api rmapi = static_cast<rtl::rtmidi::api>(rapi);
        rtl::rtmidi_out * rout = new rtl::rtmidi_out(rmapi, name);
        wrp->ptr = (void *) rout;
        wrp->data = nullptr;
        wrp->ok  = true;
        wrp->msg = "";
    }
    catch (const rtl::rterror & err)
    {
        wrp->ptr = nullptr;
        wrp->data = nullptr;
        wrp->ok  = false;
        wrp->msg = err.what();
    }
    return wrp;
}

/**
 *  Free the given RtMidiOutPtr.
 */

void
rtmidi_out_free (RtMidiOutPtr device)
{
    delete static_cast<rtl::rtmidi_out *>(device->ptr);
    delete device;
}

/**
 *  Returns the MIDI API specifier for the given instance of rtmidi_out.
 *
 *  See \ref rtmidi_out::get_current_api().
 */

RtMidiApi
rtmidi_out_get_current_api (RtMidiPtr device)
{
    try
    {
        rtl::rtmidi_out * r =
            static_cast<rtl::rtmidi_out *>(device->ptr);

        return static_cast<RtMidiApi>((r->get_current_api()));
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
        return RTMIDI_API_UNSPECIFIED;
    }
}

/**
 *  Immediately send a single message out an open MIDI output port.
 *
 * qSee \ref rtmidi_out::send_message().
 */

int
rtmidi_out_send_message
(
    RtMidiOutPtr device, const midi::byte * message, int len
)
{
    try
    {
        (void) static_cast<rtl::rtmidi_out *>(device->ptr)->send_message
        (
            message, len
        );
        return 0;
    }
    catch (const rtl::rterror & err)
    {
        device->ok  = false;
        device->msg = err.what();
        return -1;
    }
    catch (...)
    {
        device->ok  = false;
        device->msg = "Unknown error";
        return -1;
    }
}

#if defined RTL66_BUILD_JACK

void
rtmidi_start_jack (bool startit)
{
    rtl::rtmidi::start_jack(startit);
}

#endif

void
rtmidi_silence_messages (bool silent)
{
    rtl::rtmidi::silence_messages(silent);
}

bool
rtmidi_set_desired_api (RtMidiApi rapi)
{
    bool result = rapi > RTMIDI_API_UNSPECIFIED && rapi < RTMIDI_API_MAX;
    if (result)
        rtl::rtmidi::desired_api(static_cast<rtl::rtmidi::api>(rapi));

    return result;
}

bool
rtmidi_set_selected_api (RtMidiApi rapi)
{
    bool result = rapi > RTMIDI_API_UNSPECIFIED && rapi < RTMIDI_API_MAX;
    if (result)
        rtl::rtmidi::selected_api(static_cast<rtl::rtmidi::api>(rapi));

    return result;
}

#if defined RTL66_USE_GLOBAL_CLIENTINFO

/*
 * Not included below: use_auto_connect() and use_port_refresh().
 */

void
rtmidi_use_virtual_ports (bool flag)
{
    midi::global_client_info().virtual_ports(flag);
}

void
rtmidi_use_auto_connect (bool flag)
{
    midi::global_client_info().auto_connect(flag);
}

void
rtmidi_global_ppqn (int ppq)
{
    midi::global_client_info().global_ppqn(static_cast<midi::ppqn>(ppq));
}

void
rtmidi_global_bpm (double b)
{
    midi::global_client_info().global_bpm(static_cast<midi::bpm>(b));
}

#endif

/*
 * Defined in the test_helpers cpp module, declare here to avoid including
 * test_helpers.hpp and causing errors in build C tests.
 */

extern bool rt_simple_cli
(
    const std::string & appname,
    int argc, char * argv []
);

/**
 *  This function, meant for C usage, calls the test_helpers version of ths
 *  function.  Thus we interface C and C++ code.
 */

bool
rtmidi_simple_cli (const char * appname, int argc, char * argv [])
{
    std::string s = appname;
    return rt_simple_cli(s, argc, argv);    /* C++ version  */
}

/*
 * rtmidi_c.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

