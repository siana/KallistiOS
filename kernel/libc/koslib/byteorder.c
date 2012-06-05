/* KallistiOS ##version##

   byteorder.c
   (c)2001 Dan Potter

*/

/* Byte-order translation functions */
#include <inttypes.h>

/* XXX needs to be in arch */

/* Network to Host short */
uint16_t ntohs(uint16_t value) {
    return ((value >> 8) & 0xff)
           | ((value << 8) & 0xff00);
}

/* Network to Host long */
uint32_t ntohl(uint32_t value) {
    return ((value >> 24) & 0xff)
           | (((value >> 16) & 0xff) << 8)
           | (((value >> 8) & 0xff) << 16)
           | (((value >> 0) & 0xff) << 24);
}

/* Host to Network short */
uint32_t htons(uint32_t value) {
    return ntohs(value);
}

/* Host to Network long */
uint32_t htonl(uint32_t value) {
    return ntohl(value);
}
