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
 * \updates       2024-06-05
 * \license       See above.
 *
 */

#include <iostream>                     /* std::cout, std::cin              */
#include <memory>                       /* std::unique_ptr<>                */

#include "midi/clientinfo.hpp"          /* midi::global_client_info()       */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in, classes          */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* free functions in global space   */

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

int
rt_choose_port_number (bool isoutput)
{
    int result = (-1);
    int portcount = 0;
    std::string direction = isoutput ? _("output") : _("input") ;
    std::string portname;
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
                << _("no") << " " << direction << " "
                << _("ports available")
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
                int p;
                for (p = 0; p < portcount; ++p)
                {
                    portname = rt->get_port_name(p);
                    std::cout
                        << "  " << direction << " " << _("port") << " #"
                        << p << ": " << portname
                        << std::endl
                        ;
                }
                do
                {
                    std::cout << _("Choose a port number") << ": ";
                    std::cin >> p;

                } while (p >= portcount);
                result = p;

                /*
                 * Set to clear and ignore the Enter after the port
                 * number, so that follow-on input requests will work.
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
        std::string msg =_("Unknown exception... fix the catch");
        errprint(msg.c_str());
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
choose_midi_port (RTMIDI_TYPE & rt, bool isoutput)
{
    bool result = true;
    if (rt_virtual_test_port())
    {
        rt.open_virtual_port();
    }
    else
    {
        int portnumber = rt_choose_port_number(isoutput);
        result = portnumber >= 0;
        if (result)
            result = rt.open_port(portnumber);
    }
    return result;
}

bool
rt_choose_input_port (rtl::rtmidi_in & rtin)
{
    return choose_midi_port<rtl::rtmidi_in>(rtin, false);  /* input   */
}

bool
rt_choose_output_port (rtl::rtmidi_out & rtout)
{
    return choose_midi_port<rtl::rtmidi_out>(rtout, true); /* output */
}

/**
 *  This stuff provides a very simple set of command-line options, mostly
 *  for the test applications.
 *
 *  TO DO: Mark this for translation.
 */

static const char * s_help_text_fmt =

"Usage: %s [ options ]\n\n"
"Runs basic tests for some APIs of Rtl66 library, v. %s.\n"
"It is based on a greatly refactored adaptation of RtMidi v. %s library.\n\n"
"Without options it uses the first MIDI API that is compiled in, falling\n"
"back to the next API if the first is not detected.  For example, in Linux\n"
"JACK will be tried first.  If not detected, then ALSA will be tried.  One\n"
"issue is that, on newer systems, jackdbus can fool JACK detection into\n"
"finding JACK, while using JACK will fail. Also, use the --quiet option to\n"
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
"  --quiet          Send console output to the bit bucket.\n"
#if defined RTL66_BUILD_ALSA
"  --alsa           Instead of the JACK/ALSA fallback, try ALSA, and fail if it\n"
"                   cannot be initialized.\n"
#endif
#endif              // defined PLATFORM_LINUX
"  --virtual        Use virtual ports (not available to some MIDI engines).\n"
"  --auto-connect   For non-virtual ports, get the existing system ports and\n"
"                   try to connect to them.\n"
"  --ppqn value     Change the default PPQN from 192 to the given value.\n"
"  --bpm value      Change the default BPM from 120.0 to the given value.\n"
"  --client cn      Provide a client name (e.g. to be shown in JACK graph.\n"
"  --port p         Set the test port, for applications that use that option.\n"
"  --port-in p      Set the input test port, for apps that need I/O ports.\n"
"  --port-out p     Set the output test port, for apps that need I/O ports.\n"
"  --port-name n    Provides a test name for the port. Otherwise empty.\n"
"  --length p       Set the amount of test data, if applicable.\n"
"  -h, --help       Show this help text.\n"
"\n"
;

/**
 *  Specifies to create a single virtual port for testing.
 */

static bool s_virtual_test_port = false;

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
 */

static int s_test_port = (-1);
static int s_test_port_in = (-1);
static int s_test_port_out = (-1);

static int
string_to_int (const std::string & s)
{
    int result = (-1);
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

static void
set_test_port (int portnumber)
{
    s_test_port = portnumber;
}

int
rt_test_port ()
{
    return s_test_port;
}

static void
set_test_port_in (int portnumber)
{
    s_test_port_in = portnumber;
#if defined RTL66_USE_GLOBAL_CLIENTINFO
    midi::global_client_info().input_portnumber(portnumber);
#endif
}

int
rt_test_port_in ()
{
    return s_test_port_in;
}

static void
set_test_port_out (int portnumber)
{
    s_test_port_out = portnumber;
#if defined RTL66_USE_GLOBAL_CLIENTINFO
    midi::global_client_info().output_portnumber(portnumber);
#endif
}

int
rt_test_port_out ()
{
    return s_test_port_out;
}

bool
rt_test_port_valid (int port)
{
    return port >= 0;
}

/**
 *  Specifies the test port name (where applicable).
 */

static std::string s_test_port_name;                    /* empty to start   */

static void
set_test_port_name (const std::string & portname)
{
    s_test_port_name = portname;
}

const std::string &
rt_test_port_name ()
{
    return s_test_port_name;
}

/**
 *  Specifies the test data-length (where applicable).
 */

static int s_test_data_length = (-1);

void
set_test_data_length (int len)
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

/**
 *  This static function processes the command-line.  It returns true if no
 *  help or version option was specified, and means the program can run
 *  normally.
 */

bool
rt_simple_cli (const std::string & appname, int argc, char * argv [])
{
    bool can_run = true;
    rtl::rtmidi::api rapi = rtl::rtmidi::api::unspecified;
    set_test_port(-1);
    set_test_port_in(-1);
    set_test_port_out(-1);
    set_test_port_name("");
    std::cout << "Application: '" << appname << "'" << std::endl;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            set_show_help(true);
            can_run = false;
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
        else if (arg == "--virtual")
        {
            set_virtual_test_port(true);
#if defined RTL66_USE_GLOBAL_CLIENTINFO
            midi::global_client_info().virtual_ports(true);
            midi::global_client_info().auto_connect(false);
#endif
        }
        else if (arg == "--auto-connect")
        {
#if defined RTL66_USE_GLOBAL_CLIENTINFO
            midi::global_client_info().virtual_ports(false);
            midi::global_client_info().auto_connect(true);
#endif
        }
        else if (arg == "--ppqn")
        {
#if defined RTL66_USE_GLOBAL_CLIENTINFO
            if (i + 1 < argc)
            {
                try
                {
                    std::string value = std::string(argv[i + 1]);
                    int v = string_to_int(value);
                    midi::ppqn ppq = static_cast<midi::ppqn>(v);
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
#endif
        }
        else if (arg == "--bpm")
        {
#if defined RTL66_USE_GLOBAL_CLIENTINFO
            if (i + 1 < argc)
            {
                try
                {
                    std::string value = std::string(argv[i + 1]);
                    double v = std::stod(value);
                    midi::bpm b = static_cast<midi::ppqn>(v);
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
#endif
        }
        else if (arg == "--client")
        {
#if defined RTL66_USE_GLOBAL_CLIENTINFO
            if (i + 1 < argc)
            {
                std::string value = std::string(argv[i + 1]);
                midi::global_client_info().client_name(value);
            }
#endif
        }
        else if (arg == "--length")
        {
            if (i + 1 < argc)
            {
                std::string value = std::string(argv[i + 1]);
                int v = string_to_int(value);
                set_test_data_length(v);
            }
        }
        else if (arg == "--port")
        {
            if (i + 1 < argc)
            {
                std::string value = std::string(argv[i + 1]);
                int v = string_to_int(value);
                set_test_port(v);
            }
        }
        else if (arg == "--port-in")
        {
            if (i + 1 < argc)
            {
                std::string value = std::string(argv[i + 1]);
                int v = string_to_int(value);
                set_test_port_in(v);
            }
        }
        else if (arg == "--port-out")
        {
            if (i + 1 < argc)
            {
                std::string value = std::string(argv[i + 1]);
                int v = string_to_int(value);
                set_test_port_out(v);
            }
        }
        else if (arg == "--port-name")
        {
            if (i + 1 < argc)
            {
                std::string value = std::string(argv[i + 1]);
                set_test_port_name(value);
            }
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
            s_help_text_fmt,
            appname.c_str(),
            rtl::get_rtl_midi_version().c_str(),
            rtl::get_rtmidi_patch_version().c_str()
        );
    }
    return can_run;
}

/*
 * test_helpers.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

