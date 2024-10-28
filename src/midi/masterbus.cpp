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
 * \updates       2023-07-20
 * \license       GNU GPLv2 or above
 *
 *  This file provides a base-class implementation for various master MIDI
 *  buss support classes.
 *
 *  The life-cycle of a midi::masterbus is something like this:
 *
 *      -   engine_query(). Create temporary I/O objects and use them
 *          to query the host for an existing API, such as ALSA or JACK.
 *          -   Gets the rtl::rtmidi::api to be used.
 *          -   Collects the I/O ports that exist into shared pointers for
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
#include "midi/track.hpp"               /* midi::track class                */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in port              */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out port             */
#include "xpc/automutex.hpp"            /* xpc::automutex                   */
#include "xpc/timing.hpp"               /* xpc::microsleep()                */

namespace midi
{

/**
 *  The masterbus default constructor fills the array with our busses.
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
 *
 * TODO: do we need a client name?
 */

masterbus::masterbus
(
    rtl::rtmidi::api rapi,
    midi::ppqn ppq,
    midi::bpm bp
) :
    m_selected_api      (rtl::rtmidi::api::unspecified),
    m_rt_api_ptr        (nullptr),
    m_engine            (this, rapi, "mbus"),
    m_mutex             (),
    m_client_handle     (nullptr),
    m_client_id         (0),
    m_max_busses        (c_busscount_max),
    m_client_info       (),
    m_ppqn              (ppq),
    m_beats_per_minute  (bp)
{
    (void) engine_query();
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
    if (! m_client_info)
        m_client_info.reset(new midi::clientinfo(midi::port::io::duplex));

    bool result = bool(client_info());
    if (result)
    {
        result = get_all_port_info(*client_info(), selected_api());
        if (result)
        {
#if defined PLATFORM_DEBUG
            std::string msg = client_info()->to_string("engine_query()");
            infoprint(msg.c_str());
#endif
        }
        else
            errprint("get_all_port_info() failed");
    }
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
 * \threadsafe
 *
 * \param ppqn
 *      The PPQN value to be set.
 */

bool
masterbus::PPQN (midi::ppqn ppq)
{
    xpc::automutex locker(m_mutex);
    bool result = m_ppqn != ppq;
    if (result)
    {
        m_ppqn = ppq;
        // api_set_ppqn(ppq);
    }
    return result;
}

/**
 *  Set the BPM value (beats per minute).  Then call the
 *  implementation-specific API function to complete the BPM setting.
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
    bool result = m_beats_per_minute != bp;
    if (result)
    {
        m_beats_per_minute = bp;
        // api_set_beats_per_minute(bp);
    }
    return result;
}

bool
masterbus::flush ()
{
    xpc::automutex locker(m_mutex);
    // api_flush();

    return true;        // TODO
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
    bool result = true;
    for (int bus = 0; bus < c_busscount_max; ++bus)
    {
        if (bus == displaybuss)             /* do not clear the Launchpad   */
            continue;

        for (int channel = 0; channel < c_channel_max; ++channel)
        {
            for (int note = 0; note < c_byte_data_max; ++note)
            {
                event e(0, midi::status::note_off, channel, note, 0);
    //              m_outbus_array.play(bus, &e, channel);
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
masterbus::sysex (midi::bussbyte bus, const event * ev)
{
    xpc::automutex locker(m_mutex);
    (void) bus;
    (void) ev;
    //  return m_outbus_array.sysex(bus, ev);
    return false;
}

/**
 *  Handle the playing of MIDI events on the MIDI buss given by the parameter,
 *  as long as it is a legal buss number.  There's currently no
 *  implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param bus
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
masterbus::play (midi::bussbyte bus, event * e24, midi::byte channel)
{
    xpc::automutex locker(m_mutex);
    (void) bus;
    (void) e24;
    (void) channel;
    // m_outbus_array.play(bus, e24, channel);
}

void
masterbus::play_and_flush (midi::bussbyte bus, event * ev, midi::byte channel)
{
    xpc::automutex locker(m_mutex);
    play(bus, ev, channel);
    flush();
}

/**
 *  Set the clock for the given (legal) buss number.  The legality checks
 *  are a little loose, however.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param bus
 *      The actual system buss to start play on.  Checked before usage.
 *
 * \param clocktype
 *      The type of clock to be set, either "off", "pos", or "mod", as noted
 *      in the midibus_common module.
 */

bool
masterbus::set_clock (midi::bussbyte /*bus*/, midi::clocking /*clocktype*/)
{
    xpc::automutex locker(m_mutex);
#if 0
    bool result = m_outbus_array.set_clock(bus, clocktype);
    if (result)
    {
        flush();
        result = save_clock(bus, clocktype);    /* save into the vector */
    }
    return result;
#else
    return false;
#endif
}

/**
 *  Saves the given clock value.
 *
 * \param bus
 *      Provides the desired buss to be set.  This must be an actual system
 *      buss, not a buss number from the output-port-map.
 *
 * \param clock
 *      Provides the clocking value to set.
 *
 * \return
 *      Returns true if the buss value is valid.
 */

bool
masterbus::save_clock (midi::bussbyte /*bus*/, midi::clocking /*clk*/)
{
    // return m_master_clocks.set(bus, cl);
    return false;
}

/**
 *  Gets the clock setting for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param bus
 *      Provides an actual system buss number to read.  Checked before usage.
 *
 * \return
 *      If the buss number is legal, and the buss is active, then its clock
 *      setting is returned.  Otherwise, e_clock::disabled is returned.
 */

midi::clocking
masterbus::get_clock (midi::bussbyte /*bus*/) const
{
    // return m_outbus_array.get_clock(bus);
    return midi::clocking::max;             // TODO. An illegal value
}

/**
 *  Set the status of the given input buss, if a legal buss number.
 *
 * \threadsafe
 *
 * \param bus
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
masterbus::set_input (midi::bussbyte /*bus*/, bool /*inputing*/)
{
    xpc::automutex locker(m_mutex);
#if 0
    bool result = m_inbus_array.set_input(bus, inputing);
    if (result)
    {
        flush();
        result = save_input(bus, inputing);     /* save into the vector */
    }
    return result;
#else
    return false;
#endif
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
 * \param bus
 *      Provides the actual system buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true, always.
 */

bool
masterbus::save_input (midi::bussbyte /*bus*/, bool /*inputing*/)
{
#if 0
    int currentcount = m_master_inputs.count();
    bool result = m_master_inputs.set(bus, inputing);
    if (! result)
    {
        for (int i = currentcount; i <= bus; ++i)
        {
            bool value = false;
            if (i == int(bus))
                value = inputing;

            m_master_inputs.add(i, value, "Why no name???");
        }
    }
    return result;          /* or true ? */
#else
    return false;
#endif
}

/**
 *  Get the input for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param bus
 *      Provides the actual system buss number.
 *
 * \return
 *      Returns the value of the busarray::get_input(bus) call.
 */

bool
masterbus::get_input (midi::bussbyte /*bus*/) const
{
    // return m_inbus_array.get_input(bus);
    return false;
}

/**
 *  Get the MIDI input/output buss name for the given (legal) buss number.
 *  This function is used for display purposes, and is also written to the
 *  options ("rc") file.
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.
 *
 * \param bus
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
masterbus::get_midi_bus_name (midi::bussbyte /*bus*/, midi::port::io /*iotype*/) const
{
#if 0
    std::string result;
    portname p = rc().port_naming();
    if (iotype == midibase::io::input)
        result = m_master_inputs.get_display_name(bus, p);
    else
        result = m_master_clocks.get_display_name(bus, p);

    return result;
#else
    return std::string("");
#endif
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

    // return api_poll_for_midi();

    (void) xpc::microsleep(xpc::std_sleep_us());
    return 0;
}

/**
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
masterbus::port_exit (int /*client*/, int /*port*/)
{
    xpc::automutex locker(m_mutex);

    // m_outbus_array.port_exit(client, port);
    // m_inbus_array.port_exit(client, port);

    return false;
}

/**
 *  Set the input sequence object, and set the m_dumping_input value to
 *  the given state.
 *
 *  The portmidi version only sets m_seq and m_dumping_input, but it seems
 *  like all the code below would apply to any masterbus.
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
masterbus::set_track_input (bool /*state*/, track * trk)
{
    xpc::automutex locker(m_mutex);
    bool result = not_nullptr(trk);
    return result;
}

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
#if 0
    size_t sz = m_vector_sequence.size();
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
#endif
}

/**
 *  Dumps a list of the ports.
 *
 *  TODO:  use client_info::port_list() instead???
 */

std::string
masterbus::port_listing () const
{
    std::string result;
    if (client_info())
    {
        if (client_info()->empty())
        {
            result = "\nPorts: none\n";
        }
        else
        {
            result = client_info()->port_list();
            result += "\n";
        }
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
masterbus::engine_initialize (midi::ppqn ppq, midi::bpm bp)
{
    PPQN(ppq);
    BPM(bp);

    /*
     * Set up I/O virtual/real midi::bus objects. See old mastermidibus ::
     * api_init().
     */

    return true;        // TODO
}

/**
 *  Creates the bus objects based on the I/O port numbers.  This base class
 *  version can create only one input port and one output port.
 *
 * \param autoconnect
 *      If true, the ports created are connected to the corresponding system
 *      ports.  Not supported in this base implementation.
 *
 * \param inputport
 *      Provides the number of the port to create.  The name of the port is
 *      based on the clientname.  A value of -1 means to create all of the input
 *      ports and perhaps auto-connect them.
 *
 * \param outputport
 *      Provides the number of the port to create.  The name of the port is
 *      based on the clientname.  A value of -1 means to create all of the
 *      output ports and perhaps auto-connect them.
 *
 * \return
 *      Returns true if the buses could be created (and autoconnected).
 */

bool
masterbus::engine_make_busses (bool autoconnect, int inputport, int outputport)
{
    bool result = ! autoconnect && outputport >= 0;
    if (result)
    {
        if (inputport >= 0)
        {
        }
        if (outputport >= 0)
        {
        }
    }
    return result;
}

/*---------------------------------------------------------------------------
 * Virtual clock functions
 *---------------------------------------------------------------------------*/

bool
masterbus::handle_clock (midi::clock::action act, midi::pulse ts)
{
    bool result = ts >= 0;
    if (result)
    {
        xpc::automutex locker(m_mutex);
        switch (act)
        {
            case midi::clock::action::init:

//              api_init_clock(tick);
//              m_outbus_array.init_clock(tick);
                break;

            case midi::clock::action::start:

//              api_start();
//              m_outbus_array.start();
                break;

            case midi::clock::action::continue_from:

//              api_continue_from(tick);
//              m_outbus_array.continue_from(tick);
                break;

            case midi::clock::action::stop:

//              m_outbus_array.stop();
//              api_stop();
                break;

            case midi::clock::action::emit:

//              m_outbus_array.clock(tick);
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

