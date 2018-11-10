/* Comment block
   CS656-005 Group W6 
   Su (ss3889), Chih-Yun (cc785), Shang-Yung (sh555), Ge (gg345), Quzhi (ql247)
*/

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <string.h>

//Get time in microsecond
long long GetTime()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000 * 1000 + t.tv_usec;
}

//Calculate connection delay
int GetDelay(struct addrinfo info)
{
	//Create socket
	int sockfd = socket(info.ai_family, info.ai_socktype, info.ai_protocol);
	if(sockfd < 0)
        return -1;
	
	//Connect
	long long startT = GetTime();
	if (connect(sockfd, info.ai_addr, info.ai_addrlen) < 0)
	{
		close(sockfd);
		return -1;
	}
	int delay = GetTime() - startT;
	close(sockfd);
	
	return delay;
}

//Get IP addresses 
struct sockaddr_in * Dns(int acpt, char* addr)
{
	struct addrinfo* infos;
	struct addrinfo prefAddr;

	if (getaddrinfo(addr, NULL, NULL, &infos) == 0)
	{
		struct addrinfo info = infos[0];
		float minD = 10000000;

		struct sockaddr_in* address;
		
		//Get every node of the list, calculate delay, find the preferred address and then send to the client
		while(info.ai_next != NULL)
		{
			address = (struct sockaddr_in*)info.ai_addr;

			int delay = GetDelay(info);//Calculate delay
			if (delay >= 0)
			{
				if (delay < minD)
				{
					minD = delay;
					prefAddr = info;
				}
			}
			info = *info.ai_next;
		}
	}
	
	//Get preferred IP address by calculating the connection delay 
	struct sockaddr_in *addrin;
	addrin = (struct sockaddr_in*)prefAddr.ai_addr;
		
	return addrin;
}

//Scan and parse the browser request
int doParse(char* req, char* hostName, char* URL)
{
	int getHostStart = -1, getURLStart = -1;
	int bufferRealSize = 0;

	//Get real size of data
	for (bufferRealSize; bufferRealSize < sizeof(req); bufferRealSize++)
	{
		//Get URL
		if (bufferRealSize > 3 &&
			req[bufferRealSize - 4] == 'G' && req[bufferRealSize - 3] == 'E' && 
			req[bufferRealSize - 2] == 'T' && req[bufferRealSize - 1] == ' ')
			getURLStart = 1;

		if (getURLStart > 0)
		{
			strcat(URL, req[bufferRealSize]);
			if (req[bufferRealSize] == ' ')
			{
				getURLStart = -1;
				strcat(URL, '\0');
			}
		}

		//Get Host
		if (bufferRealSize > 5 &&
			req[bufferRealSize - 6] == 'H' && req[bufferRealSize - 5] == 'o' && req[bufferRealSize - 4] == 's' &&
			req[bufferRealSize - 3] == 't' && req[bufferRealSize - 2] == ':' && req[bufferRealSize - 1] == ' ')
			getHostStart = 1;

		if (getHostStart > 0)
		{
			strcat(hostName, req[bufferRealSize]);
			if (req[bufferRealSize] == '\r' || req[bufferRealSize] == ':')
				getHostStart = -1;
		}

		if (buffer[bufferRealSize] == '\0')
			break;
	}

	return bufferRealSize;
}

//Transfer data
void doHTTP()
{

}

int main(int argc, char **argv)
{
	struct sockaddr_in server, client;
	int clientAddrLen = sizeof(client);
	bzero(&server, sizeof(server));
	bzero(&client, sizeof(client));

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[1]));
	server.sin_addr.s_addr = inet_addr("0.0.0.0");
	
	//Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		exit(1);

	//Bind socket to address and port
	if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0)
		exit(1);

	//Begin to listen to specified port
	if (listen(sockfd,3) < 0)
		exit(1);

	printf("stage 2 program by ss3889 listening on socket %s \n", argv[1]);
		
	while(1)
	{	
		//Accept connection from the client
		int acpt = accept(sockfd, (struct sockaddr*)&client, (socklen_t*)&clientAddrLen);
		if (acpt < 0)
			exit(1);

		//Receive message from client
		char reqBuff[2048] = "";
		char URL[256] = "";
		char host[128] = "";
		int bufferRealSize = 0;

		recv(acpt, reqBuff, sizeof(reqBuff), 0);

		doParse(reqBuff, )

		char localMsg[256] = "REQ ";
		strcat(localMsg, URL);
		printf("%s \n", localMsg);

		//Close connection
		close(acpt);
	}
	
	close(sockfd);
	return 0;
}
