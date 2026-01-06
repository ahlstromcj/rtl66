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
 * \file          test_helpers.cpp
 *
 *      Simple helper functions re-used in the test applications.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-06-30
 * \updates       2026-01-02
 * \license       See above.
 *
 *  We have a lot of functions for selecting ports !
 */

#include <iostream>                     /* std::cout, std::cin              */
#include <memory>                       /* std::unique_ptr<>                */

#include "midi/clientinfo.hpp"          /* midi::global_client_info()       */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in, classes          */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* free functions in global space   */
#include "util/msgfunctions.hpp"        /* util::verbose()                  */

#if defined LIBS66_USE_POTEXT
#include "po/potext.hpp"                /* the po::gettext() interfaces     */
#else
#define _(str)      (str)
#define N_(str)     str
#endif

/*
 *  Platform-dependent sleep routines. Eventually replace the Seq66
 *  millisleep() function.
 */

#if defined PLATFORM_WINDOWS

#include <windows.h>

void
rt_test_sleep (int ms)
{
    Sleep((DWORD) ms);
}

#else                   /* Unix variants */

#include <unistd.h>

void
rt_test_sleep (int ms)
{
    usleep((unsigned long)(ms * 1000.0));
}

#endif

/**
 * Functions to choose ports:
 *
 *  -   rt_choose_port(midi::port::io, int &) [static]
 *
 *      -   Creates an rtmidi_out or rtmidi_in object.
 *      -   Gets the port count; it is copied to the integer output
 *          parameter.
 *      -   For each port, gets the number and name, and shows it
 *          for the user to pick, along with the "All ports" option..
 *          Sets the static value s_open_all_ports to true with the
 *          "all" option.
 *      -   Note: No port-opening is done.
 *
 *  -   rt_choose_port_number(midi::port::io).
 *
 *      -   Calls the rt_choose_port() function described above.
 *      -   Ignores the port count.
 *
 *  -   choose_midi_port(RTMIDI_TYPE &, isoutput)
 *
 *      -   A template called with type rtmidi_out or rtmidi_in.
 *      -   If virtual, a virtual port is opened, otherwise...
 *      -   rt_choose_port_number() is called as above.
 *      -   The test output or input port number is set to the chosen
 *          port.
 *      -   The test port is opened.
 *
 *  -   rt_choose_input_port() calls choose_midi_port<rtl::rtmidi_in>(false);
 *  -   rt_choose_output_port() calls choose_midi_port<rtl::rtmidi_out>(false);
 *  -   rt_choose_input_ports() calls rt_choose_port(io::input, portcount);
 *  -   rt_choose_output_ports() calls rt_choose_port(io::output, portcount);
 */

/**
 *  Note:
 *
 *      Once the port is chosen and is valid, the caller should then
 *      call rt_test_port(result) or whatever is needed to make the
 *      port number official.
 */

static bool s_allow_open_all_ports { true };
static bool s_open_all_ports { false };

bool
rt_allow_open_all_ports ()
{
    return s_allow_open_all_ports;
}

void
set_rt_allow_open_all_ports (bool flag)
{
    s_allow_open_all_ports = flag;
}

bool
rt_open_all_ports ()
{
    return s_open_all_ports;
}

void
set_rt_open_all_ports ()
{
    s_open_all_ports = true;
}

/**
 *  This function gets the portname, and then the alias(es) of the
 *  port (a feature only of JACK). Here is an example of JACK port names
 *  and their aliases:
 *
 *    input port #0: system:midi_capture_1
 *        [alsa_pcm:Midi-Through/midi_playback_1 or Midi-Through:midi/playback_1]
 *
 *    input port #1: system:midi_capture_2
 *        [alsa_pcm:nanoKEY2/midi_playback_1 or nanoKEY2:midi/playback_1]
 *
 *    input port #2: system:midi_capture_3
 *        [alsa_pcm:Q25/midi_playback_1 or Q25:midi/playback_1]*
 */

int
rt_choose_port (midi::port::io iotype, int & portcount, bool showalloption)
{
    int result { -1 };
    bool isoutput { iotype == midi::port::io::output };
    std::string direction { isoutput ? _("Output") : _("Input") };
    std::unique_ptr<rtl::rtmidi> rt;
    try
    {
        if (isoutput)
        {
            rt.reset(new rtl::rtmidi_out(rtl::rtmidi::desired_api()));
            portcount = rt->get_port_count();
        }
        else
        {
            rt.reset(new rtl::rtmidi_in(rtl::rtmidi::desired_api()));
            portcount = rt->get_port_count();
        }
        if (portcount == 0)
        {
            std::cout
                << _("no") << " " << direction << " " << _("ports available")
                << std::endl
                ;
        }
        else
        {
            if (portcount == 1)
            {
                result = 0;
                std::cout
                    << _("Only one port; will use") << " "
                    << direction << " " << _("port") << " "
                    << rt->get_port_name()
                    << std::endl
                    ;
            }
            else
            {
                /*
                 *  For the best clarity in the prompt, if there are aliases
                 *  then show them first (in JACK, the second one is
                 *  simplest).
                 */

                int p;
                for (p = 0; p < portcount; ++p)
                {
                    std::string portname { rt->get_port_name(p) };
                    std::string alias0 { rt->get_port_alias(portname, 0) };
                    std::string alias1 { rt->get_port_alias(portname, 1) };
                    bool noalias { alias0.empty() };
                    if (noalias)
                    {
                        std::cout
                            << "  " << direction << " #"
                            << p << ": " << portname << std::endl
                            ;
                    }
                    else
                    {
                        if (! alias1.empty())
                        {
                            std::cout
                                << "  " << direction << " #"
                                << p << ": " << alias1
                                << std::endl
                                << "    [" << alias0
                                << " & " << portname << "]"
                                << std::endl
                                ;
                        }
                        else if (! alias0.empty())
                        {
                            std::cout
                                << "  " << direction << " #"
                                << p << ": " << alias0
                                << std::endl
                                << "    [" << portname << "]"
                                << std::endl
                                ;
                        }
                    }
                }

                if (showalloption)
                {
                    std::cout
                        << "  " << direction << " #"
                        << portcount << ": All ports"
                        << std::endl
                        ;
                }
                do
                {
                    std::cout << _("Choose a port number") << ": ";
                    try
                    {
                        std::cin >> p;
                    }
                    catch (...)
                    {
                        /*
                         * Entering a letter yields p == 0, but causes
                         * a seqfault. Entering 0? No problem. So we catch.
                         * DOESN'T HELP.
                         */
                    }
                    if (p == portcount && showalloption)
                    {
                        p = midi::c_ports_all;
                        s_open_all_ports = true;
                        break;
                    }
                } while (p < 0 || p >= portcount);
                result = p;
                set_rt_test_port(p);            /* includes RTL66_PORTS_ALL */

                /*
                 * Set to clear and ignore the Enter after the port
                 * number, so that follow-on input requests will work.
                 *
                 * TODO: Add to the xpc66 kbhit module.
                 */

                std::cin.clear();
                std::cin.ignore(1, '\n');
            }
            std::cout << "\n";
        }
    }
    catch (rtl::rterror & error)
    {
        error.print_message();
    }
    catch (...)
    {
        std::string msg { _("Unknown exception... fix the catch") };
        errprint(CSTR(msg));
    }
    return result;
}

int
rt_choose_port_number (midi::port::io iotype)
{
    int portcount { 0 };
    int result
    {
        rt_choose_port(iotype, portcount, rt_allow_open_all_ports())
    };
    if (rt_test_port_valid(result))
    {
        set_rt_test_port(result);
    }
    else
    {
        set_rt_test_port(0);
        result = 0;
        infoprint("Using port 0; use --port p option if desired.");
    }
    return result;
}

/**
 *  This function should be embedded in a try/catch block in case of an
 *  exception.  It offers the user a choice of MIDI ports to open.  It returns
 *  false if there are no ports available.
 *
 *  Also note that using a virtual port is now a test command-line option,
 *  "--virtual".
 */

template<typename RTMIDI_TYPE>
bool
choose_midi_port (RTMIDI_TYPE & rt, midi::port::io iotype)
{
    bool result { true };
    if (rt_virtual_test_port())
    {
        rt.open_virtual_port();
    }
    else
    {
        int portno { rt_choose_port_number(iotype) };
        result = portno >= 0;
        if (result)
        {
            set_rt_test_port(portno);       /* just in case; app decides    */
            if (iotype == midi::port::io::output)
                set_rt_test_port_out(portno);
            else
                set_rt_test_port_in(portno);

            result = rt.open_port(portno);
        }
    }
    return result;
}

/**
 *  This function also sets s_test_port_in.
 */

bool
rt_choose_input_port (rtl::rtmidi_in & rtin)
{
    return choose_midi_port<rtl::rtmidi_in>(rtin, midi::port::io::input);
}

/**
 *  This function also sets s_test_port_out.
 */

bool
rt_choose_output_port (rtl::rtmidi_out & rtout)
{
    return choose_midi_port<rtl::rtmidi_out>(rtout, midi::port::io::output);
}

/**
 *  These function choose the port number and also return the port-count,
 *  for convenience.
 */

int
rt_choose_input_ports (int & portcount)
{
    return rt_choose_port
    (
        midi::port::io::input, portcount, rt_allow_open_all_ports()
    );
}

int
rt_choose_output_ports (int & portcount)
{
    return rt_choose_port
    (
        midi::port::io::output, portcount, rt_allow_open_all_ports()
    );
}

/**
 *  These function encapsulate a common sequence of selecting one or
 *  more ports in an application. See the MIDI input test applications.
 *
 *  Does not handle virtual ports yet.
 */

bool
rt_select_input_ports (int & portcount)
{
    bool result { false };
    int port = rt_test_port();
    if (port < 0)
    {
        /*
         * We have added new test functions to also get the port
         * count.
         *
         *  port = rt_choose_port_number(false); // for in, not out
         */

        port = rt_choose_input_ports(portcount);
        result = port >= 0;
    }
    else
    {
        if (rt_open_all_ports())                /* the "--port all" option. */
        {
            port = rt_choose_input_ports(portcount);
            result = port >= 0;                 /* includes RTL66_PORTS_ALL */
        }
        else
            result = rt_test_port_valid(port);
    }
    return result;
}

/**
 *  This stuff provides a very simple set of command-line options, mostly
 *  for the test applications.
 *
 *  TO DO: Mark this for translation.
 */

static const std::string s_help_text_fmt
{
"Usage: %s [ options ]\n\n"
"Runs basic tests for some APIs of Rtl66 library, v. %s.\n"
"It is based on a greatly refactored adaptation of RtMidi v. %s library.\n\n"
"Without options it uses the first MIDI API that is compiled in, falling\n"
"back to the next API if the first is not detected.  For example, in Linux\n"
"JACK will be tried first.  If not detected, then ALSA will be tried.  One\n"
"issue is that, on newer systems, jackdbus can fool JACK detection into\n"
"finding JACK, while using JACK will fail. Use the --quiet-jack option to\n"
"hide the voluminous JACK console log output.\n"
"\n"
#if defined PLATFORM_LINUX
"Linux options:\n"
"\n"
#if defined RTL66_BUILD_JACK
"  --jack           Instead of the JACK/ALSA fallback, try JACK, and fail if it\n"
"                   cannot be initialized.\n"
"  --quiet-jack     Hide JACK console output. Send it to the bit bucket.\n"
"  --start-jack     Start the JACK server if not running. NOT READY.\n"
#endif
#if defined RTL66_BUILD_ALSA
"  --alsa           Instead of the JACK/ALSA fallback, try ALSA, and fail if it\n"
"                   cannot be initialized.\n"
#endif
#endif              // defined PLATFORM_LINUX
"  --test name      Run only the selected test or data file (app-dependent).\n"
"  --virtual        Use virtual ports (not available to some MIDI engines).\n"
"  --auto-connect   For non-virtual ports, get the existing system ports and\n"
"                   try to connect to them.\n"
"  --ppqn value     Change the default PPQN from 192 to the given value.\n"
"  --bpm value      Change the default BPM from 120.0 to the given value.\n"
"  --client cn      Provide a client name (e.g. to be shown in JACK graph.\n"
"  --port p         Set the test port, for applications that use that option.\n"
"                   'all' means all ports, if the application supports it.\n"
"  --port-in p      Set the input test port, for apps that need I/O ports.\n"
"  --port-out p     Set the output test port, for apps that need I/O ports.\n"
"  --port-name n    Provides a test name for the port. Otherwise empty.\n"
"  --length p       Set the amount of test data, if applicable.\n"
"  --callback       Use an input callback instead of polling.\n"
"  --verbose        Set verbosity to show additional information.\n"
"  --quiet          Set to show less information.\n"
"  -h, --help       Show this help text.\n"
"\n"
};

/**
 *  Specifies to create a single virtual port for testing.
 */

static bool s_virtual_test_port { false };

static void
set_virtual_test_port (bool flag)
{
    s_virtual_test_port = flag;
}

bool
rt_virtual_test_port ()
{
    return s_virtual_test_port;
}

/**
 *  Specifies the test port (where applicable).
 *
 *      -   s_test_port. This is the main port number for the application,
 *          whether it is an input or output port.
 *      -   s_test_port_in. Provides the input port for a two-way test.
 *      -   s_test_port_out. Provides the output port for a two-way test.
 */

static int s_test_port { -1 };
static int s_test_port_in { -1 };
static int s_test_port_out { -1 };

static int
string_to_int (const std::string & s)
{
    int result { -1 };
    try
    {
        result = std::stoi(s);
    }
    catch (std::invalid_argument const &)
    {
        // no code
    }
    return result;
}

void
set_rt_test_port (int portno)
{
    s_test_port = portno;
}

int
rt_test_port ()
{
    return s_test_port;
}

void
set_rt_test_port_in (int portno)
{
    s_test_port_in = portno;
    midi::global_client_info().input_portnumber(portno);
}

int
rt_test_port_in ()
{
    return s_test_port_in;
}

void
set_rt_test_port_out (int portno)
{
    s_test_port_out = portno;
    midi::global_client_info().output_portnumber(portno);
}

int
rt_test_port_out ()
{
    return s_test_port_out;
}

bool
rt_test_port_valid (int portnumber)
{
    return
    (
        (portnumber >= 0 && portnumber <= RTL66_PORT_NUMBER_LIMIT) ||
        portnumber == RTL66_PORTS_ALL
    );
}

/**
 *  The default port number is RTL66_PORTS_ALL.
 */

bool
rt_open_all_ports (int portnumber)
{
    return portnumber == RTL66_PORTS_ALL;
}

/**
 *  Specifies the test port name (where applicable).
 */

static std::string s_test_port_name;                    /* empty to start   */

void
set_rt_test_port_name (const std::string & portname)
{
    s_test_port_name = portname;
}

const std::string &
rt_test_port_name ()
{
    return s_test_port_name;
}

/**
 *  Allows for selecting a particular test in a test application.
 */

static std::string s_test_name;                    /* empty to start   */

void
set_rt_test_name (const std::string & portname)
{
    s_test_name = portname;
}

const std::string &
rt_test_name ()
{
    return s_test_name;
}

/**
 *  Specifies the test data-length (where applicable).
 */

static int s_test_data_length { -1 };

void
set_rt_test_data_length (int len)
{
    s_test_data_length = len;
}

int
rt_test_data_length ()
{
    return s_test_data_length;
}

static bool s_test_show_help = false;

void
set_show_help (bool flag)
{
    s_test_show_help = flag;
}

bool
rt_show_help ()
{
    return s_test_show_help;
}

static bool s_test_use_callback = false;

void
set_use_callback (bool flag)
{
    s_test_use_callback = flag;
}

bool
rt_use_callback ()
{
    return s_test_use_callback;
}

/**
 *  This static function processes the command-line.  It returns true if no
 *  help or version option was specified, and means the program can run
 *  normally.
 */

bool
rt_simple_cli (const std::string & appname, int argc, char * argv [])
{
    bool can_run { true };
    rtl::rtmidi::api rapi { rtl::rtmidi::api::unspecified };
    set_rt_test_port(-1);
    set_rt_test_port_in(-1);
    set_rt_test_port_out(-1);
    set_rt_test_port_name("");
    std::cout << "Application: '" << appname << "'" << std::endl;
    midi::global_client_info().app_name(appname);
    for (int i = 1; i < argc; ++i)
    {
        std::string arg { argv[i] };
        if (arg == "--help" || arg == "-h")
        {
            set_show_help(true);
            can_run = false;
        }
        else if (arg == "--verbose")
        {
            util::set_verbose(true);
        }
        else if (arg == "--quiet")
        {
            util::set_quiet(true);
        }
#if defined PLATFORM_LINUX
#if defined RTL66_BUILD_JACK
        else if (arg == "--jack")
        {
            rapi = rtl::rtmidi::api::jack;
        }
        else if (arg == "--quiet-jack" || arg == "--quiet")
        {
            rtl::rtmidi::silence_messages(true);
        }
        else if (arg == "--start-jack")
        {
            rtl::rtmidi::start_jack(true);
        }
#endif
#if defined RTL66_BUILD_ALSA
        else if (arg == "--alsa")
        {
            rapi = rtl::rtmidi::api::alsa;
        }
#endif
#endif          // defined PLATFORM_LINUX
        else if (arg == "--test")
        {
            if (i + 1 < argc)
            {
                std::string value = std::string(argv[i + 1]);
                set_rt_test_name(value);
            }
        }
        else if (arg == "--virtual")
        {
            set_virtual_test_port(true);
            midi::global_client_info().virtual_ports(true);
            midi::global_client_info().auto_connect(false);
        }
        else if (arg == "--auto-connect")
        {
            midi::global_client_info().virtual_ports(false);
            midi::global_client_info().auto_connect(true);
        }
        else if (arg == "--ppqn")
        {
            if (i + 1 < argc)
            {
                try
                {
                    std::string value { std::string(argv[i + 1]) };
                    int v { string_to_int(value) };
                    midi::ppqn ppq { static_cast<midi::ppqn>(v) };
                    midi::global_client_info().global_ppqn(ppq);
                }
                catch (const std::invalid_argument &)
                {
                    std::cerr << "--ppqn argument invalid" << std::endl;
                }
                catch (const std::out_of_range &)
                {
                    std::cerr << "--ppqn argument out-of-range" << std::endl;
                }
            }
        }
        else if (arg == "--bpm")
        {
            if (i + 1 < argc)
            {
                try
                {
                    std::string value { std::string(argv[i + 1]) };
                    double v { std::stod(value) };
                    midi::bpm b { midi::bpm(static_cast<midi::ppqn>(v)) };
                    midi::global_client_info().global_bpm(b);
                }
                catch (const std::invalid_argument &)
                {
                    std::cerr << "--bpm argument invalid" << std::endl;
                }
                catch (const std::out_of_range &)
                {
                    std::cerr << "--bpm argument out-of-range" << std::endl;
                }
            }
        }
        else if (arg == "--client")
        {
            if (i + 1 < argc)
            {
                std::string value { std::string(argv[i + 1]) };
                midi::global_client_info().client_name(value);
            }
        }
        else if (arg == "--length")
        {
            if (i + 1 < argc)
            {
                std::string value { std::string(argv[i + 1]) };
                int v { string_to_int(value) };
                set_rt_test_data_length(v);
            }
        }
        else if (arg == "--port")
        {
            if (i + 1 < argc)
            {
                std::string value { std::string(argv[i + 1]) };
                if (value == "all")
                {
                    set_rt_test_port(RTL66_PORTS_ALL);
                }
                else
                {
                    int v { string_to_int(value) };
                    set_rt_test_port(v);
                }
            }
        }
        else if (arg == "--port-in")
        {
            if (i + 1 < argc)
            {
                std::string value { std::string(argv[i + 1]) };
                if (value == "all")
                {
                    set_rt_test_port(RTL66_PORTS_ALL);
                }
                else
                {
                    int v { string_to_int(value) };
                    set_rt_test_port_in(v);
                }
            }
        }
        else if (arg == "--port-out")
        {
            if (i + 1 < argc)
            {
                std::string value { std::string(argv[i + 1]) };
                if (value == "all")
                {
                    set_rt_test_port(RTL66_PORTS_ALL);
                }
                else
                {
                    int v { string_to_int(value) };
                    set_rt_test_port_out(v);
                }
            }
        }
        else if (arg == "--port-name")
        {
            if (i + 1 < argc)
            {
                std::string value { std::string(argv[i + 1]) };
                set_rt_test_port_name(value);
            }
        }
        else if (arg == "--callback")
        {
            set_use_callback(true);
        }
    }
    if (can_run)
    {
        rtl::rtmidi::desired_api(rapi);
    }
    else
    {
        printf
        (
            s_help_text_fmt.c_str(),
            V(appname),
            V(rtl::get_rtl_midi_version()),
            V(rtl::get_rtmidi_patch_version())
        );
    }
    return can_run;
}

bool
rt_get_extra_option
(
    int argc, char * argv [],
    const std::string & longopt, char shortopt,
    std::string * parameter
)
{
    bool result { false };
    std::string targetstr { "--" };
    std::string charstr { "-" };
    targetstr += longopt;
    charstr += shortopt;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg { argv[i] };
        if (arg == targetstr || arg == charstr)
        {
            result = true;
            if (not_nullptr(parameter))
            {
                ++i;
                if (i < argc)
                    *parameter = argv[i];
            }
            break;
        }
    }
    return result;
}

/*
 * test_helpers.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

