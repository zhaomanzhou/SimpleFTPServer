#include <sys/types.h>
#include <sys/stat.h>
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
#include <fcntl.h>

#include "message.h"
#include "server.h"

#define BUFF_LEN 1024

using namespace std;

Server_State_T server_state;
string cmd_string[] = {" ", "CMD_LS", "CMD_SEND","CMD_GET","CMD_REMOVE", "CMD_SHUTDOWN"};
int server_fd;
struct sockaddr_in clent_addr;






int main(int argc, char *argv[])
{
    unsigned short udp_port = 0;
	if ((argc != 1) && (argc != 3))
	{
		cout << "Usage: " << argv[0];
		cout << " [-p <udp_port>]" << endl;
		return 1;
	}
	else
	{
		//system("clear");
		//process input arguments
		for (int i = 1; i < argc; i++)
		{				
			if (strcmp(argv[i], "-p") == 0)
				udp_port = (unsigned short) atoi(argv[++i]);
		    else
		    {
		        cout << "Usage: " << argv[0];
		        cout << " [-p <udp_port>]" << endl;
		        return 1;
		    }
		}
	}


	checkDirectory("./backup/");



    struct sockaddr_in ser_addr;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        printf("- failed to create/bind TCP/UDP socket\n");
        return -1;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(udp_port);

    int ret = bind(server_fd, (struct sockaddr *) &ser_addr, sizeof(ser_addr));
    if (ret < 0) {
        printf("- failed to create/bind TCP/UDP socket\n");
        return -1;
    }


    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    getsockname(server_fd, (struct sockaddr *)&sin, &len);



	server_state = WAITING;
    Cmd_Msg_T cmd;

    while(true)
    {
        usleep(100);

        switch(server_state)
        {
            case WAITING:
            {
                cout<<"Waiting UDP command @: "<<ntohs(sin.sin_port)<<endl;
                handleWaiting(cmd);
                break;
            }
            case PROCESS_LS:
            {
                handleLs();
                server_state = WAITING;
                break;
            }
            case PROCESS_SEND:
            {
                handleSend(cmd);
                server_state = WAITING;
                break;
            }
            case PROCESS_REMOVE:
            {
                handleRemove(cmd);
		        server_state = WAITING;
                break;
            }
            case SHUTDOWN:
            {
                handleShutdown();

                close(server_fd);
                return 0;
            }
            default:
            {
           		server_state = WAITING;
                break;
            }
        }
    }
    return 0;
}

void handleShutdown()
{
    Cmd_Msg_T cmd;
    cmd.cmd = CMD_ACK;
    cmd.error = 0;
    sendCmd(cmd);
    cout<<" - send acknowledgemet."<<endl;
}

void handleRemove(Cmd_Msg_T &cmd)
{
    char destfile[120];
    memset(destfile, '\0', 120);
    strcpy(destfile, "./backup/");
    strcat(destfile, cmd.filename);
    if(checkFile(destfile))
    {
        if(remove(destfile) == 0)
            cout<<" - "<<destfile<<" has been removed."<<endl;
        cmd.error = 0;
        sendCmd(cmd);

    } else
    {
        cout<<" - file doesn't exist."<<endl;
        cmd.error = 1;
        sendCmd(cmd);
    }

    cout<<" - send acknowledgemet."<<endl;
}

void handleSend(Cmd_Msg_T& cmd)
{

    cout<<" - filename: "<<cmd.filename<<endl;
    cout<<" - filesize: "<<cmd.size<<endl;
    char destfile[120];
    memset(destfile, '\0', 120);
    strcpy(destfile, "./backup/");
    strcat(destfile, cmd.filename);
    if(checkFile(destfile))
    {
        cmd.error = 2;
        cout<<" - file "<<destfile<<" exist; overwrite?"<<endl;
        sendCmd(cmd);
        usleep(100);
        receiveCmd(cmd);
        if(cmd.error == 2)
        {
            cout<<" - do not overwrite."<<endl;
            return;
        } else
        {
            cout<<" - overwrite the file"<<endl;
        }
    } else
    {
        cmd.error = 0;
        sendCmd(cmd);
    }

    int file_fd = open(destfile, O_WRONLY | O_CREAT, 0755);
    if(file_fd < 0)
    {
        cout<<" - cannot create file: "<<destfile<<endl;
        return;
    }

    // start tcp server
    int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_fd < 0)
    {
        printf(" - failed to create/bind TCP/UDP socket\n");
        return;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr =  htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    int ret = listen(listen_fd, 1);
    if(ret < 0)
    {
        printf(" - failed to create/bind TCP/UDP socket\n");
        return;
    }
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    getsockname(listen_fd, (struct sockaddr *)&sin, &len);

    uint16_t tcp_port = ntohs(sin.sin_port);
    cmd.port = tcp_port;
    sendCmd(cmd);
    cout<<"- listen @: "<<cmd.port<<endl;

    int connect_fd;
    if( (connect_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL)) == -1){
        printf(" - failed to create/bind TCP/UDP socket\n");
        return;
    }

    cout<<" - connected with client."<<endl;


    ssize_t n = -1;
    ssize_t total = 0;
    char buff[3000];
    while ((n = read(connect_fd, buff, 3000)) > 0)
    {
        total += n;
        cout<<" - total bytes received:"<<total<<endl;
        write(file_fd, buff, n);
    }

    close(connect_fd);
    close(file_fd);
    close(listen_fd);
    cout<<" - "<<cmd.filename<<" has been received."<<endl;
    cmd.cmd = CMD_ACK;
    cmd.error = 0;

    sendCmd(cmd);
    cout<<" - send acknowledgemet."<<endl;

    
}

void handleLs()
{
    Cmd_Msg_T cmd;
    cmd.cmd = CMD_LS;
    cmd.error = 0;
    vector<string> files;
    getDirectory("./backup/", files);
    cmd.size= static_cast<uint32_t>(files.size());
    if(cmd.size == 0)
        cout<<" - server backup folder is empty."<<endl;
    sendCmd(cmd);
    for(int i = 0; i < files.size(); i++)
    {
        strncpy(cmd.filename, files[i].c_str(), sizeof(cmd.filename)/ sizeof(char));
        cout<<" - "<<files[i]<<endl;
        sendCmd(cmd);
    }
}


void sendCmd(Cmd_Msg_T& cmd)
{
    socklen_t len;
    len = sizeof(clent_addr);
    sendto(server_fd, (char*)&cmd, sizeof(cmd) + 1, 0, (struct sockaddr*)&clent_addr, len);
}



void receiveCmd(Cmd_Msg_T& cmd)
{
    char buf[BUFF_LEN];  //接收缓冲区，1024字节
    socklen_t len;
    ssize_t count;
    memset(buf, 0, BUFF_LEN);
    len = sizeof(clent_addr);
    count = recvfrom(server_fd, buf, BUFF_LEN, 0, (struct sockaddr *) &clent_addr, &len);  //recvfrom是拥塞函数，没有数据就一直拥塞
    if (count == -1) {
        printf("receive data fail!\n");
        return;
    }
    memcpy(&cmd, buf, sizeof(cmd)+1);
}

void handleWaiting(Cmd_Msg_T& cmd)
{


    receiveCmd(cmd);

    cout<<"[CMD RECEIVED]: "<<cmd_string[cmd.cmd]<<endl;

    switch (cmd.cmd)
    {
        case CMD_LS: server_state = PROCESS_LS;
            break;
        case CMD_SEND: server_state = PROCESS_SEND;
            break;
        case CMD_REMOVE: server_state = PROCESS_REMOVE;
            break;
        case CMD_SHUTDOWN: server_state = SHUTDOWN;
            break;
    }


}

//this function check if the backup folder exist
int checkDirectory (string dir)
{
	DIR *dp;
	if((dp  = opendir(dir.c_str())) == NULL) {
        //cout << " - error(" << errno << ") opening " << dir << endl;
        if(mkdir(dir.c_str(), S_IRWXU) == 0)
            cout<< " - Note: Folder "<<dir<<" does not exist. Created."<<endl;
        else
            cout<< " - Note: Folder "<<dir<<" does not exist. Cannot created."<<endl;
        return errno;
    }
    closedir(dp);

    return 0;
}


//this function is used to get all the filenames from the
//backup directory
int getDirectory (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        //cout << " - error(" << errno << ") opening " << dir << endl;
        if(mkdir(dir.c_str(), S_IRWXU) == 0)
            cout<< " - Note: Folder "<<dir<<" does not exist. Created."<<endl;
        else
            cout<< " - Note: Folder "<<dir<<" does not exist. Cannot created."<<endl;
        return errno;
    }

    int j=0;
    while ((dirp = readdir(dp)) != NULL) {
    	//do not list the file "." and ".."
        if((string(dirp->d_name)!=".") && (string(dirp->d_name)!=".."))
        	files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}
//this function check if the file exists
bool checkFile(const char *fileName)
{
    ifstream infile(fileName);
    return infile.good();
}

