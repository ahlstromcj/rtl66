#!/usr/bin/sed -i -f

s/SEQ66/RTL66/g
s/is part of seq66/is part of rtl66/g
s/seq66 is/rtl66 is/g
s/seq66;/rtl66;/g
s/seq66 application/rtl66 library/g

s/midi::bussbyte/midi::bussbyte/g
s/midi::midibyte/midi::byte/g
s/midi::midibytes/midi::bytes/g
s/midi::midibool/midi::boolean/g
s/midi::midibooleans/midi::booleans/g
s/midi::midishort/midi::ushort/g
s/midi::miditag/midi::tag/g
s/midi::midilong/midi::ulong/g
s/midi::midipulse/midi::pulse/g
s/midi::midibpm/midi::bpm/g
s/midi::midippqn/midi::ppqn/g
s/midi::midistring/midi::bytestring/g

s/midibyte/byte/g
s/midibytes/bytes/g
s/midibool/boolean/g
s/midibooleans/booleans/g
s/midishort/ushort/g
s/miditag/tag/g
s/midilong/ulong/g
s/midipulse/pulse/g
s/midibpm/bpm/g
s/midippqn/ppqn/g
s/midistring/bytestring/g
s/tokenization/lib66::tokenization/g

s/bytes\.hpp/midibytes.hpp/
s/basic_macros/cpp_types/g



