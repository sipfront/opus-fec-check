#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <opus/opus.h>
#include <pcap.h>

unsigned int packet_count = 0;

void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {

    unsigned char *payload;
    int n, payload_len;

    opus_int16 frame_sizes[48];
    const unsigned char *frame_data[48];
    int opus_frame_count, silk_frame_count;

    struct ether_header *eth_header;
    const u_char *ip_header;
    const struct udphdr *udp_header;
    const u_char *udp_payload;

    int ethernet_header_length = 14;
    int ip_header_length;
    int udp_header_length;
    int payload_length;
    u_char protocol;


    //printf("Processing next packet...\n");
    packet_count++;

    eth_header = (struct ether_header *) packet;
    if (ntohs(eth_header->ether_type) != ETHERTYPE_IP) {
        //printf("Not an IP packet, skipping.\n");
        return; 
    }

    ip_header = packet + ethernet_header_length;
    ip_header_length = ((*ip_header) & 0x0f);
    ip_header_length = ip_header_length * 4;

    protocol = *(ip_header + 9);
    if (protocol != IPPROTO_UDP) {
        //printf("Not a UDP packet, skipping.\n");
        return;
    }

    udp_header = (struct udphdr*)(packet + ethernet_header_length + ip_header_length);
    udp_header_length = sizeof(*udp_header);

    udp_payload = packet + ethernet_header_length + ip_header_length + udp_header_length;

    if ((udp_payload[0] >> 6) != 2) {
        //printf("Not an RTP packet, skipping.\n");
        return;
    }

    if ((udp_payload[0] >> 4) & 0x1) {
        //printf("RTP packet has extension bit set, not supported, skipping.\n");
        return;
    }
    if (udp_payload[0] & 0xf) {
        //printf("RTP packet has csrc counters set, not supported, skipping.\n");
        return;
    }

    payload = (unsigned char*)udp_payload + 12;
    payload_len = payload - packet;

    //printf("Successfully extracted %d bytes payload from RTP packet\n", payload_len);

    if (payload[0] & 0x80) {
        //printf("Payload is in CELT mode, there is no FEC ever\n");
        return;
    }

    if ((opus_frame_count = opus_packet_parse(payload, payload_len, NULL, frame_data, frame_sizes, NULL)) <= 0) {
        //printf("Failed to parse opus payload\n");
        return;
    }

    //printf("Successfully parsed %d opus frames\n", opus_frame_count);

    if ((payload[0] >> 3) >= 16) { // 0-11 silk, 12-15 hybrid, 16+ celt
        //printf("FEC is only in SILK\n");
        return;
    }

    silk_frame_count = (payload[0] >> 3) & 0x3;
    if(silk_frame_count  == 0) {
        silk_frame_count = 1;
    }
    //printf("Successfully parsed %d SILK frames\n", silk_frame_count);

    if (silk_frame_count == 1 && opus_frame_count == 1) {
        for (n = 0; n <= (payload[0] & 0x4); ++n) {
            if (frame_data[0][0] & (0x80 >> ((n + 1) * (silk_frame_count + 1) - 1))) {
                printf("FEC frame found in packet %d!\n", packet_count);
                return;
            }
        }
    } else {
        opus_int16 LBRR_flag = 0;
        for (n = 0 ; n < opus_frame_count; ++n) {
            LBRR_flag = (frame_data[n][0] >> (7 - silk_frame_count)) & 0x1;
            if (LBRR_flag) {
                printf("FEC frame found in packet %d!\n", packet_count);
                return;
            }
        }
    }

    //printf("No FEC frame found..\n");
    return;
}

int main(int argc, char** argv) {

    pcap_t *pcap;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (argc != 2) {
        printf("Usage: %s <file.opus>\n", argv[0]);
        return -1;
    }

    if ((pcap = pcap_open_offline(argv[1], errbuf)) == NULL) {
        printf("Failed to open pcap file: %s\n", errbuf);
        return -1;
    }
    pcap_loop(pcap, 0, packet_handler, NULL);

    printf("Done processing pcap file\n");
    return 0;
}
