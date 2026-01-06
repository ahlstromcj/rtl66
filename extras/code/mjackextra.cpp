
/**
 *  An overload to get both sets of I/O ports at once.
 *  EXPERIMENTAL.
 */

int
midi_jack::get_io_port_info
(
    midi::ports & inports,
    midi::ports & outports
)
{
    int result { 0 };
    bool iswriteable { false };
    midi_jack_data & data { jack_data() };
    if (not_nullptr(data.jack_client()))
    {
        for (int i = 0; i < 2; ++i)
        {
            midi::port::io iotype
            {
                iswriteable ? midi::port::io::output : midi::port::io::input
            };
            midi::ports & ioports { iswriteable ? outports : inports };
            unsigned long flag
            {
                iswriteable ? JackPortIsInput : JackPortIsOutput
            };
            const char ** ports = ::jack_get_ports
            (
                data.jack_client(), NULL, RTL66_JACK_MIDI_TYPE, flag
            );
            if (is_nullptr(ports))
            {
                std::string msg { "found no " };
                msg += iswriteable ? "writeable" : "readable" ;
                msg += " ports";
                error_print("jack_get_ports()", msg);
            }
            else
            {
                int clientnumber { 0 };             /* JACK: doesn't apply  */
                int count { 0 };
                while (not_nullptr(ports[count]))
                {
                    std::string fullname { ports[count] };
                    std::string clientname;
                    std::string portname;
                    lib66::tokenization aliases { get_port_aliases(fullname) };
                    ioports.add
                    (
                        fullname,
                        aliases,
                        clientnumber,                       /* buss number  */
                        count,                              /* port number  */
                        iotype,
                        midi::port::kind::normal,
                        count,                              /* port ID      */
                        0                                   /* queue number */
                    );
                    ++count;
                }
                ::jack_free(ports);
                result += count;
            }
            iswriteable = ! iswriteable;
        }
    }
    return result;
}
