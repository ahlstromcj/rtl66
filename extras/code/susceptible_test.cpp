
/*
 * This code was extracted from tests/midi/busin.cpp.
 */

#if defined USE_SUSCEPTIBLE_TEST

/**
 *  A usage that breaks (can cause segfaults) in ALSA because the RtMidi-based
 *  implementation uses a polling thread, but ALSA is not thread-safe, and thus
 *  cannot be used with a single ALSA client and multiple ports.
 */

bool
run_susceptible_test (rtl::rtmidi::api rapi, int portno)
{
    midi::masterbus & master { master_bus(rapi, app_client_info()) };
    midi::bus_in & busin { master.get_in_bus(portno) };
    bool result { busin.initialize() };
    if (result)
    {
        try
        {
            /*
             * Don't ignore sysex, timing, or active sensing
             * messages. Install an interrupt handler function.
             * Periodically check input queue.
             *
             * busin.ignore_midi_types(false, false, false);
             */

            if (rt_use_callback())
            {
                /*
                 * Disabled, occurs too late in the process.
                 *
                 * busin.set_input_callback(&midibytes_callback);
                 */

                std::cout
                    << "Reading MIDI input ... press <Enter> to quit.\n"
                    ;

                char input;
                std::cin.get(input);
            }
            else
            {
                s_is_done = false;
                (void) signal(SIGINT, finish);
                std::cout
                    << "Reading MIDI from port "
                    << busin.port_name()
                    << " ... quit with any key or <Ctrl-C>."
                    << std::endl
                    ;
                xpc::clear_kb_ex();
                while (! s_is_done)
                {
                    midi::message msg { busin.get_message() };
                    if (msg.count() > 0)
                    {
                        std::string msgline { "Msg:" };
                        msgline += msg.to_string();
                        util::status_message(msgline);
                    }
                    if (xpc::kbcheck_ex())
                        break;

                    rt_test_sleep(10);  /* sleep for 10 msec    */
                }
            }
        }
        catch (const rtl::rterror & error)
        {
            std::cerr << "Caught rtl::rterror!" << std::endl;
            result = false;
        }
    }
    return result;
}

#endif

#if defined USE_SUSCEPTIBLE_TEST
            bool ok { run_susceptible_test(portno) };
            if (ok)
                ok = poll_port(portno);

            bool ok { poll_port(portno) };

            if (! ok)
                success = false;
#endif


