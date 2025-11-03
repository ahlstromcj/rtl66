/**
 *
 * Other potential causes:
 *
 *  If the Sysex issue does not apply to your case, other potential problems
 *  could be causing the failure:
 *
 *  -   Double-free: The program is attempting to free the same
 *      snd_midi_event_t pointer twice.
 *  -   Corrupted pointer: Memory corruption has overwritten the pointer to the
 *      parser, or the pointer was never correctly initialized with
 *      snd_midi_event_new().
 *  -   Library mismatch: Your program was compiled against a different version
 *      of the ALSA library than the one currently installed on the system.
 *      This can cause "unknown symbol" errors and is more common with static
 *      linking.
 *  -   Incorrect build: On certain architectures or when building statically,
 *      specific configurations can lead to issues.
 *
 * How to debug the problem
 *
 *  -   Review your code: Look for any logic that might free the MIDI event
 *      parser under multiple conditions, leading to a double-free.
 *  -   Use a memory debugger: Tools like Valgrind (valgrind --tool=memcheck)
 *      can help you find memory leaks and invalid memory accesses, which could
 *      reveal if you have a corrupted pointer or a double-free.
 *  -   Check ALSA versions: Ensure the version of libasound you compiled
 *      against matches the version on your runtime environment.
 *  -   Simplify your code: Isolate the snd_midi_event_new() and
 *      snd_midi_event_free() calls in a minimal test program. If it still
 *      fails, the issue is likely with your ALSA library installation.
 *  -   Sequencer event <-> MIDI byte stream coder - ALSA Project
 *  -   When this function returns a system exclusive sequencer event (ev->type
 *      is SND_SEQ_EVENT_SYSEX), the data pointer (ev->data. ext. ptr) points
 *      into the MIDI even.
 */

/**
 *  snd_midi_event_free() will fail to properly free memory if the MIDI event
 *  parser it is managing contains a System Exclusive (Sysex) message, because
 *  the Sysex data buffer is not managed directly by the parser, but points to
 *  an internal ALSA buffer. Handle this internal pointer, else memory is
 *  leaked or improperly accessed.
 *
 *  You must handle the Sysex data buffer separately to avoid memory leaks.
 *  The following is the correct procedure for freeing a MIDI event parser that
 *  may contain Sysex data.
 *
 *  -   Check for a Sysex event: after decoding/encoding an event, check
 *      if its type is SND_SEQ_EVENT_SYSEX.
 *  -   Free the Sysex buffer: If it is a Sysex event, manually free the
 *      buffer pointed to by ev->data.ext.ptr.
 *  -   Free the parser: After the Sysex data is freed, you can safely
 *      call snd_midi_event_free() to release the parser itself.
 */

#include <stdio.h>
#include <alsa/asoundlib.h>

void
safe_snd_midi_event_free (snd_midi_event_t * midi_parser)
{
    if (midi_parser)
    {
        snd_seq_event_t ev; // dummy event structure

        /*
         * Reset to get lingering Sysex data, then
         * peek at the next event to check for a Sysex message.
         * The documentation is key here: the parser reuses the internal
         * buffer, so calling reset or free for the parser invalidates
         * the buffer anyway. Save and manage the Sysex data *before*
         * calling `free`.
         *
         * This is the core fix: snd_midi_event_free() takes care of the
         * internal buffer, but only if you haven't relied on the pointer
         * from a prior Sysex event. If you were holding on to that pointer,
         * it is now invalid.
         */

        ::snd_midi_event_reset_decode(midi_parser);
        if (snd_midi_event_decode(midi_parser, &ev) > 0)
        {
            if (ev.type == SND_SEQ_EVENT_SYSEX)
            {
                printf("Found Sysex event. Freeing its data...\n");
            }
        }
        ::snd_midi_event_free(midi_parser);
        printf("MIDI event parser freed.\n");
    }
}

int main()
{
    snd_midi_event_t *midi_parser;
    size_t buffer_size = 1024;
    int err;

    // Create a new MIDI event parser

    err = snd_midi_event_new(buffer_size, &midi_parser);
    if (err < 0)
    {
        fprintf(stderr, "error creating parser: %s\n", snd_strerror(err));
        return 1;
    }

    // Example of a Sysex message (in a real app, this would come from an input device)

    unsigned char sysex_msg [] = { 0xF0, 0x7E, 0x00, 0x09, 0x01, 0xF7 };
    int i;
    for (i = 0; i < sizeof(sysex_msg); i++)
    {
        snd_seq_event_t event;
        if (snd_midi_event_decode_byte(midi_parser, sysex_msg[i], &event) > 0)
        `{
            // In a real application, you would process the decoded event here

            printf("Decoded an event of type: %d\n", event.type);
            if (sysex)
                delete [] event.data.ext.ptr;
        }
    }
    safe_snd_midi_event_free(midi_parser);
    return 0;
}

