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
 * \file          masterbus.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2016-11-23
 * \updates       2025-09-13
 * \license       GNU GPLv2 or above
 *
 *  This file provides a base-class implementation for various master MIDI
 *  buss support classes.
 *
 *  The life-cycle of a midi::masterbus is something like this:
 *
 *      -   engine_query(). Create temporary I/O objects and use them
 *          to query the host for an existing API, such as ALSA or JACK.
 *          -   Get the rtl::rtmidi::api to be used.
 *          -   Collect the I/O ports that exist into shared pointers for
 *              midi::clientinfo for input and output.
 *      -   engine_connect(). Creates the MIDI engine client, which should
 *          serve as the "client" for all operations.  Creates a handle to the
 *          "engine", which is a way to access the functions of an API
 *          function (from ALSA or JACK, for example).  This handle is usually
 *          a void *, with the actual type provided by the
 *          rtl::selected_api().  It might also log any callbacks needed by
 *          the API.
 *      -   engine_initialize(). Use I/O port-selection options to pick
 *          either a subset of ports to "create", or create all ports that
 *          exist. If desired, auto-connect to the existing ports.
 *      -   engine_activate().  Some engines, like JACK, need to be activated
 *          after everything is set up in apple-pie order.
 *      -   During execution:
 *          -   Event I/O.
 *              -   1 input, 1 output port.
 *              -   A full set of I/O ports.
 *                  -   Processed serially in polling loops (Seq66)
 *                  -   Round robin processing?
 *                  -   Largest queue processing?
 *          -   Handle hanges to the set of external ports.
 *          -   Handle changes to transport (PPQN, BPM)
 *      -   engine_deactivate().  Deactivate the engine, if applicable.
 *      -   engine_deinitialize().
 *      -   engine_disconnect().
 *
 * Duplex/Service comments about I/O specific functions:
 *
 *  Input:
 *
 *      -   set_input_callback() and cancel_input_callback()
 *      -   ignore_midi_types()
 *      -   get_message()
 *      -   poll_for_midi()
 *      -   get_midi_event()
 *
 *  Output:
 *
 *      -   send_message() [two overloads]
 *      -   send_message (const midi::byte * message, size_t sz);
 *
 *  Both:
 *
 *      -   open_port()
 *      -   open_virtual_port()
 *      -   open_midi_api()
 *
 *  Service:
 *
 *      -   PPQN() and BPM()    [is_output() check, engine_initialize()]
 *      -   activate() and connect() [api_connect()]
 *
 *  More:
 *
 *      -   make_virtual_bus(), make_normal_bus()
 */

#include "midi/event.hpp"               /* midi::event class                */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */
#include "midi/bus_in.hpp"              /* midi::bus_in class               */
#include "midi/bus_out.hpp"             /* midi::bus_out class              */
#include "midi/track.hpp"               /* midi::track class                */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in port              */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out port             */
#include "xpc/automutex.hpp"            /* xpc::automutex                   */
#include "xpc/timing.hpp"               /* xpc::microsleep()                */

namespace midi
{

/**
 *  The masterbus default constructor has the following features:
 *
 *      -   It uses the rtl::rtmidi_engine to hold a client
 *          pointer for use by all of the ports. (The normal RtMidi
 *          paradigm is one client per port, it seems.)
 *      -   It fills the input and output arrays with the busses
 *          existing on the system.
 *
 *  Once constructed, if the caller wants to use the Seq66-derived
 *  buss feature, the caller should call masterbus::client_info_reset().
 *  Normally it is a null pointer.
 *
 * \param rapi
 *      The rtmidi API to use, either already vetted and selected, or
 *      rtl::rtmidi::api::unspecified.
 *
 * \param ppq
 *      Provides the PPQN value for this object.  However, in most cases, the
 *      default baseline PPQN should be specified.  Then the caller of this
 *      constructor should call masterbus::set_ppqn() to set up the proper
 *      PPQN value.
 *
 * \param bp
 *      Provides the beats per minute value, which defaults to
 *      c_beats_per_minute.
 */

masterbus::masterbus
(
    rtl::rtmidi::api rapi,
    midi::ppqn ppq,
    midi::bpm bp
) :
    m_selected_api          (rapi),         /* rtmidi::api::unspecified)    */
    m_inbus_array           (),
    m_outbus_array          (),
    m_dumping_input         (false),
    m_input_track           (nullptr),
    m_mutex                 (),
    m_void_client_handle    (nullptr),
    m_client_id             (0),
    m_max_busses            (c_busscount_max),
    m_client_info           (),
    m_ppqn                  (ppq),
    m_beats_per_minute      (bp),
    m_engine                (*this, rapi)   /* "mbus", keep client name     */
{
    // no code
}

/**
 *  Creates a copy of the given clientinfo object. The old clientinfo
 *  is removed, and a query of the existing ports is made.
 *
 *  Use this function to activate (or change) from using the
 *  original RtMidi API to using our Seq66-derived "midi::bus" API.
 *
 *  The normal usage of this function is to (1) set up the default
 *  clientinfo in the constructor or (2) get the global
 *  clientinfo settings from the application.
 *
 *  If the caller wants to see the ports, use the info reference
 *  returned by the client_info() function.
 */

bool
masterbus::client_info_reset ()
{
    clientinfo & ci { global_client_info() };
    return client_info_reset(ci);
}

bool
masterbus::client_info_reset (clientinfo & cinfo)
{
    m_client_info = cinfo;

    bool result { engine_query() };
    if (result)
        cinfo = m_client_info;                      /* return to the caller */

    return result;
}

/**
 *  Log the client handle with the masterbus and, for possible use
 *  elsewhere, in the clientinfo structure.
 */

void
masterbus::void_client_handle (void * clienthandle)
{
    m_void_client_handle = clienthandle;
    client_info().void_client_handle(clienthandle);

#if defined PLATFORM_DEBUG_TMI
    printf("masterbus client handle = %p\n", clienthandle);
#endif
}

/**
 *  Create the I/O info objects as necessary. Calling this function means
 *  that the application is going to share the masterbus object so that
 *  it can provide the MIDI engine handle itself.
 *
 *  If this function is not called, then the legacy RtMidi rules apply.
 *  But this function is called in the constructor, so that means that
 *  no masterbus was created.
 *
 *  This function works by creating temporary midi::bus_in (or midi::bus_out)
 *  objects in order to create the desired rtl::midi_api object for
 *  the engine, allowing us to get the MIDI client/engine handle for re-use.
 *
 * Other things we can do:
 *
 *      Set the queuesizelimit.
 *
 *  Note that we try/catch here.  Not fond of forcing users of the library
 *  to catch everything.
 *
 * \return
 *      Returns true if at least one I/O port has been found.
 */

bool
masterbus::engine_query ()
{
    bool result { client_info().get_all_port_info(selected_api()) };
    if (result)
    {
#if defined PLATFORM_DEBUG_TMI
        std::string msg { client_info().to_string("engine_query()") };
        infoprint(msg.c_str());
#endif
    }
    else
        errprint("get_all_port_info() failed");

    return result;
}

bool
masterbus::engine_connect ()
{
    return false;
}

/**
 *  Set the PPQN value (parts per quarter note). Then call the
 *  implementation-specific API function to complete the PPQN setting.
 *
 *  We do this even if the PPQN is not different from the nominal PPQN,
 *  to guarantee the change at startup.
 *
 *  TODO: do we want to use choose_ppqn(). Compare to
 *        mastermidibase::set_ppqn().
 *
 * \threadsafe
 *
 * \param ppqn
 *      The PPQN value to be set.
 */

bool
masterbus::PPQN (midi::ppqn ppq)
{
    xpc::automutex locker(m_mutex);
    bool result { engine().PPQN(ppq) };
    if (result)
        m_ppqn = ppq;

    return result;
}

/**
 *  Set the BPM value (beats per minute).  Then call the
 *  implementation-specific API function to complete the BPM setting.
 *
 *  We do this even if the BPM is not different from the nominal BPM,
 *  to guarantee the change at startup.
 *
 * \threadsafe
 *
 * \param bpm
 *      Provides the beats-per-minute value to set.
 */

bool
masterbus::BPM (midi::bpm bp)
{
    xpc::automutex locker(m_mutex);
    bool result { engine().BPM(bp) };
    if (result)
        m_beats_per_minute = bp;

    return result;
}

/**
 *  Initializes and activates the busses, in a partly API-dependent manner.
 *  Currently re-implemented only in the rtmidi JACK API.
 */

bool
masterbus::activate ()
{
    bool result { inbus_array().initialize() };
    if (result)
        result = outbus_array().initialize();

    if (result)
        set_client_id(outbus_array().client_id(0));

    return result;
}

bool
masterbus::flush ()
{
    xpc::automutex locker(m_mutex);
    return engine().flush();
}

bool
masterbus::flush_port (midi::bussbyte b)
{
    xpc::automutex locker(m_mutex);
    return engine().flush_port(b);
}

/**
 *  Stops all notes on all channels on all busses.  Adapted from Oli Kester's
 *  Kepler34 project.  Whether the buss is active or not is ultimately checked
 *  in the busarray::play() function.  A bit wasteful, but do we really care?
 */

bool
masterbus::panic (int displaybuss)
{
    xpc::automutex locker(m_mutex);
    bool result { true };
    for (int b = 0; b < c_busscount_max; ++b)
    {
        if (b == displaybuss)             /* do not clear the Launchpad   */
            continue;

        for (int channel = 0; channel < c_channel_max; ++channel)
        {
            for (int note = 0; note < c_byte_data_max; ++note)
            {
                event e(0, midi::status::note_off, channel, note, 0);
                m_outbus_array.send_event(b, &e, channel);
            }
        }
    }
    if (result)
        result = flush();

    return result;
}

/**
 *  Handle the sending of SYSEX events.  There's currently no
 *  implementation-specific API function for this call.
 *
 * \threadsafe
 *
 * \param ev
 *      Provides the event pointer to be set.
 */

bool
masterbus::sysex (midi::bussbyte b, const event * ev)
{
    xpc::automutex locker(m_mutex);
    bool result { not_nullptr(ev) };
    if (result)
        m_outbus_array.send_sysex(b, ev);

    return result;
}

/**
 *  Handle the playing of MIDI events on the MIDI buss given by the parameter,
 *  as long as it is a legal buss number.  There's currently no
 *  implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param b
 *      The actual system buss to start play on.  The caller is expected to
 *      make sure this buss is the correct buss.
 *
 * \param ev
 *      The seq66 event to play on the buss.  For speed, we don't bother to
 *      check the pointer.
 *
 * \param channel
 *      The channel on which to play the event.
 */

void
masterbus::play (midi::bussbyte b, event * e24, midi::byte channel)
{
    xpc::automutex locker(m_mutex);
    m_outbus_array.send_event(b, e24, channel);
}

void
masterbus::play_and_flush (midi::bussbyte b, event * ev, midi::byte channel)
{
    xpc::automutex locker(m_mutex);
    play(b, ev, channel);
    (void) flush();
}

/**
 *  Set the clock for the given (legal) buss number.  The legality checks
 *  are a little loose, however.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param b
 *      The actual system buss to start play on.  Checked before usage.
 *
 * \param clocktype
 *      The type of clock to be set, either "off", "pos", or "mod", as noted
 *      in the midibus_common module.
 */

bool
masterbus::set_clock (midi::bussbyte b, midi::clocking clocktype)
{
    xpc::automutex locker(m_mutex);
    bool result { m_outbus_array.set_clock(b, clocktype) };
    if (result)
    {
        flush();
        result = save_clock(b, clocktype);          /* save into the vector */
    }
    return result;
}

/**
 *  Saves the given clock value. This is a Seq66 concept, the clockslist,
 *  used in port-mapping, and is not implemented here yet. Instead,
 *  we modify the midi::port objects in the midi::ports class.
 *  Compare to mastermidibase::save_clock().
 *
 * \param b
 *      Provides the desired buss to be set. This must be an actual system
 *      buss, not a buss number from the output-port-map.
 *
 * \param clock
 *      Provides the clocking value to set.
 *
 * \return
 *      Returns true if the buss value is valid.
 */

bool
masterbus::save_clock (midi::bussbyte /*b*/, midi::clocking /*clk*/)
{
#if THIS_CODE_IS_READY
    bool result { m_master_clocks.set(b, clk) };
    if (! result)
    {
        int currentcount { m_master_clocks.count() };
        errprint("mmb::save_clock(): missing bus");
        for (int i = currentcount; i <= b; ++i)
        {
            clocking value { clocking::disabled };
            if (i == int(b))
            {
                value = clock;
                m_master_clocks.add(i, false, value, "Null clock");
            }
        }
    }
#endif
    return true;
}

/**
 *  Gets the clock setting for the given (legal) buss number.
 *
 * \param b
 *      Provides an actual system buss number to read.  Checked before usage.
 *
 * \return
 *      If the buss number is legal, and the buss is active, then its clock
 *      setting is returned.  Otherwise, e_clock::disabled is returned.
 */

midi::clocking
masterbus::get_clock (midi::bussbyte b) const
{
    return m_outbus_array.get_clock(b);
}

// TODO: perhaps implement:
//
// mastermidibase::copy_io_busses ()
// mastermidibase::get_port_statuses()
// mastermidibase::get_out_port_statuses()
// mastermidibase::get_in_port_statuses()
//      

/**
 *  Set the status of the given input buss, if a legal buss number.
 *
 * \threadsafe
 *
 * \param bs
 *      Provides the actual system buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true if the input buss array item could be set and then saved
 *      into the status container.
 */

bool
masterbus::set_input (midi::bussbyte b, bool inputing)
{
    xpc::automutex locker(m_mutex);
    bool result { m_inbus_array.set_input(b, inputing) };
    if (result)
    {
        result = flush();
        if (result)
            result = save_input(b, inputing);     /* save into the vector */
    }
    return result;
}

/**
 *  Saves the input status (as selected in the MIDI Input tab).  Now, we were
 *  checking this bus number against the size of the vector as gotten from the
 *  performer object, which it got the from the "rc" file's [midi-input]
 *  section.  However, the "rc" file won't necessarily match what is on the
 *  system now.  So we might have to adjust.
 *
 *  Do we also have to adjust the performer's vector?  What about the name of
 *  the buss?
 *
 * \param b
 *      Provides the actual system buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true, always.
 */

bool
masterbus::save_input (midi::bussbyte b, bool inputing)
{
    int currentcount { inbus_array().count() };
    bool result { inbus_array().set_input(b, inputing) };
    if (! result)
    {
        for (int i = currentcount; i <= b; ++i)
        {
#if THIS_CODE_IS_READY
            bool value { false };
            if (i == int(bs))
                value = inputing;

            m_master_inputs.add(i, value, "Why no name???");
#endif
        }
    }
    return result;          /* or true ? */
}

/**
 *  Get the input for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param b
 *      Provides the actual system buss number.
 *
 * \return
 *      Returns the value of the busarray::get_input(bus) call.
 */

bool
masterbus::get_input (midi::bussbyte b) const
{
    return m_inbus_array.get_input(b);
}

// TODO:
//
// mastermidibase::is_input_system_port (bussbyte bus) const
// mastermidibase::is_port_unavailable (bussbyte bus, midibase::io iotype) const
// mastermidibase::is_port_locked (bussbyte bus, midibase::io iotype) const

/**
 *  Get the MIDI input/output buss name for the given (legal) buss number.
 *  This function is used for display purposes, and is also written to the
 *  options ("rc") file.
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.
 *
 * \param b
 *      Provides the I/O buss number.
 *
 * \param iotype
 *      Indicates which I/O list is used for the lookup.
 *
 * \return
 *      Returns the buss name as a standard C++ string.  Also contains an
 *      indication that the buss is disconnected or unconnected.  If the buss
 *      number is illegal, this string is empty.
 */

std::string
masterbus::get_midi_bus_name
(
    midi::bussbyte b,
    midi::port::io iotype
) const
{
#if USE_THIS_SEQ66_CODE_FROM_PORTSLIST
    std::string result;
    portname p = rc().port_naming();
    if (iotype == midibase::io::input)
        result = m_master_inputs.get_display_name(b, p);
    else
        result = m_master_clocks.get_display_name(b, p);

    return result;
#else
    std::string result;
    if (iotype == midi::port::io::input)
        result = inbus_array().get_midi_bus_name(b);
    else
        result = outbus_array().get_midi_bus_name(b);

    return result;
#endif
}

/**
 *  Print some information about the available MIDI input and output busses.
 */

void
masterbus::print () const
{
    inbus_array().print();
    outbus_array().print();
}

/**
 *  Initiate a poll() on the existing poll descriptors.
 *  This base-class implementation could be made identical to
 *  portmidi's poll_for_midi() function, maybe.  But currently it is better
 *  just call the implementation-specific API function.
 *
 * \warning
 *      Do we need to use a mutex lock? No! It causes a deadlock!!!
 *
 * \return
 *      Returns the result of the poll, or 0 if the API is not supported.
 */

int
masterbus::poll_for_midi () const
{
    xpc::automutex locker(m_mutex);
    int result { inbus_array().poll_for_midi() };
    if (result > 0)
    {
        if (result <= 2)
            (void) xpc::microsleep(xpc::std_sleep_us());    /* sensible?    */
    }
    else
    {
        (void) xpc::microsleep(xpc::std_sleep_us());
    }
    return result;
}

bool
masterbus::get_midi_event (midi::event * inev)
{
    xpc::automutex locker(m_mutex);
    return engine().get_midi_event(inev);
}

/**
 * NOTE: In Seq66, we have midi_alsa_info::api_port_start() which gets
 *       port information and creates a midibus. Undefined for JACK.
 *
 *       See midi_alsa::get_midi_event().
 *
 *       TODO
 *
 *  Start the given MIDI port.  This function is called by
 *  api_get_midi_event() when the ALSA event SND_SEQ_EVENT_PORT_START is
 *  received.  Unlike port_exit(), the port_start() function does rely on
 *  API-specific code, so we do need to create a virtual api_port_start()
 *  function to implement the port-start event.
 *
 *  \threadsafe
 *      Quite a lot is done during the lock for the ALSA implimentation.
 *
 * \param client
 *      Provides the client number, which is actually an ALSA concept.
 *
 * \param port
 *      Provides the client port, which is actually an ALSA concept.
 */

bool
masterbus::port_start (int /*client*/, int /*port*/)
{
    xpc::automutex locker(m_mutex);

    // api_port_start(client, port);        // TODO

    return false;
}

/**
 *  Turn off the given port for the given client.  Both the input and output
 *  busses for the given client are stopped: that is, set to inactive.
 *
 *  This function is called by api_get_midi_event() when the ALSA event
 *  SND_SEQ_EVENT_PORT_EXIT is received.  Since port_exit() has no direct
 *  API-specific code in it, we do not need to create a virtual
 *  api_port_exit() function to implement the port-exit event.
 *
 * \threadsafe
 *
 * \param client
 *      The client to be matched and acted on.  This value is actually an ALSA
 *      concept.
 *
 * \param port
 *      The port to be acted on.  Both parameter must be matched before the
 *      buss is made inactive.  This value is actually an ALSA concept.
 */

bool
masterbus::port_exit (int client, int port)
{
    xpc::automutex locker(m_mutex);
    m_outbus_array.port_exit(client, port);
    m_inbus_array.port_exit(client, port);
    return false;
}

/**
 *  Set the input sequence object, and set the m_dumping_input value to
 *  the given state.
 *
 *  The portmidi version only sets m_input_track and m_dumping_input,
 *  but it seems like all the code below would apply to any masterbus.
 *
 *  -   qseqeditframe64::toggle_midi_rec() and _thru()
 *  -   sequence::set_input_recording() and _thru()
 *  -   performer::set_recording() and _thru()
 *
 * \threadsafe
 *
 * \param state
 *      Provides the dumping-input (recording) state to be set.  This value,
 *      as used in seqedit, can represent the state of the thru button or the
 *      record button.
 *
 * \param seq
 *      Provides the sequence pointer to be logged as the masterbus's
 *      current sequence.  Can also be used to set a null pointer, to disable
 *      the sequence setting.
 *
 * \return
 *      Returns true if the sequence pointer is not null.
 */

bool
masterbus::set_track_input (bool state, track * trk)
{
    xpc::automutex locker(m_mutex);
    bool result { not_nullptr(trk) };
    if (result)
    {
        /*
         * See mastermidbase for extenstion to this function.
         */

        if (state)
        {
            if (not_nullptr(m_input_track))
            {
                if (trk != m_input_track)
                    result = false;     /* We already got one! Is very nice */
            }
            else
            {
                m_dumping_input = state;
                m_input_track = trk;
            }
        }
        else
        {
            m_dumping_input = false;
            m_input_track = nullptr;
        }
    }
    return result;
}

#if THIS_CODE_IS_READY

/**
 *  This function augments the recording functionality by looking for a
 *  sequence that has a matching channel number, logging the event to that
 *  sequence, and then immediately exiting.  It should be called only if
 *  m_filter_by_channel is set.
 *
 *  If we have more than one sequence recording, and the channel-match feature
 *  [the sequence::channels_match() function] is disabled, then only the first
 *  sequence will get the events.  So now we add an additional call to the new
 *  sequence::channel_match() function.
 *
 * \param ev
 *      The event that was recorded, passed as a copy.  (Do we really need a
 *      copy?)
 */

void
masterbus::dump_midi_input (event /*& ev*/)
{
    size_t sz { m_vector_sequence.size() };
    for (size_t i = 0; i < sz; ++i)
    {
        if (is_nullptr(m_vector_sequence[i]))          // error check
        {
            errprint("dump_midi_input(): bad sequence");
            continue;
        }
        else if (m_vector_sequence[i]->stream_event(ev))
        {
            /*
             * Did we find a match to the sequence channel?  Then don't
             * bother with the remaining sequences.  Otherwise, pass the
             * event to any other recording sequences.
             */

            if (m_vector_sequence[i]->channel_match())
                break;
        }
    }
}
#endif

/**
 *  Dumps a list of the ports.
 */

std::string
masterbus::port_listing () const
{
    std::string result;
    if (client_info().empty())
    {
        result = "\nPorts: none\n";
    }
    else
    {
        result = client_info().port_list();
        result += "\n";
    }
    return result;
}

/*---------------------------------------------------------------------------
 * Virtual functions
 *---------------------------------------------------------------------------*/

/**
 *  [ Initializes and ] activates the busses, in a partly API-dependent manner.
 *  Currently implemented only in the rtmidi JACK API.
 *
 *  Compare to the legacy function masterbus::activate (). That function
 *  calls initialize() for each I/O midibus. The midibase::initialize() checks
 *  if the port is enabled. If so, it calls the inits for input vs output and
 *  real vs virtual ports. The midibus::api_init_in() function, as an example,
 *  creates a new rtmidi_in object (with "this" and master info). The rtmidi_in
 *  then gets the API pointer and calls its api_init_in(), which gets the port
 *  nameand then calls register_port().
 *
 *  Here, what exactly should we do?  Sometimes we'll have just one in and out
 *  port, or a whole array that we have to manage "optimally".
 */

bool
masterbus::engine_activate ()
{
    xpc::automutex locker(m_mutex);
    return engine().engine_activate();
}

/**
 *  The purpose of initialize() is to:
 *
 *      -   Set the PPQN value, including passing work to the selected
 *          rtl::rtmidi::api.
 *      -   Set the BPM value, including passing work to the selected
 *          rtl::rtmidi::api.
 *      -   Create any specified midi::bus objects, whether input/output or
 *          auto/virtual.  This base class supports ....
 *
 *  Compare it to the original, masterbus::api_init().
 *
 *  Note that we then need to propagate the new PPQN and BPM values to
 *  the ports, the engine, and the clientinfo.  Yeeeesh!
 *
 * \param ppq
 *      The new PPQN value.
 *
 * \param bp
 *      The new BPM value.
 */

bool
masterbus::engine_initialize ()
{
    bool result { client_info_reset() };
    if (result)
        result = engine_initialize(m_client_info);

    return result;
}

/**
 *  The port::io enum value can be input, output, duplex (the default),
 *  engine, and dummy. The masterbus requires duplex. OR ENGINE???
 *
 *  Should we also call client_info_reset() here?
 *
 *  Compare to mastermidibus::api_init (ppqn, bpm).
 */

bool
masterbus::engine_initialize (const clientinfo & ci)
{
    bool result { ci.port_type() == port::io::duplex };
    if (result)
    {
        result = PPQN(ci.global_ppqn());
        if (result)
            result = BPM(ci.global_bpm());

        if (result)
        {
            if (ci.virtual_ports())
            {
                // TODO
                //
                // Perhaps we should just add virtual port's information
                // to clientinfo at setup time. Also need to create
                // the set_virtual_name() function, or maybe have
                // a class virtualbus : public midibus
                //
                // Also don't forget to set ci.is_connected()
            }
            else
            {
                result = ci.ports_queried();
                if (result)
                {
                    bool swap_io
                    {
                        rtl::rtmidi::selected_api() == rtl::rtmidi::api::jack
                    };
                    bool isinput { ! swap_io };
                    midi::port::io iotype
                    {
                        isinput ?
                            midi::port::io::input : midi::port::io::output
                    };
                    int pcount { ci.port_count(iotype) };
                    midi::busarray & busarray_1
                    {
                        isinput ? inbus_array() : outbus_array()
                    };
                    for (int p = 0; p < pcount; ++p)
                    {
                        midi::bus * b = make_bus(p, iotype);
                        if (not_nullptr(b))
                        {
                            bool ok = busarray_1.add(b);  /* add unique_ptr */
                            if (! ok)
                            {
                                result = false;
                                break;
                            }
                        }
                        else
                            break;                        /* error          */
                    }
                    isinput = ! isinput;
                    iotype = isinput ?
                        midi::port::io::input : midi::port::io::output;

                    pcount = ci.port_count(iotype);
                    midi::busarray & busarray_2
                    {
                        isinput ? inbus_array() : outbus_array()
                    };
                    for (int p = 0; p < pcount; ++p)
                    {
                        midi::bus * b = make_bus(p, iotype);
                        if (not_nullptr(b))
                        {
                            bool ok = busarray_2.add(b);  /* add unique_ptr */
                            if (! ok)
                            {
                                result = false;
                                break;
                            }
                        }
                        else
                            break;                          /* error            */
                    }
                }
            }
        }
    }
    return result;
}

/**
 *  Creates a bus object, either midi::bus_in or midi::bus_out, and either
 *  virtual (manual) or normal. The bus constructors grab a lot of
 *  information about the ports from the ports list stored in the
 *  masterbus:
 *
 *      -   Bus ID. This is essentially an index number re 0.
 *      -   Port ID. For ALSA, this is ALSA's number for the port.
 *          For JACK, this is the same as the index number.
 *      -   Bus name. The system name for the bus.
 *      -   Port name The system name for the port.
 *      -   Port alias. In some recent JACK setups, a shorter port name.
 *      -   Port kind. Normal, virtual, or system ports.
 *
 * \param busno
 *      An index number re 0 for the bus. A port number, really.
 *
 * \param iotype
 *      Indicates if the bus represent a input port or an output.
 *      If duplex, engine, or dummy are specified, the bus is not created.
 *
 * \param iokind
 *      Indicates if the bus is a normal port, a manual (virtual) port, or
 *      a system port.
 *
 * \return
 *      A pointer to the created bus is returned, or a null pointer upon
 *      error.
 */

midi::bus *
masterbus::make_bus
(
    int busno,
    midi::port::io iotype
)
{
    midi::bus * result { nullptr };
    if (iotype == midi::port::io::input)
    {
        const unsigned qsize { 0 };                       /* TODO */
        result = new (std::nothrow) midi::bus_in(*this, busno, qsize);
    }
    else if (iotype == midi::port::io::output)
    {
        result = new (std::nothrow) midi::bus_out(*this, busno);
    }
    return result;
}

/*---------------------------------------------------------------------------
 * Virtual clock functions
 *---------------------------------------------------------------------------*/

bool
masterbus::handle_clock (midi::clock::action act, midi::pulse ts)
{
    bool result { ts >= 0 };
    if (result)
    {
        xpc::automutex locker(m_mutex);
        switch (act)
        {
            case midi::clock::action::init:

                m_outbus_array.init_clock(ts);
                break;

            case midi::clock::action::start:

                m_outbus_array.clock_start();
                break;

            case midi::clock::action::continue_from:

                m_outbus_array.clock_continue(ts);
                break;

            case midi::clock::action::stop:

                m_outbus_array.clock_stop();
                break;

            case midi::clock::action::emit:

                // TODO // m_outbus_array.clock(tick);
                break;

            default:

                result = false;
                break;
        }
    }
    return result;
}

}           // namespace midi

/*
 * masterbus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
