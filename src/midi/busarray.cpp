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
 * \file          busarray.cpp
 *
 *  This module declares/defines an "array" of MIDI busses.
 *  the ALSA system.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2024-06-02
 * \updates       2025-08-28
 * \license       GNU GPLv2 or above
 *
 *  This file provides a base-class implementation for various master MIDI
 *  buss support classes.  There is a lot of common code between these MIDI
 *  buss classes.
 */

#include "midi/bus.hpp"                 /* midi::bus, clocking              */
#include "midi/busarray.hpp"            /* rtl66::busarray class            */
#include "midi/event.hpp"               /* rtl66::event class               */

namespace midi
{

/*
 * -------------------------------------------------------------------------
 * class busarray::container defined for the pimpl paradigm.
 * -------------------------------------------------------------------------
 */

class busarray::container
{

private:

    /**
     *  The bus::pointer is a unique_ptr<> defined in the bus.hpp module.
     */

    using busses = std::vector<midi::bus::pointer>;

    /**
     *  The container of bus pointers.
     */

    busses m_bus_container;

public:

    container () : m_bus_container ()
    {
        // no code
    }

    ~container ()
    {
        // no code
    }

    int count () const
    {
        return int(m_bus_container.size());
    }

    /**
     *  This container takes ownership of the pointer provided by the
     *  caller. We need to be careful of lifetimes here. Also note
     *  that the bus will already have gotten its data from the
     *  masterbus.
     */

    bool add (midi::bus * b)
    {
        bool result { not_nullptr(b) };
        if (result)
        {
            bus::pointer bp { b };
            m_bus_container.push_back(std::move(bp));
        }
        return result;
    }

    bool bus_valid (midi::bussbyte b) const
    {
        return b < midi::bussbyte(m_bus_container.size());
    }

    midi::bus * bus_ptr (midi::bussbyte b)
    {
        return bus_valid(b) ? m_bus_container[b].get() : nullptr ;
    }

    const midi::bus * bus_ptr (midi::bussbyte b) const
    {
        return bus_valid (b) ? m_bus_container[b].get() : nullptr ;
    }

    bool initialize ()
    {
        bool result { true };
        for (auto & b : m_bus_container)
        {
            if (! b->initialize())
                result = false;
        }
        return result;
    }

    void clock_start ()
    {
        for (auto & b : m_bus_container)
            b->clock_start();
    }

    void clock_stop ()
    {
        for (auto & b : m_bus_container)
            b->clock_start();
    }

    void clock_continue (pulse tick)
    {
        for (auto & b : m_bus_container)
            b->clock_continue(tick);
    }

    void init_clock (pulse tick)
    {
        for (auto & b : m_bus_container)
            b->init_clock(tick);
    }

    void set_clock (clocking clocktype)
    {
        for (auto & b : m_bus_container)
            b->set_clock(clocktype);
    }

    void print () const
    {
        for (const auto & b : m_bus_container)
            b->print();
    }

    void port_exit (int client, int p)
    {
        for (auto & b : m_bus_container)
        {
            if (b->match(client, p))
               b->deactivate();
        }
    }

    void set_all_inputs (bool inputing)
    {
        for (auto & b : m_bus_container)
            b->init_input(inputing);
    }

    int poll_for_midi () const
    {
        int result { 0 };
        for (auto & b : m_bus_container)
        {
            result = b->poll_for_midi();
            if (result > 0)
                break;
        }
        return result;
    }

    bool get_midi_event (event * inev)
    {
        for (auto & b : m_bus_container)
        {
            if (b->get_midi_event(inev))
            {
                bussbyte p = bussbyte(b->bus_index());
                inev->set_input_bus(p);
#if defined PLATFORM_DEBUG_TMI
                printf("[rtl66] input event on bus %d\n", int(p));
#endif
                return true;
            }
        }
        return false;
    }

    int replacement_port (int b, int p)
    {
        int result { -1 };
        int counter { 0 };
        for (auto & bs : m_bus_container)
        {
            if (bs->match(b, p) && ! bs->active())
            {
                result = counter;
                if (bool(bs))
                    errprint("port_start(): bus out not null");

                break;
            }
            ++counter;
        }
        return result;
    }

};          // class busarray::container

/*
 * -------------------------------------------------------------------------
 * class busarray
 * -------------------------------------------------------------------------
 */

/**
 *  Checks that the bus pointer is good and the bus is enabled.
 */

static bool
bus_active (midi::bus * bptr)
{
    return not_nullptr(bptr) && bptr->port_enabled();
}

/**
 *  A new class to hold a vector of MIDI busses and flags for more controlled
 *  access than using arrays of booleans and pointers.
 */

busarray::busarray () :
    p_impl      { std::make_unique<busarray::container>() }
{
    // Empty body
}

/**
 *  We can't define this destructor in the header because it leads to
 *  the error "invalid application of ‘sizeof’ to incomplete type"
 */

busarray::~busarray ()
{
    // no code
}

/**
 *  Creates and adds a new midi::bus object to the list.  Then the clock value
 *  is set. This function is meant for output ports.
 *
 *  We need to belay the initialization until later, when we know the
 *  configured clock settings for the output ports.  So initialization has
 *  been removed from the constructor and moved to the initialize() function.
 *
 * \param b
 *      The midi::bus to be hooked into the array of busses.
 *
 * \return
 *      Returns true if the bus was added successfully, though, really, it
 *      cannot fail.
 */

bool
busarray::add (midi::bus * b)
{
    return p_impl->add(b);
}

#if 0

/**
 *  Adds a new midi::bus object to the list.  Then the inputing value
 *  is set.  This function is meant for input ports.
 *
 *  We need to belay the initialization until later, when we know the
 *  configured inputing settings for the input ports.  So initialization
 *  has been removed from the constructor and moved to the initialize()
 *  function. However, now we know the configured status and can apply
 *  it right away.
 *
 * \param b
 *      The midi::bus to be hooked into the array of busses.
 *
 * \param inputing
 *      The input flag value for the bus.  If true, this value indicates that
 *      the user has selected this bus to be the input MIDI bus.
 *
 * \return
 *      Returns true if the bus was added successfully, though, really, it
 *      cannot fail.
 */

bool
busarray::add (midi::bus * b, bool inputing)
{
    clocking c = bool_to_clocking(inputing);
    return add(b, c);
}

#endif

int
busarray::client_id (midi::bussbyte b)
{
    midi::bus * bptr { p_impl->bus_ptr(b) };
    return not_nullptr(bptr) ? bptr->client_id() : (-1) ;
}

/**
 *  Checks that a specific port is enabled. Compare to the static function
 *  bus_active().
 */

bool
busarray::port_active (midi::bussbyte b)
{
    midi::bus * bptr { p_impl->bus_ptr(b) };
    return bus_active(bptr);
}

/**
 *  Initializes all busses.  Not sure we need this function.
 *
 * \return
 *      Returns true if all busses initialized successfully.  It currently
 *      keeps going even after a failure, though.
 */

bool
busarray::initialize ()
{
    return p_impl->initialize();
}

/**
 *  Starts all of the busses; used for output busses only, but no check is
 *  made at present.
 *
 *  Hmmm, is this REALLY a bus-specific action? Same for the below?
 */

void
busarray::clock_start ()
{
    p_impl->clock_start();
}

/**
 *  Stops all of the busses; used for output busses only, but no check is
 *  made at present.
 */

void
busarray::clock_stop ()
{
    p_impl->clock_stop();
}

/**
 *  Continues from the given tick for all of the busses; used for output
 *  busses only.
 *
 * \param tick
 *      Provides the tick value for all busses to continue from.
 */

void
busarray::clock_continue (pulse tick)
{
    p_impl->clock_continue(tick);
}

/**
 *  Initializes the clocking at the given tick for all of the busses; used
 *  for output busses only.
 *
 * \param tick
 *      Provides the tick value for all busses use as the clock tick.
 */

void
busarray::init_clock (pulse tick)
{
    p_impl->init_clock(tick);
}

/**
 *  Sets the clock type for all busses, usually the output buss. Note that
 *  the settings to apply are added when the add() call is made. This is a
 *  bit ugly.
 */

void
busarray::set_clock (clocking clk)
{
    p_impl->set_clock(clk);
}

/**
 *  Plays an event, if the bus is proper.
 *
 * \param b
 *      The MIDI buss on which to play the event. The buss number must be
 *      valid (in range) and the bus must be active.
 *
 * \param e24
 *      A pointer to the event to be played. Currently we don't bother to
 *      check it!
 *
 * \param channel
 *      The MIDI channel on which to play the event.  Seq66 controls
 *      the actual channel of playback, no matter what the channel specified
 *      in the event.
 */

void
busarray::send_event (bussbyte b, const event * e24, midi::byte channel)
{
    midi::bus * bptr { p_impl->bus_ptr(b) };
    if (bus_active(bptr))
        bptr->send_event(e24, channel);
}

/**
 *  Handles SysEx events; used for output busses.
 *
 * \param ev
 *      Provides the SysEx event to handle.
 */

void
busarray::send_sysex (bussbyte b, const event * e24)
{
    midi::bus * bptr { p_impl->bus_ptr(b) };
    if (bus_active(bptr))
        bptr->send_sysex(e24);
}

/**
 *  Sets the clock type for the given bus, usually the output buss.
 *  This code is a bit more restrictive than the original code in
 *  mastermidi::bus::set_clock().
 *
 *  Getting the current clock setting is essentially equivalent to:
 *
 *      m_bus_container[bus].bus()->get_clock();
 *
 *  The check for a change in status is commented out because it
 *  can disable setting values for the same item as stored in the portmap.
 *  Need to investigate this at some point.
 *
 * \param b
 *      The MIDI bus for which the clock is to be set.
 *
 * \param clocktype
 *      Provides the type of clocking for the buss.
 *
 * \return
 *      Returns true if the change was made.  It is made only if needed.
 */

bool
busarray::set_clock (bussbyte b, clocking clocktype)
{
    clocking current { get_clock(b) };
    midi::bus * bptr { p_impl->bus_ptr(b) };
    bool result { bus_active(bptr) };
    if (! result)
        result = current == clocking::disabled;

    if (result)
        bptr->set_clock(clocktype);             /* also handles set_clock() */

    return result;
}

/**
 *  Gets the clock type for the given bus; really applies only to an
 *  output buss.
 *
 * \param b
 *      The MIDI bus for which the clock is to be set.
 *
 * \return
 *      Returns the clock value set for the desired buss. If the buss is
 *      invalid, clocking::unavailable is returned.  If the buss is not
 *      active, we still get the existing clock value.  The theory here is
 *      that we don't want to junk the current clock value; it could alter
 *      what was read from the "rc" file.
 */

clocking
busarray::get_clock (bussbyte b) const
{
    midi::bus * bptr { p_impl->bus_ptr(b) };
    return bus_active(bptr) ?
        bptr->clock_type() : clocking::unavailable ;
}

/**
 *  Get the MIDI output buss name (i.e. the full display name) for the given
 *  (legal) buss number.
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.  It's main use is to display the
 *  full portname in one of two forms:
 *
 *      -   "[0] 0:0 clientname:portname"
 *      -   "[0] 0:0 portname"
 *
 *  The second version is chosen if "clientname" is already included in the
 *  port name, as many MIDI clients do that.  However, the name gets
 *  modified to reflect the remote system port to which it will connect.
 *
 * \param b
 *      Provides the output buss number.  Checked before usage.
 *      Actually is now be an index number.
 *
 * \return
 *      Returns the buss name as a standard C++ string, truncated to 80-1
 *      characters.  Also contains an indication that the buss is disconnected
 *      or unconnected.  If the buss number is illegal, this string is empty.
 */

std::string
busarray::get_midi_bus_name (int b) const
{
    std::string result;
    const midi::bus * bptr { p_impl->bus_ptr(b) };
    if (not_nullptr(bptr))
    {
        clocking current { bptr->clock_type() };
        if (bptr->port_enabled() || current == clocking::disabled)
        {
            std::string busname { bptr->bus_name() };
            std::string portname { bptr->port_name() };
            std::size_t len { busname.size() };
            int test { busname.compare(0, len, portname, 0, len) };
            if (test == 0)
            {
                char tmp[80];
                snprintf
                (
                    tmp, sizeof tmp, "[%d] %d:%d %s",
                    b, bptr->bus_id(), bptr->port_id(), portname.c_str()
                );
                result = tmp;
            }
            else
                result = bptr->display_name();
        }
        else
        {
            /*
             * The display name gets saved, and so must be used unaltered
             * here.
             */

            result = bptr->display_name();
        }
    }
    return result;
}

/**
 *  The function gets the port name.  We're trying to keep our own client name
 *  (normally "rtl66") out of the 'rc' input and clock sections.
 */

std::string
busarray::get_midi_port_name (int b) const
{
    std::string result;
    const midi::bus * bptr { p_impl->bus_ptr(b) };
    if (not_nullptr(bptr))
        result = bptr->port_name();

    return result;
}

/**
 *  This function gets only the alias name of the port, if it exists.
 *  Some (later) versions of JACK support getting the alias in the manner
 *  of "a2jmidid --export-hw", which can be used to use the device's model name
 *  rather that some generic name.
 */

std::string
busarray::get_midi_alias (int b) const
{
    std::string result;
    const midi::bus * bptr { p_impl->bus_ptr(b) };
    if (not_nullptr(bptr))
        result = bptr->port_alias();

    return result;
}

/**
 *  Print some information about the available MIDI input or output busses.
 */

void
busarray::print () const
{
    printf("Available busses:\n");
    p_impl->print();
}

/**
 *  Turn off the given port for the given client.  Both the busses for the given
 *  client are stopped: that is, set to inactive.
 *
 *  This function is called by api_get_midi_event() when the ALSA event
 *  SND_SEQ_EVENT_PORT_EXIT is received.  Since port_exit() has no direct
 *  API-specific code in it, we do not need to create a virtual
 *  api_port_exit() function to implement the port-exit event.
 *
 * \param client
 *      The client to be matched and acted on.  This value is actually an ALSA
 *      concept.
 *
 * \param p
 *      The port to be acted on.  Both parameter must be matched before the
 *      buss is made inactive.  This value is actually an ALSA concept.
 */

void
busarray::port_exit (int client, int p)
{
    p_impl->port_exit(client, p);
}

/**
 *  Set the status of the given input buss, if a legal buss number.  There's
 *  currently no implementation-specific API function called directly here.
 *  What happens is that midibase::set_input() uses the \a inputing parameter
 *  to decide whether to call init_in() or deinit_in(), and these functions
 *  ultimately lead to an API specific called.
 *
 *  Note that the call to midibase::set_input() will set its m_inputing flag,
 *  and then call init_in() or deinit_in() if that flag changed. This change
 *  is important, so we have to call midibase::set_input() first. Then the
 *  call to busarray::init_input() will set that flag again (plus another
 *  flag).  A bit confusing in sequence and in function naming.
 *
 *  This function should be used only for the input busarray, obviously.
 *
 *  The check for a change in status is not done because it
 *  can disable setting values for the same item as stored in the portmap.
 *  Need to investigate this at some point.
 *
 * \threadsafe
 *
 * \param b
 *      Provides the buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true if the buss number is valid and was active, and so could
 *      be set.
 */

bool
busarray::set_input (bussbyte b, bool inputing)
{
    midi::bus * bptr { p_impl->bus_ptr(b) };
    bool result { not_nullptr(bptr) };
    if (result)
    {
        bool current { get_input(b) };                      /* see below    */

        /*
         *  The init_input() call here first sets the m_init_input flag in
         *  busarrayr.  Then it sets the I/O status flag of the midi::bus. It
         *  does this only if the busarray is active (i.e. initialized) and the
         *  status has changed.
         */

        result = bptr->active() || ! current;
        if (result)
            bptr->init_input(inputing);
    }
    return result;
}

/**
 *  Set the status of all input busses.  There's no implementation-specific
 *  API function here.  This function should be used only for the input
 *  busarray, obviously.  Note that the input settings used here were stored
 *  when the add() function was called.  They can be changed by the user via
 *  the Options / MIDI Input tab.
 */

void
busarray::set_all_inputs (bool inputing)
{
    p_impl->set_all_inputs(inputing);
}

/**
 *  Get the input for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param b
 *      Provides the buss number.
 *
 * \return
 *      If the buss is a system buss, always returns true.  Otherwise, if the
 *      buss is inactive, returns false. Otherwise, the buss's port_enabled()
 *      status is returned.
 */

bool
busarray::get_input (bussbyte b) const
{
    const midi::bus * bptr { p_impl->bus_ptr(b) };
    bool result { not_nullptr(bptr) };
    if (result)
    {
        if (bptr->active())
            result = bptr->is_system_port() ? true : bptr->port_enabled();
    }
    return result;
}

/**
 *  Get the system-port status for the given (legal) buss number.
 *
 * \param b
 *      Provides the buss number.
 *
 * \return
 *      Returns the selected buss's is-system-port status.  If the buss number
 *      is out of range, then false is returned.
 */

bool
busarray::is_system_port (bussbyte b) const
{
    const midi::bus * bptr { p_impl->bus_ptr(b) };
    bool result { not_nullptr(bptr) };
    if (result)
    {
        if (bptr->active())
            result = bptr->is_system_port();
    }
    return result;
}

bool
busarray::is_port_unavailable (bussbyte b) const
{
    bool result { true };
    const midi::bus * bptr { p_impl->bus_ptr(b) };
    if (not_nullptr(bptr))
        result = bptr->port_unavailable();

    return result;
}

/**
 *  Modified to account for the fact that if a port has not been
 *  added to the array, it cannot be locked.  This check is made by
 *  comparing the bus to the count().
 *
 * \param b
 *      The buss/port number.
 */

bool
busarray::is_port_locked (bussbyte b) const
{
    const midi::bus * bptr { p_impl->bus_ptr(b) };
    bool result { not_nullptr(bptr) };
    if (result)
        result = bptr->is_port_locked();

    return result;
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a primitive
 *  poll, which exits when some data is obtained.  It also applies only to the
 *  input busses.
 *
 *  One issue is that we have no way of knowing here which MIDI input device
 *  has MIDI input events waiting.  Should we randomize the order of polling
 *  in order to avoid starving some input devices?
 *
 * \return
 *      Returns the number of MIDI events detected on one of the busses.  Note
 *      that this is no longer a boolean value.
 */

int
busarray::poll_for_midi () const
{
    return p_impl->poll_for_midi();
}

/**
 *  Gets the first MIDI event in finds on an input bus.
 *
 *  Note that this function risks starving the second input device if more
 *  than one is enabled in Seq66.  We will figure that one out later.
 *
 * \param inev
 *      A pointer to the event to be modified by incoming data, if any.
 *
 * \return
 *      Returns true if an event's data was copied into the event pointer.
 */

bool
busarray::get_midi_event (event * inev)
{
    return p_impl->get_midi_event(inev);
}

/**
 *  Provides a function to use in api_port_start(), to determine if the port
 *  is to be a "replacement" port.  This function is meant only for the output
 *  buss (so far).
 *
 *  Still need to determine exactly what this function needs to do.
 *
 * \param b
 *      The buss to be affected.
 *
 * \param port
 *      The prot to be affected.
 *
 * \return
 *      Returns -1 if no matching port is found, otherwise it returns the
 *      replacement-port number.
 */

int
busarray::replacement_port (int b, int p)
{
    return p_impl->replacement_port(b, p);
}

int
busarray::count () const
{
    return p_impl->count();
}

bool
busarray::bus_valid (midi::bussbyte b) const
{
    return p_impl->bus_valid(b);
}

midi::bus &
busarray::buss (midi::bussbyte b)
{
    static midi::bus s_dummy_bus;
    midi::bus * bptr { p_impl->bus_ptr(b) };
    bool good_busnumber { not_nullptr(bptr) };
    return good_busnumber ? *bptr : s_dummy_bus ;
}

/**
 *  This free function swaps the contents of two busarray objects.
 *
 * \param buses0
 *      Provides the first buss in the swap.
 *
 * \param buses1
 *      Provides the second buss in the swap.
 */

#if defined THIS_CODE_IS_READY

void
swap (bus & buses0, bus & buses1)
{
    bus temp { buses0 };
    buses0 = buses1;
    buses1 = temp;
}

#endif

}           // namespace midi

/*
 * busarray.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
