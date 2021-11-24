# Tools to verify FEC in Opus RTP payload

Based on Freeswitch `switch_opus_has_fec()` function (c) Anthony Minessale II et al.

## Install

Install gcc, make, libpcap-dev and libopus-dev, then run `make`.

## has_opus_fec

A tool to check if a raw Opus payload contains FEC frames. To obtain a raw Opus frame,
capture a call with tcpdump, open with Wireshark, click on an RTP packet, expand the Real-Time Transport Protocol
section, right-click on the Payload and click Export Packet Bytes.

Run the tool with `has_opus_fec opus.raw` to check if FEC is carried in the payload.

## iterate_opus_fec

A tool which iterates over all packets of a pcap file and checks if it's RTP and whether its payload
carries FEC.

Run the tool with `iterate_opus_fec opus.pcap` to check which packets carry FEC in the payload.

# License

Version: MPL 1.1

The contents of this repo are subject to the Mozilla Public License Version
1.1 (the "License"); you may not use this file except in compliance with
the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/
