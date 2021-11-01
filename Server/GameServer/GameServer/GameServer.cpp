// GameServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>
#include <bitset>
#include "../../../Network.h"
#include "Player.h"

using namespace std::chrono;

using frame = duration<int32_t, std::ratio<1, 60>>;
using ms = duration<float, std::milli>;

bool gameLoopRunning = true;
bool recvLoopRunning = true;

WSASession Session;
UDPSocket Socket;
char buffer[500];
char sendbuffer[500];
bool newData;

std::vector<Player> players;




//Receives all client communications
void RecvFromLoop()
{
    while (recvLoopRunning)
    {
        while (!newData)
        {
            try
            {
                sockaddr_in sender = Socket.RecvFrom(&buffer[0], 500);
                newData = true;
                
                //Can we handle this data ourselves?
                unsigned int* msgType = (unsigned int*)&buffer;
                
                //Connection Request
                if (*msgType == 1)
                {
                    //Respond with 1 to accept, followed by a player ID

                    int newID = players.size();
                    Player newPlayer = Player(sender, newID);
                    players.push_back(newPlayer);

                    std::fill_n(sendbuffer, 500, 0);

                    unsigned int data = 1;
                    std::memcpy(&sendbuffer, &data, 4);
                    data = newPlayer.GetID();
                    std::memcpy(&sendbuffer + 4, &data, 4);

                    for (size_t i = 0; i < 8; i++)
                    {
                        std::bitset<8> x(sendbuffer[i]);
                        std::cout << x << " ";
                    }

                    Socket.SendTo(sender, sendbuffer, 500);

                    std::cout << "Player " << newPlayer.GetID() << " joined.\n";

                    //Clear buffer
                    std::fill_n(buffer, 500, 0);
                    newData = false;

                }

                //Socket.SendTo(sender, "awrfawfr", 8);
            }
            catch (std::exception& ex)
            {
                std::cout << ex.what() << std::endl;
            }
        }
    }
}

//Updates at 60 frames
void GameLoop()
{
    time_point<steady_clock> fpsTimer(steady_clock::now());
    frame FPS{};

    while (gameLoopRunning)
    {
        FPS = duration_cast<frame>(steady_clock::now() - fpsTimer);
        if (FPS.count() >= 1)
        {
            fpsTimer = steady_clock::now();
            //std::cout << "LastFrame: " << duration_cast<ms>(FPS).count() << "ms  |  FPS: " << FPS.count() * 60 << std::endl;
        }
    }
}

int main()
{
    std::string IP = "127.0.0.1";
    int PORT = 8888;

    try
    {
        Socket.Bind(PORT);

        std::thread gameLoop = std::thread(&GameLoop);
        std::thread recvLoop = std::thread(&RecvFromLoop);

        std::cout << "Server online. Port number " << PORT << std::endl;

        gameLoop.join();

        recvLoop.join();

    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

