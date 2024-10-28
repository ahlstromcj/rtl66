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
 * \file          midi_macosx_core.cpp
 *
 *    Message about this module...
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-03-17
 * \license       See above.
 *
 */

#include "rtl/midi/macosx/midi_macosx_core.hpp" /* rtl::midi_core classes   */

#if defined RTL66_PLATFORM_MACOSX

#include "midi/eventcodes.hpp"              /* midi::status enum, etc.      */

namespace rtl
{

/**
 *  Mac OSC Core MIDI detection function. TODO!
 */

bool
detect_core ()
{
    return false;
}

/**
 *  The CoreMIDI API is based on the use of a callback function for MIDI input.
 *  We convert the system specific time stamps to delta time values.  Some
 *  headers/functions are not available on iOS.
 */

#if defined RTL66_BUILD_IPHONE_OS

#define AUDIO_GET_HOST_TIME CAHostTimeBase::GetCurrentTime
#define AUDIO_CONVERT_TO_NANOSECONDS CAHostTimeBase::ConvertToNanos

#include <mach/mach_time.h>

class ctime_2ns_factor
{
public:

    ctime_2ns_factor ()
    {
        mach_timebase_info_data_t tinfo;
        mach_timebase_info(&tinfo);
        Factor = double(tinfo.numer) / tinfo.denom;
    }
    static double Factor;
};

double ctime_2ns_factor::Factor;
static ctime_2ns_factor InitTime2nsFactor;

#undef AUDIO_GET_HOST_TIME
#undef AUDIO_CONVERT_TO_NANOSECONDS

#define AUDIO_GET_HOST_TIME (uint64_t) mach_absolute_time
#define AUDIO_CONVERT_TO_NANOSECONDS(t) t * ctime_2ns_factor::Factor
#define EndianS32_BtoN(n) n

#else

#include <CoreAudio/HostTime.h>
#include <CoreServices/CoreServices.h>

#endif  // defined RTL66_BUILD_IPHONE_OS

/**
 *  A structure to hold variables related to the CoreMIDI API
 *  implementation.
 */

struct CoreMidiData
{
    MIDIClientRef client;
    MIDIPortRef port;
    MIDIEndpointRef endpoint;
    MIDIEndpointRef destinationId;
    unsigned long long lastTime;
    MIDISysexSendRequest sysexreq;
};

static MIDIClientRef CoreMidiClientSingleton = 0;

void
rtmidi_set_core_midi_singleton (MIDIClientRef client)
{
    CoreMidiClientSingleton = client;
}

void
rtmidi_dispose_core_midi_singleton ()
{
    if (CoreMidiClientSingleton != 0)
    {
        MIDIClientDispose(CoreMidiClientSingleton);
        CoreMidiClientSingleton = 0;
    }
}

/**
 *  My interpretation of the CoreMIDI documentation: all message
 *  types, except sysex, are complete within a packet and there may
 *  be several of them in a single packet.  Sysex messages can be
 *  broken across multiple packets and PacketLists but are bundled
 *  alone within each packet (these packets do not contain other
 *  message types).  If sysex messages are split across multiple
 *  MIDIPacketLists, they must be handled by multiple calls to this
 *  function.
 */

static void
midi_input_callback (const MIDIPacketList * list, void * procref, void /**srcRef*/)
{
    using namespace midi;               /* midi::byte(), midi::status, etc. */

    rtmidi_in_data * data = static_cast<rtmidi_in_data *>(procref);
    CoreMidiData * apidata = static_cast<CoreMidiData *>(data->api_data());
    midi::byte status;
    unsigned short nbytes, ibyte, size;
    unsigned long long time;
    bool & moresysex = data->continue_sysex();
    midi_in_api::midi_message & message = data->message();
    const MIDIPacket * packet = &list->packet[0];
    for (unsigned i = 0; i < list->numPackets; ++i)
    {
        nbytes = packet->length;
        if (nbytes == 0)
        {
            packet = MIDIPacketNext(packet);
            continue;
        }
        if (data->first_message())      /* calculate time stamp             */
        {
            message.time_stamp(0.0);
            data->first_message(false);
        }
        else
        {
            time = packet->timeStamp;
            if (time == 0)              /* for getting asynch sysex msgs    */
                time = AUDIO_GET_HOST_TIME();

            time -= apidata->lastTime;
            time = AUDIO_CONVERT_TO_NANOSECONDS(time);
            if (! moresysex)
                message.time_stamp(time * 0.000000001);
        }

        /*
         * Track whether any non-filtered messages were found in this
         * packet for timestamp calculation
         */

        bool found_nonfiltered = false;
        ibyte = 0;
        if (moresysex)                  // continuing, segmented sysex
        {
            if (data->allow_sysex())
            {
                /* If not ignoring sysex messages, copy entire packet       */

                for (unsigned j = 0; j < nbytes; ++j)
                    message.bytes.push_back(packet->data[j]);
            }
            moresysex = ! is_sysex_end(packet->data[nbytes-1])  // 0xF7

            // If not a continuing sysex message, invoke the user callback
            // function or queue the message.

            if (data->allow_sysex() && ! moresysex)
            {
                if (data->using_callback())
                {
                    rtmidi_in_data::callback_t cb = data->user_callback();
                    cb
                    (
                        message.time_stamp(), &message.data(),
                        data->user_data()
                    );
                }
                else
                {
                    // As long as we haven't reached our queue size limit,
                    // push the message.

                    if (! data->queue().push(message))
                        error_print("midi_core_in", "queue limit reached");
                }
                message.bytes.clear();
            }
        }
        else
        {
            while (ibyte < nbytes)
            {
                // We are expecting that the next byte in the packet is
                // a status byte.

                size = 0;
                status = packet->data[ibyte];
                if (! is_status(status))        // ! (status & 0x80)
                    break;

                /*
                 * Determine the number of bytes in the MIDI message.
                 */

                /*
                 * TODO
                 *
                 * size = (unsigned short) status_msg_size(status);
                 */

                if (status < 0xC0)
                    size = 3;
                else if (status < 0xE0)
                    size = 2;
                else if (status < 0xF0)
                    size = 3;
                else if (is_sysex(status))      // 0xF0
                {
                    if (! data->allow_sysex())
                    {
                        size = 0;
                        ibyte = nbytes;
                    }
                    else
                        size = nbytes - ibyte;

                    moresysex = ! is_sysex_end(packet->data[nbytes-1]); // 0xF7
                }
                else if (status == 0xF1)    // A MIDI time code message
                {
                    if (! data->allow_sysex())
                    {
                        size = 0;
                        ibyte += 2;
                    }
                    else
                        size = 2;
                }
                else if (status == 0xF2)
                    size = 3;
                else if (status == 0xF3)
                    size = 2;
                else if (status == 0xF8 && ! data->allow_time_code())
                {

                    size = 0;   /* MIDI timing tick message; ignoring it    */
                    ++ibyte;
                }
                else if (status == 0xFE && ! data->allow_active_sensing())
                {

                    size = 0;   /* MIDI active sensing; ignoring it         */
                    ++ibyte;
                }
                else
                    size = 1;

                if (size > 0)   // Copy the MIDI data to our vector.
                {
                    found_nonfiltered = true;
                    message.bytes.assign
                    (
                        &packet->data[ibyte], &packet->data[ibyte+size]
                    );
                    if (! moresysex)
                    {
                        // If not a continuing sysex message, invoke the user
                        // callback function or queue the message.

                        if (data->usingCallback)
                        {
                            rtmidi_in_data::callback_t cb =
                                data->user_callback();

                            cb
                            (
                                message.timestamp(), &message.data(),
                                data->user_data()
                            );
                        }
                        else
                        {
                            // As long as we haven't reached our queue size limit,
                            // push the message.

                            if (! data->queue.push( message ) )
                            error_print("midi_core_in", "queue limit reached");
                        }
                        message.bytes.clear();
                    }
                    ibyte += size;
                }
            }
        }
        if (found_nonfiltered)   // Save time of the last non-filtered message
        {
            apidata->lastTime = packet->timeStamp;

            // this happens when receiving asynchronous sysex messages

            if (apidata->lastTime == 0 )
            {
                apidata->lastTime = AUDIO_GET_HOST_TIME();
            }
        }
        packet = MIDIPacketNext(packet);
    }
}

/*------------------------------------------------------------------------
 * midi_core_in
 *------------------------------------------------------------------------*/

midi_core_in::midi_core_in
(
    const std::string & clientname, unsigned queuesizelimit
) :
    midi_in_api (queuesizelimit)
{
    midi_core_in::initialize(clientname);
}

midi_core_in::~midi_core_in ()
{
    midi_core_in::close_port(); // Close a connection if it exists.

    CoreMidiData * data = static_cast<CoreMidiData *>(m_api_data); // Cleanup.
    if (data->endpoint)
        MIDIEndpointDispose(data->endpoint);

    delete data;
}

MIDIClientRef
midi_core_in::get_core_midi_client_singleton
(
    const std::string & clientname
) noexcept
{
    if (CoreMidiClientSingleton == 0)
    {
        MIDIClientRef client;   // Set up our client.
        CFStringRef name = CFStringCreateWithCString
        (
            NULL, clientname.c_str(), kCFStringEncodingASCII
        );
        OSStatus result = MIDIClientCreate(name, NULL, NULL, &client);
        if (result != noErr)
        {
            std::ostringstream ost;
            ost << "midi_core_in::initialize: "
                << "error creating OS-X MIDI client object (" << result << ")."
                ;
            m_error_string = ost.str();
            error(rterror::kind::driver_error, m_error_string);
            return 0;
        }
        CFRelease (name);
        CoreMidiClientSingleton = client;
    }
    return CoreMidiClientSingleton;
}

/**
 *  Set up the client, then save the api-specific connection information.
 */

bool
midi_core_in::initialize (const std::string & clientname)
{
    MIDIClientRef client = get_core_midi_client_singleton(clientname);
    CoreMidiData * data = (CoreMidiData *) new CoreMidiData;
    data->client = client;
    data->endpoint = 0;
    m_api_data = (void *) data;
    m_input_data.api_data((void *) data);
    return true;
}

bool
midi_core_in::open_port (int portnumber, const std::string & portname)
{
    if (m_is_connected)
    {
        m_error_string = "midi_core_in::open_port: connection already exists";
        error(rterror::kind::warning, m_error_string);
        return true;
    }

    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    unsigned nsrc = MIDIGetNumberOfSources();
    if (nsrc < 1)
    {
        m_error_string = "midi_core_in::open_port: no MIDI input sources found";
        error(rterror::kind::no_devices_found, m_error_string);
        return false;
    }
    if (portnumber >= nsrc)
    {
        std::ostringstream ost;
        ost << "midi_core_in::open_port: portnumber argument ("
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::kind::invalid_parameter, m_error_string);
        return false;
    }

    MIDIPortRef port;
    CoreMidiData * data = static_cast<CoreMidiData *>(m_api_data);
    CFStringRef portnameref = CFStringCreateWithCString
    (
        NULL, portname.c_str(), kCFStringEncodingASCII
    );
    OSStatus result = MIDIInputPortCreate
    (
        data->client, portnameref, midi_input_callback,
        (void *) &m_input_data, &port
    );
    CFRelease (portnameref);

    if (result != noErr)
    {
        m_error_string = "midi_core_in::open_port: error creating input port.";
        error(rterror::kind::driver_error, m_error_string);
        return false;
    }

    // Get the desired input source identifier.

    MIDIEndpointRef endpoint = MIDIGetSource (portnumber);
    if (endpoint == 0)
    {
        MIDIPortDispose (port);
        m_error_string = "midi_core_in::open_port: error getting input reference.";
        error(rterror::kind::driver_error, m_error_string);
        return false;
    }

    // Make the connection.

    result = MIDIPortConnectSource(port, endpoint, NULL);
    if (result != noErr)
    {
        MIDIPortDispose (port);
        m_error_string = "midi_core_in::open_port: error connecting input port.";
        error(rterror::kind::driver_error, m_error_string);
        return false;
    }
    data->port = port;  // save api-specific port information
    m_is_connected = true;
    return true;
}

/**
 *  Create a virtual MIDI input destination.
 *  Save the api-specific connection information.
 */

bool
midi_core_in::open_virtual_port (const std::string & portname)
{
    CoreMidiData * data = static_cast<CoreMidiData *>(m_api_data);
    MIDIEndpointRef endpoint;
    CFStringRef portnameref = CFStringCreateWithCString
    (
        NULL, portname.c_str(), kCFStringEncodingASCII
    );
    OSStatus result = MIDIDestinationCreate
    (
        data->client, portnameref, midi_input_callback,
        (void *) &m_input_data, &endpoint
    );
    CFRelease(portnameref);
    if (result != noErr)
    {
        m_error_string =
            "midi_core_in::open_virtual_port: error creating virtual destination.";

        error(rterror::kind::driver_error, m_error_string);
        return false;
    }
    data->endpoint = endpoint;
    return true;
}

bool
midi_core_in::close_port ()
{
    CoreMidiData *data = static_cast<CoreMidiData *>(m_api_data);
    if (data->endpoint)
    {
        MIDIEndpointDispose(data->endpoint);
        data->endpoint = 0;
    }
    if (data->port)
    {
        MIDIPortDispose(data->port);
        data->port = 0;
    }
    m_is_connected = false;
    return true;
}

bool
midi_core_in::set_client_name (const std::string &)
{
    m_error_string =
        "midi_core_in::set_client_name: unimplemented for Core API";

    error(rterror::kind::warning, m_error_string);
    return true;
}

bool
midi_core_in::set_port_name  (const std::string& )
{
    m_error_string =
        "midi_core_in::set_port_name: unimplemented for Core API";

    error(rterror::kind::warning, m_error_string);
    return true;
}

int
midi_core_in::get_port_count ()
{
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    return int(MIDIGetNumberOfSources());
}

/**
 *  This function was submitted by Douglas Casey Tucker and apparently
 *  derived largely from PortMidi.
 *  Begin with the endpoint's name.
 *  Some MIDI devices have a leading space in endpoint name; trim it.
 */

CFStringRef
EndpointName (MIDIEndpointRef endpoint, bool isExternal)
{
    CFMutableStringRef result = CFStringCreateMutable (NULL, 0);
    CFStringRef str = NULL;
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &str);
    if (str != NULL)
    {
        CFStringAppend (result, str);
        CFRelease (str);
    }

    CFStringRef space = CFStringCreateWithCString
    (
        NULL, " ", kCFStringEncodingUTF8
    );
    CFStringTrim(result, space);
    CFRelease(space);

    MIDIEntityRef entity = 0;
    MIDIEndpointGetEntity (endpoint, &entity);
    if (entity == 0)                                // probably virtual
        return result;

    if (CFStringGetLength( result ) == 0 )
    {
        // endpoint name has zero length -- try the entity

        str = NULL;
        MIDIObjectGetStringProperty(entity, kMIDIPropertyName, &str);
        if (str != NULL)
        {
            CFStringAppend (result, str);
            CFRelease (str);
        }
    }

    MIDIDeviceRef device = 0;           // now consider the device's name
    MIDIEntityGetDevice(entity, &device);
    if (device == 0)
        return result;

    str = NULL;
    MIDIObjectGetStringProperty(device, kMIDIPropertyName, &str);
    if (CFStringGetLength(result) == 0)
    {
        CFRelease(result);
        return str;
    }
    if (str != NULL )
    {
        /*
         * If an external device has only one entity, throw away
         * the endpoint name and just use the device name
         */

        if (isExternal && MIDIDeviceGetNumberOfEntities(device) < 2)
        {
            CFRelease (result);
            return str;
        }
        else
        {
            if (CFStringGetLength(str) == 0)
            {
                CFRelease (str);
                return result;
            }

            /*
             * Does the entity name already start with the device name?
             * (Some drivers do this though they shouldn't.)
             * If so, do not prepend.
             * Otherwise,  prepend the device name to the entity name.
             */

            bool prepend = CFStringCompareWithOptions
            (
                result /* endpoint name */, str /* device name */,
                CFRangeMake(0, CFStringGetLength(str)), 0
            ) != kCFCompareEqualTo
            if (prepend)
            {
                if (CFStringGetLength(result) > 0)
                  CFStringInsert(result, 0, CFSTR(" "));

                CFStringInsert(result, 0, str);
            }
            CFRelease (str);
        }
    }
    return result;
}

/**
 *  This function was submitted by Douglas Casey Tucker and apparently derived
 *  largely from PortMidi.  Does the endpoint have connections?  If it has
 *  connections, follow them.  Concatenate the names of all connected devices.  At
 *  the very end, either the endpoint had no connections, or we failed to obtain
 *  names
 */

static CFStringRef
ConnectedEndpointName (MIDIEndpointRef endpoint )
{
    CFMutableStringRef result = CFStringCreateMutable(NULL, 0);
    CFStringRef str;
    int i;
    CFDataRef connections = NULL;
    int nConnected = 0;
    bool anystrings = false;
    OSStatus err = MIDIObjectGetDataProperty
    (
        endpoint, kMIDIPropertyConnectionUniqueID, &connections
    );
    if (connections != NULL)
    {
        nConnected = CFDataGetLength(connections ) / sizeof(MIDIUniqueID);
        if (nConnected)
        {
            const SInt32 * pid = (const SInt32 *)(CFDataGetBytePtr(connections));
            for (i = 0; i < nConnected; ++i, ++pid)
            {
                MIDIUniqueID id = EndianS32_BtoN(*pid);
                MIDIObjectRef connObject;
                MIDIObjectType connObjectType;
                err = MIDIObjectFindByUniqueID(id, &connObject, &connObjectType);
                if (err == noErr)
                {
                    bool extcheck =
                        connObjectType == kMIDIObjectType_ExternalSource  ||
                        connObjectType == kMIDIObjectType_ExternalDestination;

                    if (extcheck)
                    {
                        // Connected to an external device's endpoint
                        // (10.3 and later).

                        str = EndpointName((MIDIEndpointRef)(connObject), true);
                    }
                    else
                    {
                        // Connected to an external device (10.2)
                        // (or something else, catch-

                        str = NULL;
                        MIDIObjectGetStringProperty
                        (
                            connObject, kMIDIPropertyName, &str
                        );
                    }
                    if (str != NULL)
                    {
                        if (anystrings)
                            CFStringAppend(result, CFSTR(", "));
                        else
                            anystrings = true;

                        CFStringAppend(result, str);
                        CFRelease(str);
                    }
                }
            }
        }
        CFRelease (connections);
    }
    if (anystrings)
        return result;

    CFRelease (result);
    return EndpointName(endpoint, false);
}

std::string
midi_core_in::get_port_name (int portnumber)
{
    std::string stringname;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    if (portnumber >= MIDIGetNumberOfSources())
    {
        std::ostringstream ost;
        ost << "midi_core_in::get_port_name: portnumber argument ("
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::kind::warning, m_error_string);
        return stringname;
    }

    MIDIEndpointRef portref = MIDIGetSource(portnumber);
    CFStringRef nameref = ConnectedEndpointName(portref);
    char name[128];
    CFStringGetCString(nameref, name, sizeof name, kCFStringEncodingUTF8);
    CFRelease(nameref);
    stringname = name;
    return stringname;
}

/*------------------------------------------------------------------------
 * midi_core_in
 *------------------------------------------------------------------------*/

midi_core_out::midi_core_out (const std::string & clientname) :
    midi_out_api()
{
    midi_core_out::initialize(clientname);
}

/**
 *  Close a connection if it exists.
 */

midi_core_out::~midi_core_out ()
{
    midi_core_out::close_port();

    CoreMidiData * data = static_cast<CoreMidiData *>(m_api_data);
    if (data->endpoint)
        MIDIEndpointDispose(data->endpoint);

    delete data;
}

MIDIClientRef
midi_core_out::get_core_midi_client_singleton
(
    const std::string & clientname
) noexcept
{
    if (CoreMidiClientSingleton == 0)
    {
        MIDIClientRef client;         // Set up our client.
        CFStringRef name = CFStringCreateWithCString
        (
            NULL, clientname.c_str(), kCFStringEncodingASCII
        );
        OSStatus result = MIDIClientCreate(name, NULL, NULL, &client);
        if (result != noErr)
        {
            std::ostringstream ost;
            ost << "midi_core_in::initialize: error creating client object ("
                << result << ")."
                ;
            m_error_string = ost.str();
            error(rterror::kind::driver_error, m_error_string);
            return 0;
        }
        CFRelease(name);
        CoreMidiClientSingleton = client;
    }
    return CoreMidiClientSingleton;
}

bool
midi_core_out::initialize (const std::string & clientname)
{
    MIDIClientRef client = get_core_midi_client_singleton(clientname);
    CoreMidiData * data = (CoreMidiData *) new CoreMidiData;
    data->client = client;
    data->endpoint = 0;
    m_api_data = (void *) data;
    return true;
}

int
midi_core_out::get_port_count ()
{
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    return MIDIGetNumberOfDestinations();
}

std::string
midi_core_out::get_port_name (int portnumber)
{
    std::string stringname;
    CFRunLoopRunInMode (kCFRunLoopDefaultMode, 0, false);
    if (portnumber >= MIDIGetNumberOfDestinations())
    {
        std::ostringstream ost;
        ost << "midi_core_out::get_port_name: portnumber argument ("
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::kind::warning, m_error_string);
        return stringname;
    }

    MIDIEndpointRef portref = MIDIGetDestination (portnumber);
    CFStringRef nameref = ConnectedEndpointName(portref);
    char name[128];
    CFStringGetCString (nameref, name, sizeof(name), kCFStringEncodingUTF8);
    CFRelease (nameref);
    stringname = name;
    return stringname;
}

bool
midi_core_out::open_port (int portnumber, const std::string & portname)
{
    if (m_is_connected)
    {
        m_error_string = "midi_core_out::open_port: valid connection exists";
        error(rterror::kind::warning, m_error_string);
        return true;
    }

    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    unsigned ndest = MIDIGetNumberOfDestinations();
    if (ndest < 1)
    {
        m_error_string = "midi_core_out::open_port: no outputs found";
        error(rterror::kind::no_devices_found, m_error_string);
        return false;
    }
    if (portnumber >= ndest)
    {
        std::ostringstream ost;
        ost << "midi_core_out::open_port: portnumber argument ("
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::kind::invalid_parameter, m_error_string);
        return false;
    }

    MIDIPortRef port;
    CoreMidiData * data = static_cast<CoreMidiData *>(m_api_data);
    CFStringRef portnameref = CFStringCreateWithCString
    (
        NULL, portname.c_str(), kCFStringEncodingASCII
    );
    OSStatus result = MIDIOutputPortCreate(data->client, portnameref, &port);
    CFRelease(portnameref);
    if (result != noErr)
    {
        m_error_string = "midi_core_out::open_port: error creating output port.";
        error(rterror::kind::driver_error, m_error_string);
        return false;
    }

    // Get the desired output port identifier.

    MIDIEndpointRef destination = MIDIGetDestination(portnumber);
    if (destination == 0)
    {
        MIDIPortDispose(port);
        m_error_string = "midi_core_out::open_port: error getting output reference.";
        error(rterror::kind::driver_error, m_error_string);
        return false;
    }

    // Save our api-specific connection information.

    data->port = port;
    data->destinationId = destination;
    m_is_connected = true;
    return true;
}

bool
midi_core_out::close_port ()
{
    CoreMidiData * data = static_cast<CoreMidiData *>(m_api_data);
    if (data->endpoint)
    {
        MIDIEndpointDispose(data->endpoint);
        data->endpoint = 0;
    }
    if (data->port)
    {
        MIDIPortDispose(data->port);
        data->port = 0;
    }
    m_is_connected = false;
    return true;
}

bool
midi_core_out::set_client_name  (const std::string &)
{
    m_error_string =
        "midi_core_out::set_client_name: unimplemented for Core API";

    error(rterror::kind::warning, m_error_string);
    return true;
}

bool
midi_core_out::set_port_name (const std::string &)
{
    m_error_string = "midi_core_out::set_port_name: unimplemented for Core API";

    error (rterror::kind::warning, m_error_string);
    return true;
}

bool
midi_core_out::open_virtual_port (const std::string & portname)
{
    CoreMidiData *data = static_cast<CoreMidiData *> (m_api_data);
    if (data->endpoint)
    {
        m_error_string =
            "midi_core_out::open_virtual_port: virtual output port already exists";
        error (rterror::kind::warning, m_error_string);
        return;
    }

    // Create a virtual MIDI output source.

    MIDIEndpointRef endpoint;
    CFStringRef portnameref = CFStringCreateWithCString
    (
        NULL, portname.c_str(), kCFStringEncodingASCII
    );
    OSStatus result = MIDISourceCreate(data->client, portnameref, &endpoint);
    CFRelease(portnameref);
    if (result != noErr)
    {
        m_error_string = "midi_core_out::initialize: error creating virtual source.";
        error(rterror::kind::driver_error, m_error_string);
        return false;
    }
    data->endpoint = endpoint; // save api-specific connection information
    return true;
}

/**
 *  We use the MIDISendSysex() function to asynchronously send sysex
 *  messages.  Otherwise, we use a single CoreMidi MIDIPacket.
 *
 *  A MIDIPacketList can only contain a maximum of 64K of data, so if our
 *  message is longer, break it up into chunks of 64K or less and send out as a
 *  MIDIPacketList with only one MIDIPacket. Here, we reuse the memory allocated
 *  above on the stack for all.
 */

bool
midi_core_out::send_message (const midi::byte * message, size_t sz)
{
    using namespace midi;               /* midi::byte(), midi::status, etc. */

    unsigned nbytes = static_cast<unsigned int>(sz);
    if (nbytes == 0)
    {
        m_error_string = "midi_core_out::send_message: no data in message";
        error (rterror::kind::warning, m_error_string);
        return true;
    }
    if (! is_sysex(message[0]) && nbytes > 3)
    {
        m_error_string = "midi_core_out::send_message: not sysex but > 3 bytes";
        error (rterror::kind::warning, m_error_string);
        return true;
    }

    MIDITimeStamp timeStamp = AUDIO_GET_HOST_TIME();
    CoreMidiData *data = static_cast<CoreMidiData *> (m_api_data);
    OSStatus result;
    ByteCount bufsize = nbytes > 65535 ? 65535 : nbytes;
    Byte buffer[bufsize + 16];                  // pad for other struct members
    ByteCount listSize = sizeof buffer;
    MIDIPacketList * packetlist = (MIDIPacketList *) buffer;
    ByteCount bytesleft = nbytes;
    while (bytesleft > 0)
    {
        MIDIPacket * packet = MIDIPacketListInit(packetlist); // see banner
        ByteCount bytesForPacket = bytesleft > 65535 ? 65535 : bytesleft;
        const Byte* dataStartPtr = (const Byte *) &message[nbytes - bytesleft];
        packet = MIDIPacketListAdd
        (
            packetlist, listSize, packet, timeStamp,
            bytesForPacket, dataStartPtr
        );
        bytesleft -= bytesForPacket;
        if (is_nullptr(packet))
        {
            m_error_string =
                "midi_core_out::send_message: could not allocate packet list";

            error(rterror::kind::driver_error, m_error_string);
            return false;
        }

        /*
         * Send to any destinations that may have connected.
         */

        if (data->endpoint)
        {
            result = MIDIReceived(data->endpoint, packetlist);
            if (result != noErr)
            {
                m_error_string =
                    "midi_core_out::send_message: error sending to virtual port.";
                error(rterror::kind::warning, m_error_string);
            }
        }

        // And send to an explicit destination port if we're connected.

        if (m_is_connected)
        {
            result = MIDISend(data->port, data->destinationId, packetlist);
            if (result != noErr)
            {
                m_error_string =
                    "midi_core_out::send_message: error sending message to port.";
                error (rterror::kind::warning, m_error_string);
            }
        }
    }
    return true;
}

}           // namespace rtl

#endif  // defined RTL66_PLATFORM_MACOSX

/*
 * midi_macosx_core.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

