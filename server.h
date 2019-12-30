#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <string>

using namespace std;

typedef enum SERVER_STATE_tag
{
    WAITING = 0,
    PROCESS_LS = 1,
    PROCESS_SEND = 2,
    PROCESS_GET = 3,
    PROCESS_REMOVE = 4,
    SHUTDOWN = 5
}Server_State_T;

bool checkFile(const char *fileName);
int checkDirectory (string dir);
int getDirectory (string dir, vector<string> &files);

void receiveCmd(Cmd_Msg_T& cmd);
void sendCmd(Cmd_Msg_T& cmd);
void handleWaiting(Cmd_Msg_T& cmd);

void handleLs();

void handleSend(Cmd_Msg_T& tag);


void handleRemove(Cmd_Msg_T &cmd);

void handleShutdown();

#endif
