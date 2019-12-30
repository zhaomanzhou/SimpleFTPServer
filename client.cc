#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <sstream>
#include <string>
#include <netdb.h>
#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>

#include "message.h"
#include "client.h"

#define BUFF_LEN 1024
using namespace std;


struct sockaddr_in dst;
struct sockaddr_in src;



int client_fd;


const char* server_host = "127.0.0.1";
int main(int argc, char *argv[])
{
    unsigned short udp_port = 0;

    //process input arguments
	if ((argc != 3) && (argc != 5))
	{
		cout << "Usage: " << argv[0];
		cout << " [-s <server_host>] -p <udp_port>" << endl;
		return 1;
	}
	else
	{
		//system("clear");
		for (int i = 1; i < argc; i++)
		{				
			if (strcmp(argv[i], "-p") == 0)
				udp_port = (unsigned short) atoi(argv[++i]);
			else if (strcmp(argv[i], "-s") == 0)
			{
				server_host = argv[++i];
				if (argc == 3)
				{
				    cout << "Usage: " << argv[0];
		            cout << " [-s <server_host>] -p <udp_port>" << endl;
		            return 1;
				}
		    }
	        else
	        {
	            cout << "Usage: " << argv[0];
		        cout << " [-s <server_host>] -p <udp_port>" << endl;
		        return 1;
	        }
		}
	}
	
	
	
	Client_State_T client_state = WAITING;
	string in_cmd;






    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }


	memset(&dst, 0, sizeof(dst));
	dst.sin_family = AF_INET;
	dst.sin_addr.s_addr = inet_addr(server_host);
	dst.sin_port = htons(udp_port);


	while(true)
	{
	    usleep(100);
	    switch(client_state)
	    {
	        case WAITING:
	        {
	            cout<<"$ ";
	            cin>>in_cmd;
	            
	            if(in_cmd == "ls")
	            {
	                client_state = PROCESS_LS;
	            }
	            else if(in_cmd == "send")
	            {
	                client_state = PROCESS_SEND;
	            }
	            else if(in_cmd == "remove")
	            {
	                client_state = PROCESS_REMOVE;
	            }
	            else if(in_cmd == "shutdown")
	            {
	                client_state = SHUTDOWN;
	            }
	            else if(in_cmd == "quit")
	            {
	                client_state = QUIT;
	            }
	            else
	            {
	                cout<<" - wrong command."<<endl;
	                client_state = WAITING;
	            }
	            break;
	        }
	        case PROCESS_LS:
	        {
				handle_ls();
		        client_state = WAITING;
	            break;
	        }
	        case PROCESS_SEND:
	        {
	        	char filename[126];
	        	scanf("%s", filename);
	            handle_send(filename);
		        client_state = WAITING;
		        break;
	        }
	        case PROCESS_REMOVE:
	        {
				char filename[126];
				scanf("%s", filename);
				handle_remove(filename);
	            client_state = WAITING;
	            break;
	        }	
	        case SHUTDOWN:
	        {
	        	handle_shutdown();
	        	client_state = WAITING;
	            break;
	        }
	        case QUIT:
	        {
	        	close(client_fd);
				return 0;
	        }
	        default:
	        {
	        	client_state = WAITING;
	            break;
	        }    
	    }
	}
    return 0;
}

void handle_shutdown()
{
	Cmd_Msg_T cmd;
	cmd.cmd = CMD_SHUTDOWN;
	sendCmd(cmd);
	receiveCmd(cmd);
	if(cmd.error == 0)
	{
		cout<<" - server is shutdown."<<endl;
	}
}

void handle_remove(char filename[])
{
	Cmd_Msg_T cmd;
	cmd.cmd = CMD_REMOVE;
	strncpy(cmd.filename, filename, sizeof(cmd.filename)/ sizeof(char));
	cmd.error = 0;
	sendCmd(cmd);
	receiveCmd(cmd);
	if(cmd.error == 0)
	{
		cout<<" - file is removed."<<endl;
	} else
	{
		cout<<" - file doesn't exist."<<endl;
	}
}

void handle_send(char filename[])
{
	int file_fd = open(filename, O_RDONLY);
	if(file_fd < 0)
	{
		cout<<" - cannot open file:"<<filename<<endl;
		return;
	}

	Cmd_Msg_T cmd;
	cmd.cmd = CMD_SEND;
	strncpy(cmd.filename, filename, sizeof(cmd.filename)/ sizeof(char));
	cmd.error = 0;
	cmd.size = static_cast<uint32_t>(get_file_size(filename));
	cout<<" - filesize:"<<cmd.size<<endl;


	sendCmd(cmd);
	receiveCmd(cmd);
	if(cmd.error == 2)
	{
		cout<<"file exists. overwrite? (y/n):";
		string choice;
		cin>>choice;
		if(choice == "y" || choice == "Y")
		{
			cmd.error = 0;
			sendCmd(cmd);
		} else
		{
			sendCmd(cmd);
			return;
		}
	}
	receiveCmd(cmd);
	cout<<" - TCP port:"<<cmd.port<<endl;


	//connect

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		printf("- failed to create/bind TCP/UDP socket\n");
		return;
	}
	struct sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_host);
	serv_addr.sin_port = htons(cmd.port);
	int res = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(res < 0)
	{
		printf("- failed to create/bind TCP/UDP socket\n");
		return;
	}

	char buff[3000];
	ssize_t readed = -1;
	while ((readed = read(file_fd, buff, 3000)) > 0)
	{
		write(sock, buff,readed);
	}
	close(file_fd);
	close(sock);

	receiveCmd(cmd);
	if(cmd.cmd == CMD_ACK && cmd.error == 0)
	{
		cout<<"- file transmission is completed."<<endl;
	}

}



void receiveCmd(Cmd_Msg_T& cmd)
{
	char buf[BUFF_LEN];
	socklen_t len;
	ssize_t count;
	memset(buf, 0, BUFF_LEN);
	len = sizeof(src);
	count = recvfrom(client_fd, buf, BUFF_LEN, 0, (struct sockaddr *) &src, &len);
	if (count == -1) {
		printf("recieve data fail!\n");
		return;
	}
	memcpy(&cmd, buf, sizeof(cmd)+1);
}


void sendCmd(Cmd_Msg_T& cmd)
{
	socklen_t len;
	len = sizeof(dst);
	sendto(client_fd, (char*)&cmd, sizeof(cmd) + 1, 0, (struct sockaddr*)&dst, len);
}

void handle_ls()
{

    Cmd_Msg_T cmd;
    cmd.cmd = CMD_LS;
    cmd.error = 0;
    sendCmd(cmd);
    receiveCmd(cmd);
    if(cmd.cmd != CMD_LS)
	{
    	cout<<"- command response error."<<endl;
		return;
	}
	uint32_t size = cmd.size;
	if(size == 0)
	{
		cout<<" - server backup folder is empty."<<endl;
	} else
	{
		for(uint32_t i = 0; i < size; i++)
		{
			receiveCmd(cmd);
			if(cmd.cmd != CMD_LS)
			{
				cout<<"- command response error."<<endl;
				return;
			}
			cout<<" - "<<cmd.filename<<endl;
		}
	}

}


off_t get_file_size(char* filename)
{
	struct stat sstat;
	if(stat(filename, &sstat)<0)
	{
		cout<<"failed"<<endl;
	}
	off_t size=sstat.st_size;

	return size;
}