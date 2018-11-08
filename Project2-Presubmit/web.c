/* Comment block
?? CS656-005 Group W6 Su (ss3889), Chih-Yun (cc785), Shang-Yung (sh555), Ge (gg345), Quzhi (ql247)
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
	printf("Proxy Server Listening on Socket %s \n", argv[1]);
		
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
			
		printf("(%d) ", connectCount);
			
		//Wait for the message from the client and send specified information
		while(1)	
		{
			//Receive message from client
			char buffer[2048] = "";
			int bufferRealSize = 0;
			
			recv(acpt, buffer, sizeof(buffer), 0);
			
			//Get real size of data
			for (bufferRealSize; bufferRealSize < sizeof(buffer); bufferRealSize++)
				if (buffer[bufferRealSize] == '\0')
					break;
			
			//Get URL from request message
			char URL[256] = "";
			int i = 0;
			for (i; i < bufferRealSize; i++)
			{
				if (buffer[i] == ' ')
				{
					i++;
					int j = 0;
					while(buffer[i] != ' ')
					{
						URL[j] = buffer[i];
						j++;
						i++;
					}
					URL[j + 1] = '\0';
					break;
				}
			}

			char localMsg[256] = "REQ ";
			strcat(localMsg, URL);
			printf("%s \n", localMsg);
			
			if (bufferRealSize > 0)
			{
				//Get the message without null char
				char* realBuffer;
				realBuffer = (char*)malloc(sizeof(char) * bufferRealSize);
				int i = 0;
				for (i; i < bufferRealSize; i++)
					realBuffer[i] = buffer[i];
				
				realBuffer[bufferRealSize - 1] = '\0';
				
				//Build HTTP response message header
				char clientMsg[8192] = "HTTP/1.1 200 OK \r\n";
				time_t currentT;
				time(&currentT);
				strcat(clientMsg, "Date: ");
				strcat(clientMsg, asctime(gmtime(&currentT)));
				strcat(clientMsg, "GMT \r\n");
				strcat(clientMsg, "Content-Type: text/html;charset=ISO-8859-1 \r\n");
				strcat(clientMsg, "Content-Length: 2048 \r\n");
				strcat(clientMsg, "\r\n");
				
				//Build HTTP response message data
				strcat(clientMsg, "<html>");
				strcat(clientMsg, "<head><title>CS656-005 W6 NW2 Pre-submit</title><head>");
				strcat(clientMsg, "<body><p>");
				
				//Get host name and add <br> at the end of every line
				i = 0;
				int hostStart = 0, hostEnd = 0;
				int getHostStart = -1;
				for (i; i < bufferRealSize; i++)
				{	
					if (realBuffer[i] == '\n')
						strcat(clientMsg, "<br>");
					else
						strncat(clientMsg, &realBuffer[i], sizeof(char));

					if (i > 5 && 
						realBuffer[i-6] == 'H' && 
						realBuffer[i-5] == 'o' && 
						realBuffer[i-4] == 's' && 
						realBuffer[i-3] == 't' &&
						realBuffer[i-2] == ':' &&
						realBuffer[i-1] == ' ')
					{
						getHostStart = 1;
						hostStart = i;
					}
					
					if (getHostStart > 0 && realBuffer[i] == '\n')
					{
						getHostStart = -1;
						hostEnd = i - 2;
					}
					else if (getHostStart > 0 && realBuffer[i] == ':')
					{
						getHostStart = -1;
						hostEnd = i - 1;
					}

				}

				//Add hostip to message
				strcat(clientMsg, "HOSTIP = ");
				char* hostName;
				hostName = (char*)malloc(sizeof(char) * (hostEnd - hostStart + 1));
				i = hostStart;
				int j = 0;
				for (i; i <= hostEnd; i++)
				{
					strncat(clientMsg, &realBuffer[i], sizeof(char));
					hostName[j] = realBuffer[i];
					j++;
				}
				hostName[hostEnd - hostStart + 1] = '\0';

				struct sockaddr_in* pIP = Dns(acpt, hostName);
				strcat(clientMsg, " (");
				strcat(clientMsg, inet_ntoa(pIP->sin_addr));
				strcat(clientMsg, ")");
				strcat(clientMsg, "<br>");

				//add hostport to message
				strcat(clientMsg, "PORT = ");
				char portNo[5] = "0";
				if (URL[0] == 'h' &&
					URL[1] == 't' &&
					URL[2] == 't' &&
					URL[3] == 'p')
				{
					portNo[0] = '8';
					portNo[1] = '0';
				}
				else if(URL[0] == 'h' &&
						URL[1] == 't' &&
						URL[2] == 't' &&
						URL[3] == 'p' && 
						URL[3] == 's')
				{
					portNo[0] = '4';
					portNo[1] = '4';
					portNo[2] = '3';
				}

				strcat(clientMsg, portNo);
				strcat(clientMsg, "<br>");

				//add path to message
				strcat(clientMsg, "PATH = ");
				i = 0;
				int count = 0;
				for (i; i < sizeof(localMsg); i++)
				{
					if (localMsg[i] == '/')
						count++;

					if (count == 3)
					{
						strcat(clientMsg, &localMsg[i+1]);
						break;
					}
				}
				strcat(clientMsg, "<br>");
				
				strcat(clientMsg, "</p></body>");
				strcat(clientMsg, "</html>");
				
				//Send message back to browser
				if (send(acpt, clientMsg, sizeof(clientMsg), 0) < 0)
				{
					perror("Send");
					break;
				}
				
				free(realBuffer);
				free(hostName);
				
				//Close connection
				close(acpt);
				break;
			}
		}
	}
	
	close(sockfd);
	return 0;
}
