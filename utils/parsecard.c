#include <stdio.h>

#define EVENP(d0,d1,d2,d3,parity)   (((d0)+(d1)+(d2)+(d3)+(parity)) % 2 == 0)

char rawData[] = "111111111111111111111111100101111101111011110111101111011110111101001001010111101011101111001100101010111011110000001111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
int rawPos = 0;

#define BUFLEN 26
unsigned char buffer[BUFLEN];
int bufferPos = 0;

void store_data(int data) 
{
    buffer[bufferPos] = data;
    bufferPos = (bufferPos+1) % BUFLEN;
}

int read_data()
{
    char b = rawData[rawPos];
    if (b == 0) return -1;
    rawPos++;
    if (b == '1') return 0;
    return 1;
}

int convert_binary(int start, int bits)
{
    int value = 0;
    int mod = 1;
    int n;
    for (n = bits-1; n >= 0; n--) {
        if (buffer[(bufferPos+start+n) % BUFLEN] == 1) {
            value += mod;
        }
        mod = mod * 2;
    }
    return value;
}

int main(void)
{
    int data = 0, d0, d1, d2, d3, parity, n;
    int facility, card;

    /* Skip the first 25 bits of data (make sure they are zeros) */
    for (n = 0; n < 25; n++)
    {
        data = read_data();
        if (data != 0) {
            /* Error */
            printf("leading zeros expected\n");
        }
    }

    /* Read in groups of 5-bits */
    int start = 1;
    while (1) 
    {
        /* The bits are sent LSB first */
        d0 = read_data();
        if (d0 == -1) {
            /* End of data */
            break;
        }

        /* Read the rest of the data */
        d1 = read_data();
        d2 = read_data();
        d3 = read_data();
        parity = read_data();

        if (d1 == -1 || d2 == -1 || d3 == -1 || parity == -1) {
            printf("premature end of data\n");
            break;
        }

        /* Verify the parity bit (odd parity) */
        if (EVENP(d0,d1,d2,d3,parity)) {
            printf("parity failure on 5-bit chunk\n");
        }

        if (start) {
            /* The first segment should be 0xB == 1011B */
            if (! (d0 & d1 & !d2 & d3) ) {
                /* Invalid starting segment */
                printf("invalid start segment %d %d %d %d\n", d0, d1, d2, d3);
            }
        } else {
            /* Check for the ending segment 0xF == 1111B */
            if (d0 & d1 & d2 & d3) {
                /* End of data sequence, the LRC follows */
                d0 = read_data();
                d1 = read_data();
                d2 = read_data();
                d3 = read_data();
                parity = read_data();
                if (EVENP(d0,d1,d2,d3,parity)) {
                    printf("LRC parity failure\n");
                }
                /* The parity for LRC checks out, verify the actual LRC */

                /* Consume the remaining zeros */
                while (1) {
                    data = read_data();
                    if (data == -1) {
                        break;
                    }
                    if (data != 0) {
                        printf("trailing data must be zeros\n");
                    }
                }
                break;
            }

            if (d3 != 0) {
                printf("data segments must have zero padding\n");
            }

            /* Otherwise this is a chunk of card data. Store it in a circular
             * buffer of 26 bits, so when we are finished iterating we are
             * left with the last 26 bits of input data (rest is discarded) */
            printf("%d%d%d\n", d2, d1, d0);
            store_data(d2);
            store_data(d1);
            store_data(d0);
        }
        start = 0;
    }

    /* Check the data parity (first and last bit */

    /* Dump the data */
    for (n = 1; n < BUFLEN-1; n++) {
        printf("%d", buffer[(bufferPos+n) % BUFLEN]);
    }
    printf("\n");

    /* Extract the facility ID */
    facility = convert_binary(1, 8);
    printf("%d\n", facility);

    card = convert_binary(9, 16);
    printf("%d\n", card);

}

