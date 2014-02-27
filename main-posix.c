#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "coap.h"

#define PORT 5683
#define HOST "54.207.46.52"

int main(int argc, char **argv)
{
    int fd, rc;
    struct sockaddr_in servaddr, cliaddr;

    uint8_t buf[4096];
    uint8_t scratch_raw[4096];
    coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};
    fd = socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    // solve hostname
    struct hostent *he;
    struct in_addr **addr_list;
    he = gethostbyname(HOST);
    addr_list = (struct in_addr **) he->h_addr_list;
    inet_aton(inet_ntoa(*addr_list[0]), &servaddr.sin_addr);

    size_t sndlen = sizeof(buf);
    coap_packet_t sndpkt;
    coap_make_req_observe(&scratch_buf, &sndpkt, "time", "serial_number=123-456-789");

    if (0 != (rc = coap_build(buf, &sndlen, &sndpkt)))
            printf("coap_build failed rc=%d\n", rc);
    else
    {
#ifdef DEBUG
        printf("Sending: ");
        coap_dump (buf, sndlen, true);
        printf("\n");
#endif
#ifdef DEBUG
        coap_dumpPacket(&sndpkt);
#endif
        sendto(fd, buf, sndlen, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    }

    while(1)
    {
        int n, rc;
        socklen_t len = sizeof(cliaddr);
        coap_packet_t pkt;

        n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len);
#ifdef DEBUG
        printf("Received: ");
        coap_dump(buf, n, true);
        printf("\n");
#endif

        if (0 != (rc = coap_parse(&pkt, buf, n)))
            printf("Bad packet rc=%d\n", rc);
        else
        {
            size_t rsplen = sizeof(buf);
            coap_packet_t rsppkt;
#ifdef DEBUG
            coap_dumpPacket(&pkt);
#endif
			//ack for package received
			//rsppkt->hdr.code = pkt->hdr.code;
            coap_make_req_observe_ack(&scratch_buf, &rsppkt, pkt.hdr.id[0], pkt.hdr.id[1]);
            
			if (0 != (rc = coap_build(buf, &rsplen, &rsppkt)))
                printf("coap_build failed rc=%d\n", rc);
            else
            {
#ifdef DEBUG
                printf("Sending: ");
                coap_dump(buf, rsplen, true);
                printf("\n");
#endif
#ifdef DEBUG
                coap_dumpPacket(&rsppkt);
#endif

                sendto(fd, buf, rsplen, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
            }
        }
    }
}
