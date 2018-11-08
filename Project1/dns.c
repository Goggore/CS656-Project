/* Comment block
   CS656-005 Group W6 Su (ss3889), Chih-Yun (cc785), Shang-Yung (sh555), Ge (gg345), Quzhi (ql247)
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
    {
        //perror("Create Socket");
        return -1;
    }
	
	//Connect
	long long startT = GetTime();
	if (connect(sockfd, info.ai_addr, info.ai_addrlen) < 0)
	{
		//perror("Connect");
		return -1;
	}
	int delay = GetTime() - startT;
	close(sockfd);
	
	return delay;
}

//Get IP addresses 
void Dns(int acpt, char* addr)
{
	struct addrinfo* infos;
	struct addrinfo prefAddr;
	
	if (getaddrinfo(addr, NULL, NULL, &infos) == 0)
	{
		struct addrinfo info = infos[0];
		float minD = 10000000;
		
		//Get every node of the list, calculate delay, find the preferred address and then send to the client
		while(info.ai_next != NULL)
		{
			struct sockaddr_in *addrin;
			addrin = (struct sockaddr_in*)info.ai_addr;
			char msg[256] = "IP = ";
			strcat(msg, inet_ntoa(addrin->sin_addr));
			strcat(msg, "\n");
			if (send(acpt, msg, sizeof(msg), 0) < 0)//Send to the client
			{
				perror("Send");
				return;
			}
			
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
	else
	{
		//If no address is available, send error message to the client
		char msg[] = "NO IP ADDRESSES FOUND \n";
		if (send(acpt, msg, sizeof(msg), 0) < 0)
		{	
			perror("Send");
			return;
		}	
		return;
	}
	
	//Get preferred IP address by calculating the connection delay 
	struct sockaddr_in *addrin;
	addrin = (struct sockaddr_in*)prefAddr.ai_addr;
	char msg[256] = "PREFERRED IP = ";
	strcat(msg, inet_ntoa(addrin->sin_addr));
	strcat(msg, "\n");
	if (send(acpt, msg, sizeof(msg), 0) < 0)
	{
		perror("Send");
		return;
	}
		
	return;
}

int main(int argc, char **argv)
{
	struct sockaddr_in server, client;
	int clientAddrLen = sizeof(client);
	int connectCount = 0;
	bzero(&server, sizeof(server));
	bzero(&client, sizeof(client));

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[1]));
	server.sin_addr.s_addr = inet_addr("0.0.0.0");
	
	//Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("Create Socket");
		return 0;
	}
	//Bind socket to address and port
	if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0)
	{
		perror("Bind");
		return 0;
	}
	//Begin to listen to specified port
	if (listen(sockfd,3) < 0)
	{
		perror("Listen");
		return 0;
	}
	printf("DNS Server Listening on Socket %s \n", argv[1]);
		
	while(1)
	{	
		//Accept connection from the client
		int acpt = accept(sockfd, (struct sockaddr*)&client, (socklen_t*)&clientAddrLen);
		if (acpt < 0)
		{
			perror("Accept");
			return 0;
		}
		else
			connectCount++;
		printf ("(%d) Incoming client connection from [%s:%d] to me [%s:%d] \n", connectCount, inet_ntoa(client.sin_addr), htons(client.sin_port), inet_ntoa(server.sin_addr), htons(server.sin_port));
	
		//Wait for the message from the client and send specified information
		while(1)	
		{
			//Receive message from client
			char buffer[512] = "";
			int bufferRealSize = 0;
			
			recv(acpt, buffer, sizeof(buffer), 0);
			
			//Get real size of data
			for (bufferRealSize; bufferRealSize < sizeof(buffer); bufferRealSize++)
			{
				if (buffer[bufferRealSize] == '\n')
					break;
			}
			
			if (bufferRealSize > 0)
			{
				//Get the message without null char
				char* realBuffer;
				realBuffer = (char*)malloc(sizeof(char) * bufferRealSize);
				int i = 0;
				for (i; i < bufferRealSize; i++)
					realBuffer[i] = buffer[i];
				
				realBuffer[bufferRealSize - 1] = '\0';
				
				char msg[256] = "REQ ";
				strcat(msg, realBuffer);
				strcat(msg, "\n");
				printf("%s \n", msg);
				if (send(acpt, msg, sizeof(msg), 0) < 0)
				{
					perror("Send");
					break;
				}
				
				Dns(acpt, realBuffer);
				
				free(realBuffer);
				
				//Close connection
				close(acpt);
				break;
			}
		}
	}
	
	return 0;
}
