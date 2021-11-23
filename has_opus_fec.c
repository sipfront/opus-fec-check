#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <opus/opus.h>

int main(int argc, char** argv) {

    FILE *stream;
    unsigned char payload[1024];
    int n, payload_len;

    opus_int16 frame_sizes[48];
    const unsigned char *frame_data[48];
    int opus_frame_count, silk_frame_count;

    if (argc != 2) {
        printf("Usage: %s <file.opus>\n", argv[0]);
        return -1;
    }

    if ((stream = fopen(argv[1], "rb")) == NULL) {
        printf("Failed to open file: %s\n", strerror(errno));
        return -1;
    }
    payload_len = fread(&payload, sizeof(char), sizeof(payload), stream);
    fclose(stream);

    printf("Successfully read %d bytes from file\n", payload_len);

    if (payload[0] & 0x80) {
        printf("Payload is in CELT mode, there is no FEC ever\n");
        return 0;
    }

    if ((opus_frame_count = opus_packet_parse(payload, payload_len, NULL, frame_data, frame_sizes, NULL)) <= 0) {
        printf("Failed to parse opus payload\n");
        return -1;
    }

    printf("Successfully parsed %d opus frames\n", opus_frame_count);

    if ((payload[0] >> 3) >= 16) {
        printf("FEC is only in SILK\n");
        return 0;
    }

    silk_frame_count = (payload[0] >> 3) & 0x3;
    if(silk_frame_count  == 0) {
        silk_frame_count = 1;
    }
    printf("Successfully parsed %d SILK frames\n", silk_frame_count);

    if (silk_frame_count == 1 && opus_frame_count == 1) {
        for (n = 0; n <= (payload[0] & 0x4); ++n) {
            if (frame_data[0][0] & (0x80 >> ((n + 1) * (silk_frame_count + 1) - 1))) {
                printf("FEC frame found!\n");
                return 0;
            }
        }
    } else {
        opus_int16 LBRR_flag = 0;
        for (n = 0 ; n < opus_frame_count; ++n) {
            LBRR_flag = (frame_data[n][0] >> (7 - silk_frame_count)) & 0x1;
            if (LBRR_flag) {
                printf("FEC frame found!\n");
                return 0;
            }
        }
    }

    printf("No FEC frame found..\n");
    return 0;
}
