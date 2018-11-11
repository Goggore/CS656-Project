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
void doParse(char* req, char* hostName, char* URL, int* buffSize)
{
	int getHostStart = -1, getURL = -1, getURLStart = -1;

	//Get real size of data
	while (1)
	{
		if (req[buffSize[0]] == '\0')
			break;

		//GetURL
		if (getURLStart > 0)
		{
			if (req[buffSize[0]] != ' ')
			{
				strncat(URL, &req[buffSize[0]], sizeof(char));
				buffSize[2]++;
			}
			else
			{
				URL[buffSize[2]] = '\0';
				getURL = 1;
				getURLStart = -1;
			}
		}

		if (getURL < 0 && getURLStart < 0 && req[buffSize[0]] == ' ')
			getURLStart = 1;

		//Get Host
		if (buffSize[0] > 5 &&
			req[buffSize[0] - 6] == 'H' && req[buffSize[0] - 5] == 'o' && req[buffSize[0] - 4] == 's' &&
			req[buffSize[0] - 3] == 't' && req[buffSize[0] - 2] == ':' && req[buffSize[0] - 1] == ' ')
			getHostStart = 1;

		if (getHostStart > 0)
		{
			if (req[buffSize[0]] == '\r' || req[buffSize[0]] == ':')
				getHostStart = -1;
			else
			{
				strncat(hostName, &req[buffSize[0]], sizeof(char));
				buffSize[1]++;
			}
		}

		buffSize[0]++;
	}

	return;
}

//Transfer data
void doHTTP()
{

}
/**
char* connectWebSever(sockaddr_in *ip, char *addr)
{
	//struct sockaddr_in net;

	//net.sin_family = AF_INET;
    //net.sin_port = htons( 80 );
    //net.sin_addr.s_addr = inet_addr(addr);
	//create socket to web server
	if ((wsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		exit(1);
	}
	if (connect(wsock , (struct sockaddr *)&ip , sizeof(ip)) < 0)
    {
    	exit(1);
    }
    if( send(wsock, addr, strlen(addr), 0) < 0)
    {
    	exit(1);
    }

    char[65536] Buf;
    if( recv(sock , Buf , 65536 , 0) < 0)
    {
    	exit(1);
    }    
    if(Buf[65535] == NULL)
    {
    	exit(1);
    }
return Buf;
}
/**/

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
		int bufferSize[3] = {0,0,0};//reqSize, hostSize, urlSize

		recv(acpt, reqBuff, sizeof(reqBuff), 0);

		doParse(reqBuff, host, URL, bufferSize);

		char localMsg[256] = "REQ ";
		strcat(localMsg, URL);
		printf("%s \n", localMsg);

		char* realHost = (char*)malloc(sizeof(char) * bufferSize[1]);
		memcpy(realHost, host, sizeof(char) * bufferSize[1]);
		struct sockaddr_in pIP = Dns(acpt, realHost);

		//Close connection
		close(acpt);
	}
	
	close(sockfd);
	return 0;
}
