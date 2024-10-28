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
 * \file          api_names.cpp
 *
 *      A test-file for the API names, codes, and lookups.
 *
 * \library       rtl66
 * \author        Jean Pierre Cimalando; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-01-31
 * \license       See above.
 *
 *      apinames.cpp
 *      by Jean Pierre Cimalando, 2018.
 *
 *      This program tests parts of RtMidi related to API names, the
 *      conversion from name to API and vice-versa.
 */

#include <iostream>                     /* std::cout, std::cin              */

#include "rtl66-config.h"               /* RTL66_HAVE_DUMMY_H test          */
#include "rtl/rt_types.hpp"             /* rtl::rtmidi::api_list            */
#include "rtl/midi/rtmidi_c.h"          /* for testing the C interfaces     */
#include "rtl/midi/rtmidi.hpp"          /* the rtl::rtmidi C++ interface    */

#if RTL66_HAVE_DUMMY_H                  /* the proper way to test the macro */
#error You big dummy!
#endif

static int
test_cpp ()
{
    rtl::rtmidi::api_list compiled_apis;
    rtl::rtmidi::get_compiled_apis(compiled_apis);
    rtl::rtmidi::show_apis("Compiled APIs", compiled_apis);

    rtl::rtmidi::api_list detected_apis;
    rtl::rtmidi::get_detected_apis(detected_apis);
    rtl::rtmidi::show_apis("Detected APIs", detected_apis);

    /*
     * Ensure the known APIs return valid names.
     */

    const rtl::rtmidi::api_list & apis = compiled_apis;
    std::cout << "API names by identifier (C++):" << std::endl;
    for (size_t i = 0; i < apis.size(); ++i)
    {
        const std::string name = rtl::rtmidi::api_name(apis[i]);
        if (name.empty())
        {
            std::cout << "Invalid name for API " << int(apis[i]) << std::endl;
            exit(EXIT_FAILURE);
        }
        const std::string dispname = rtl::rtmidi::api_display_name(apis[i]);
        if (dispname.empty())
        {
            std::cout << "Invalid display name for API "
                << int(apis[i]) << std::endl
                ;
            exit(EXIT_FAILURE);
        }
        std::cout
            << "-  " << int(apis[i]) << " '" << name << "': '"
            << dispname << "'" << std::endl
            ;
    }

    /*
     * Ensure unknown APIs return the empty string.
     */

    rtl::rtmidi::api bogus = rtl::rtmidi::api::max;
    const std::string name = rtl::rtmidi::api_name(bogus);
    if (! name.empty())
    {
        std::cout << "Bad string for invalid API '" << name << "'" << std::endl;
        exit(EXIT_FAILURE);
    }
    const std::string dispname = rtl::rtmidi::api_display_name(bogus);
    if (! dispname.empty())
    {
        std::cout << "Invalid API code" << std::endl;
        exit(EXIT_FAILURE);
    }

    /*
     * Try getting the API identifier by name.
     */

    std::cout << "API identifiers by name (C++):" << std::endl;
    for (auto a: apis)                          /* might only be a couple   */
    {
        std::string name = rtl::rtmidi::api_name(a);
        if (rtl::rtmidi::api_by_name(name) != a)
        {
            std::cout << "Bad identifier for API '" << name << "'" << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cout << "-  '" << name << "': " << int(a) << std::endl;
        for (size_t j = 0; j < name.size(); ++j)
            name[j] = (j & 1) ? toupper(name[j]) : tolower(name[j]);

        rtl::rtmidi::api rapi = rtl::rtmidi::api_by_name(name);
        if (rapi != rtl::rtmidi::api::unspecified)
        {
            std::cout << "Identifier " << int(rapi)
                << " for invalid API '" << name << "'" << std::endl
                ;
            exit(EXIT_FAILURE);
        }
    }

    /*
     * Try getting an API identifier by an unknown name.
     */

    rtl::rtmidi::api rapi = rtl::rtmidi::api_by_name("");
    if (rapi != rtl::rtmidi::api::unspecified)
    {
        std::cout << "Bad identifier for unknown API name\n";
        exit(EXIT_FAILURE);
    }
    return 0;
}


/*
* Now test the C inteface.
*/

static int
test_c ()
{
    unsigned api_count = rtmidi_get_compiled_apis(nullptr, 0);
    std::vector<RtMidiApi> apis(api_count);
    rtmidi_get_compiled_apis(apis.data(), api_count);

    /*
     * We have to cast because this module isn't C, but C++.
     */

    RtMidiApi * detected_apis = (RtMidiApi *) calloc(8, sizeof(RtMidiApi));
    int detected_count = rtmidi_get_detected_apis(detected_apis, 8);
    std::cout << "Detected APIs:" << std::endl;
    for (int i = 0; i < detected_count; ++i)
    {
        const char * name = rtmidi_api_name(detected_apis[i]);
        const char * displayname = rtmidi_api_display_name(detected_apis[i]);
        printf("%12s: %s\n", name, displayname);
    }

    /*
     * Ensure the known APIs return valid names.
     */

    bool ok = true;
    std::cout << "API names by identifier (C):" << std::endl;
    for (size_t i = 0; i < api_count; ++i)
    {
        const std::string name = rtmidi_api_name(apis[i]);
        if (name.empty())
        {
            std::cout << "Invalid API code " << int(apis[i]) << std::endl;
            ok = false;
        }
        const std::string dispname = rtmidi_api_display_name(apis[i]);
        if (dispname.empty())
        {
            std::cout << "Invalid API code " << int(apis[i]) << std::endl;
            ok = false;
        }
        else if (dispname == "Fallback" && i > 0)
        {
            std::cout
                << "Invalid name for valid code "
                << int(apis[i]) << std::endl
                ;
            ok = false;
        }
        std::cout
            << "-  " << int(apis[i]) << " '"
            << name << "': '" << dispname << "'" << std::endl
            ;
    }
    if (! ok)
        exit(EXIT_FAILURE);

    /*
     * Ensure unknown APIs return the empty string.
     */

    rtl::rtmidi::api bogus = static_cast<rtl::rtmidi::api>(RTMIDI_API_MAX);
    const std::string name = rtl::rtmidi::api_name(bogus);
    if (! name.empty())
    {
        std::cout << "Bad string for invalid API '" << name << "'" << std::endl;
        exit(EXIT_FAILURE);
    }
    const std::string dispname  = rtl::rtmidi::api_display_name(bogus);
    if (! dispname.empty())
    {
        std::cout << "Invalid API code" << std::endl;
        exit(EXIT_FAILURE);
    }

    /*
     * Try getting API identifier by name.
     */

    std::cout << "API identifiers by name (C):" << std::endl;
    for (size_t i = 0; i < api_count ; ++i)
    {
        const char * s = rtmidi_api_name(apis[i]);
        std::string name = not_nullptr(s) ? s : "";
        if (rtmidi_api_by_name(name.c_str()) != apis[i])
        {
            std::cout << "Bad identifier for API '" << name << "'" << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cout << "-  '" << name << "': " << int(apis[i]) << std::endl;
        for (size_t j = 0; j < name.size(); ++j)
            name[j] = (j & 1) ? toupper(name[j]) : tolower(name[j]);

        RtMidiApi midiapi = rtmidi_api_by_name(name.c_str());
        if (midiapi != RTMIDI_API_UNSPECIFIED)
        {
            std::cout << "Identifier " << int(midiapi)
                << " for invalid API '" << name << "'" << std::endl
                ;
            exit(EXIT_FAILURE);
        }
    }

    /*
     * Try getting an API identifier by unknown name
     */

    RtMidiApi midiapi = rtmidi_api_by_name("");
    if (midiapi != RTMIDI_API_UNSPECIFIED)
    {
        std::cout << "Bad identifier for unknown API name\n";
        exit(EXIT_FAILURE);
    }
    return 0;
}

int
main (int argc, char * argv [])
{
    if (argc > 1)
    {
        std::cout
            << argv[0]
            << " needs no options. It merely shows the APIs compiled in.\n"
            << " Continuing..."
            << std::endl
            ;
    }
    test_cpp();
    test_c();
    std::cout << "All tests in 'api_names' passed." << std::endl;
}

/*
 * api_names.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

