/* Comment block
   CS656-005 Group W6 
   Su (ss3889), Chih-Yun (cc785), Shang-Yung (sh555), Ge (gg345), Quzhi (ql247)
*/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

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
        exit(1);
	
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
	int i = 0;

	//Get real size of data
	while (1)
	{
		if (req[i] == '\0')
			break;

		//GetURL
		if (getURLStart > 0)
		{
			if (req[i] != ' ')
			{
				strncat(URL, &req[i], sizeof(char));
				buffSize[2]++;
			}
			else
			{
				URL[buffSize[2]] = '\0';
				getURL = 1;
				getURLStart = -1;
			}
		}

		if (getURL < 0 && getURLStart < 0 && req[i] == ' ')
			getURLStart = 1;

		//Get Host
		if (i > 5 &&
			req[i - 6] == 'H' && req[i - 5] == 'o' && req[i - 4] == 's' &&
			req[i - 3] == 't' && req[i - 2] == ':' && req[i - 1] == ' ')
			getHostStart = 1;

		if (getHostStart > 0)
		{
			if (req[i] == '\r' || req[i] == ':')
				getHostStart = -1;
			else
			{
				strncat(hostName, &req[i], sizeof(char));
				buffSize[1]++;
			}
		}

		i++;
	}

	return;
}

//Transfer data
void doHTTP(struct sockaddr_in brw, struct sockaddr_in pServ, struct sockaddr_in wServ, int accept, char* req, int reqSize)
{
	char tempBuff[20480];
	
	//Create sock for connection with web server
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		exit(1);

	printf("[5]\n");

	//Connect with web server
	struct sockaddr* temp = (struct sockaddr*)&wServ;
	if (connect(sockfd, temp, sizeof(*temp)) < 0)
	{
		close(sockfd);
		exit(1);
	}

	printf("[6]\n");

	//Send request message to web server
	if (send(sockfd, req, reqSize, 0) < 0)
		exit(1);

	printf("[7]\n");

	//Receive respone message from web server
	int respSize = recv(sockfd, tempBuff, sizeof(tempBuff), 0);
	char* respBuff = (char*)malloc(sizeof(char) * respSize);
	memcpy(respBuff, tempBuff, respSize);

	//printf(respBuff);

	//Send respone message back to browser
	if (send(accept, respBuff, respSize, 0) < 0)
		exit(1);

	printf("[8]\n");

	free(respBuff);
}

int main(int argc, char **argv)
{
	//Open and import block list
	char blkBuff[3072];
	int blkFileDes = open(argv[2], O_RDONLY);
	int blkFileSize = read(blkFileDes, blkBuff, sizeof(blkBuff));
	
	//Parse block list

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

	printf("[1]\n");

	//Bind socket to address and port
	if (bind(sockfd, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) < 0)
		exit(1);

	printf("[2]\n");

	//Begin to listen to specified port
	if (listen(sockfd,3) < 0)
		exit(1);

	printf("[3]\n");

	printf("stage 2 program by ss3889 listening on socket %s \n", argv[1]);
		
	while(1)
	{	
		//Accept connection from the client
		int acpt = accept(sockfd, (struct sockaddr*)&client, (socklen_t*)&clientAddrLen);
		if (acpt < 0)
			exit(1);

		printf("[4]\n");

		//Init buffers
		char reqBuff[2048] = "";
		char URL[256] = "";
		char host[128] = "";
		int bufferSize[3] = {0,0,0};//reqSize, hostSize, urlSize

		//Receive message from client
		bufferSize[0] = recv(acpt, reqBuff, sizeof(reqBuff), 0);

		printf("[12]\n");

		//Parse request
		doParse(reqBuff, host, URL, bufferSize);

		printf("[13]\n");

		char localMsg[512] = "REQUEST: ";
		strcat(localMsg, URL);
		printf("%s \n", localMsg);

		//Close connect if the size of request is larger than 65535 bytes
		if (bufferSize[0] > 65535)
		{
			close(acpt);
			continue;
		}

		//Build buffer with real size
		char* realHost = (char*)malloc(sizeof(char) * bufferSize[1]);
		char* realReq = (char*)malloc(sizeof(char) * bufferSize[0]);
		memcpy(realHost, host, sizeof(char) * bufferSize[1]);
		memcpy(realReq, reqBuff, sizeof(char) * bufferSize[0]);

		//Check block
		int i = 0, match = -1, end = -1;
		while (end < 0)
		{
			if (strncmp(&blkBuff[i], realHost, bufferSize[1]) == 0)
			{
				match = 1;
				end = 1;
				break;
			}
			else
			{
				for (i; i < blkFileSize; i++)
				{
					if (blkBuff[i] == '\n')
					{
						i++;
						break;
					}
					if (i == blkFileSize - 1)
					{
						end = 1;
						break;
					}
				}
			}

			printf("[11]\n");
		}

		printf("[10]\n");

		if (match > 0)
		{
			char blkMsg[256] = "HTTP/1.1 403 Forbidden\r\n";
			time_t currentT;
			time(&currentT);
			strcat(blkMsg, "Date: ");
			strcat(blkMsg, asctime(gmtime(&currentT)));
			strcat(blkMsg, "GMT \r\n");
			strcat(blkMsg, "Content-Type: text/html;charset=ISO-8859-1 \r\n");
			strcat(blkMsg, "Content-Length: 0 \r\n");
			strcat(blkMsg, "\r\n");

			if (send(acpt, blkMsg, sizeof(blkMsg), 0) < 0)
				exit(1);

			printf("[9]\n");
		}
		else
		{
			//Get prefered IP
			struct sockaddr_in* desIP = Dns(acpt, realHost);
			desIP->sin_port = htons(80);

			//Transform data
			doHTTP(client, server, *desIP, acpt, realReq, bufferSize[0]);
		}

		//Close connection
		close(acpt);
		free(realHost);
		free(realReq);
	}
	
	close(sockfd);
	return 0;
}
