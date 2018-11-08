/* Comment block
   CS656-005 Group W6
   Le (lg282), Su (ss3889), Chih-Yun (cc785), Shang-Yung (sh555), Ge (gg345), Quzhi (ql247)
*/

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
    struct sockaddr_in server, client;
    char buffer[128] = "";
    char mes[] = "www.berkeley.edu\n REQ www.berkeley.edu\n  IP = 2.3.4.5\n  IP = 1.2.3.4\n  IP = 1.2.3.4\n  PREFERED IP = 2.3.4.5\n";
    int clientAddrLen = sizeof(client);
    bzero(&server, sizeof(server));
    bzero(&client, sizeof(client));

    server.sin_family = AF_INET;
    server.sin_port = htons(3565);
    server.sin_addr.s_addr = inet_addr("0.0.0.0");

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        perror("Create Socket");
        return 1;
    }
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0)
    {
        perror("Bind");
        return 1;
    }
    if (listen(sockfd,3) < 0)
    {
        perror("Listen");
        return 1;
    }
    while(1)
    {
        int acpt = accept(sockfd, (struct sockaddr*)&client, (socklen_t*)&clientAddrLen);
        if (acpt < 0)
        {
            perror("Accept");
            continue;
        }
        printf ("Incoming client connection from [%s:%d] \n", inet_ntoa(client.sin_addr), htons(client.sin_port));
        printf (" to me [%s:%d] \n", inet_ntoa(server.sin_addr), htons(server.sin_port));
        if (send(acpt, mes, sizeof(mes), 0))
        {
            perror("Send");
            continue;
        }
    }
}
