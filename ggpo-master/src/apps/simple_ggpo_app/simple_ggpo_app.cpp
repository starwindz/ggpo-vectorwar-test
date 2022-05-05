#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "ggponet.h"

#define word            int16_t
#define FRAME_DELAY     2

struct IPInfo {
    char ip[32];
    int port;
};
IPInfo g_RemoteLoc, g_LocalLoc;

class MPGame {
public:
    int p2pMode = 0;

    GGPOSession* ggpo;
    GGPOSessionCallbacks cb;
    GGPOErrorCode result;

    unsigned short localport;
    int num_players = 2;
    GGPOPlayer players[2];
    GGPOPlayerHandle local_player_handle;

public:
    void init();
    void close();
    void idle();
    bool syncInputs();
    void advanceFrame();
    void renderFrame();

    void setInputs(word* inputs);
    word getLocalInput();
};
MPGame mg;

void logInfo(const char* s)
{
    printf("%s\n", s);
}

bool __cdecl begin_game_callback(const char*)
{
    return true;
}

bool __cdecl advance_frame_callback(int)
{
    int disconnect_flags;
    word inputs[2] = { 0, };

    ggpo_synchronize_input(mg.ggpo, (void*)inputs, sizeof(word) * 2, &disconnect_flags);
    mg.setInputs(inputs);
    mg.advanceFrame();

    return true;
}

bool __cdecl load_game_state_callback(unsigned char* buffer, int len)
{
    /*
    memcpy(gs, buffer, GS_LEN);
    */
    return true;
}

bool __cdecl save_game_state_callback(unsigned char** buffer, int* len, int* checksum, int)
{
    /*
    *len = sizeof(gs);
    *buffer = (unsigned char*)malloc(*len);
    if (!*buffer) {
        return false;
    }
    memcpy(*buffer, &GS, *len);
    */
    return true;
}

void __cdecl free_buffer(void* buffer)
{
    free(buffer);
}

bool __cdecl on_event_callback(GGPOEvent* info)
{
    switch (info->code) {
    case GGPO_EVENTCODE_CONNECTED_TO_PEER:
        logInfo("GGPO_EVENTCODE_CONNECTED_TO_PEER");
        break;
    case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
        logInfo("GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER");
        break;
    case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
        logInfo("GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER");
        break;
    case GGPO_EVENTCODE_RUNNING:
        break;
    case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
        break;
    case GGPO_EVENTCODE_CONNECTION_RESUMED:
        break;
    case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
        break;
    case GGPO_EVENTCODE_TIMESYNC:
        break;
    }
    return true;
}

void MPGame::init()
{
    // init socket
    WSADATA wd = { 0 };
    WSAStartup(MAKEWORD(2, 2), &wd);

    // init callback functions and others
    cb.begin_game = begin_game_callback;
    cb.advance_frame = advance_frame_callback;
    cb.load_game_state = load_game_state_callback;
    cb.save_game_state = save_game_state_callback;
    cb.free_buffer = free_buffer;
    cb.on_event = on_event_callback;

    // start session
    if (p2pMode == 0) {
        g_LocalLoc.port = 7000;  // 37865; // 7000;
        strcpy_s(g_RemoteLoc.ip, "127.0.0.1");
        g_RemoteLoc.port = 7001; // 37866; // 7001;
    }
    else {
        strcpy_s(g_RemoteLoc.ip, "127.0.0.1");
        g_RemoteLoc.port = 7000; // 37865; // 7000;
        g_LocalLoc.port = 7001;  // 37866; // 7001;
    }
    localport = g_LocalLoc.port;

#if defined(SYNC_TEST)
    result = ggpo_start_synctest(&ggpo, &cb, "mp_game", num_players, sizeof(word), 1);
#else
    result = ggpo_start_session(&ggpo, &cb, "mp_game", num_players, sizeof(word), localport);
#endif
    ggpo_set_disconnect_timeout(ggpo, 3000);
    ggpo_set_disconnect_notify_start(ggpo, 1000);

    // sending player locations
    players[0].size = players[1].size = sizeof(GGPOPlayer);
    if (p2pMode == 0) {
        players[0].type = GGPO_PLAYERTYPE_LOCAL;
        players[0].player_num = 1;

        players[1].type = GGPO_PLAYERTYPE_REMOTE;
        strcpy_s(players[1].u.remote.ip_address, g_RemoteLoc.ip);
        players[1].u.remote.port = g_RemoteLoc.port;
        players[1].player_num = 2;
    }
    else {
        players[0].type = GGPO_PLAYERTYPE_REMOTE;
        strcpy_s(players[0].u.remote.ip_address, g_RemoteLoc.ip);
        players[0].u.remote.port = g_RemoteLoc.port;
        players[0].player_num = 1;

        players[1].type = GGPO_PLAYERTYPE_LOCAL;
        players[1].player_num = 2;
    }

    for (int i = 0; i <= num_players - 1; i++) {
        GGPOPlayerHandle handle;
        result = ggpo_add_player(ggpo, players + i, &handle);
        if (players[i].type == GGPO_PLAYERTYPE_LOCAL) {
            ggpo_set_frame_delay(ggpo, handle, FRAME_DELAY);
        }
    }
}

void MPGame::advanceFrame()
{
    //
    ggpo_advance_frame(ggpo);
}

void MPGame::renderFrame()
{
    //
}

void MPGame::close()
{
    // close ggpo
    ggpo_close_session(ggpo);

    // close socket
    WSACleanup();
}

void MPGame::idle()
{
    ggpo_idle(ggpo, 0);
}

void MPGame::setInputs(word* inputs)
{
    //
}

word MPGame::getLocalInput()
{
    word dummy = 1;
    return dummy;
}

bool MPGame::syncInputs()
{
    GGPOErrorCode result = GGPO_OK;
    int disconnect_flags;
    word inputs[2] = { 0, };
    bool ret;

    if (local_player_handle != GGPO_INVALID_HANDLE) {
        word input = mg.getLocalInput();
#if defined(SYNC_TEST)
        input = 0;
#endif
        result = ggpo_add_local_input(ggpo, local_player_handle, &input, sizeof(input));
    }

    if (GGPO_SUCCEEDED(result)) {
        result = ggpo_synchronize_input(ggpo, (void*)inputs, sizeof(word) * 2, &disconnect_flags);
        if (GGPO_SUCCEEDED(result)) {
            mg.setInputs(inputs);
            ret = true;
            return ret;
        }
    }
    ret = false;
    return ret;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("argument error. simple_ggpo_app.exe 0|1\n");
        return 1;
    }

    // 0: player1 is local,  player 2 is remote
    // 1: player1 is remote, player 2 is local  
    if (strcmp(argv[1], "0") == 0) mg.p2pMode = 0;
    else mg.p2pMode = 1;

    mg.init();
    printf("running loop...\n");
    while (1) {
        if (_kbhit())
            if (_getch() == 'q') break;
        mg.idle();
        if (mg.syncInputs()) mg.advanceFrame();
        mg.renderFrame();
    }
    mg.close();

    return 0;
}

