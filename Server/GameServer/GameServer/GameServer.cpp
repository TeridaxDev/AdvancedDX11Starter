// GameServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>
#include <bitset>
#include "Player.h"
#include "Helpers.h"
#include "../../../Network.h"

using namespace std::chrono;

#define MAX_PROJECTILES 6

using frame = duration<int32_t, std::ratio<1, 60>>;
using ms = duration<float, std::milli>;

bool gameLoopRunning = true;
bool recvLoopRunning = true;

WSASession Session;
UDPSocket Socket;
char buffer[500];
char sendbuffer[500];
bool newData;

std::vector<Player*> players;
Projectile projectiles[MAX_PROJECTILES];



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
                    //Tell every other played someone joined
                    for (size_t i = 0; i < players.size(); i++)
                    {
                        //Send a response
                        std::fill_n(sendbuffer, 500, 0);

                        unsigned int data = 2;
                        std::memcpy(&sendbuffer, &data, 4);
                        Socket.SendTo(players[i]->client, sendbuffer, 500);
                    }


                    //Respond with 1 to accept, followed by a player ID

                    int newID = players.size();
                    Player* np = new Player(sender, newID);
                    players.push_back(np);

                    //Read player initial position and velocity
                    Helpers::ReadPlayerMovementData(np, &buffer[0] + 1);

                    /*for (size_t i = 0; i < 8; i++)
                    {
                        std::bitset<8> x(sendbuffer[i]);
                        std::cout << x << " ";
                    }*/

                    //Send a response
                    std::fill_n(sendbuffer, 500, 0);

                    unsigned int data = 1;
                    std::memcpy(&sendbuffer, &data, 4);
                    data = np->GetID();
                    std::memcpy(&sendbuffer[4], &data, 4);

                    Socket.SendTo(sender, sendbuffer, 500);

                    std::cout << "Player " << np->GetID() << " joined.\n";

                    //Clear buffer
                    std::fill_n(buffer, 500, 0);
                    newData = false;

                }
                else if (*msgType == 3) //New projectile
                {
                    int* index = (int*)(&buffer[0] + 4);

                    Projectile* p = &projectiles[*index];
                    Helpers::ReadProjectileMovementData(p, &buffer[0] + 8);

                    newData = false;

                }
                else if(*msgType == 10) //Player update
                { 
                    unsigned int playerID = *(msgType + 1);
                    //Find that player
                    Player* p = nullptr;
                    for (size_t i = 0; i < players.size(); i++)
                    {
                        if (players[i]->ID == playerID)
                        {
                            p = players[i];
                            break;
                        }
                    }
                    if (p == nullptr)
                    {
                        //Clear buffer
                        std::fill_n(buffer, 500, 0);
                        newData = false;
                        continue;
                    }

                    //Read player initial position and velocity
                    float posX, posY, posZ, velX, velY, velZ;
                    float* posData = (float*)&buffer;
                    posX = *(posData + 2);
                    posY = *(posData + 3);
                    posZ = *(posData + 4);
                    velX = *(posData + 5);
                    velY = *(posData + 6);
                    velZ = *(posData + 7);

                    p->SetPosition(posX, posY, posZ);
                    p->SetVelocity(velX, velY, velZ);

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
            float deltaTime = duration_cast<ms>(FPS).count() / 1000;
            //std::cout << "LastFrame: " << duration_cast<ms>(FPS).count() << "ms  |  FPS: " << FPS.count() * 60 << std::endl;
        
            //Update every player
            for (size_t i = 0; i < players.size(); i++)
            {
                players[i]->Update(deltaTime);
            }

            //Update every projectile
            for (int i = 0; i < MAX_PROJECTILES; i++)
            {
                Projectile* p = &(projectiles[i]);
                if (!p->dead)
                {
                    p->Update(deltaTime);
                    if (p->dead)
                    {
                        //Do nothing I guess?? lmao
                        p->GetTransform()->SetPosition(0, -5000, 0);
                    }
                }
            }

            for (int i = 0; i < players.size(); i++)
            {
                for (int j = 0; j < MAX_PROJECTILES; j++)
                {
                    //ignore a few frames to avoid instant self collision
                    if (projectiles[j].dead || projectiles[j].age < 0.1f) continue; 
                    if (Helpers::CheckProjectileCollision(players[i], &projectiles[j], deltaTime, 3))
                    {
                        std::cout << "Player " << i << " is hit!" << std::endl;
                        projectiles[j].dead = true;
                        projectiles[j].age = projectiles[j].lifespan + 1; //tells the clients it's dead
                        projectiles[j].GetTransform()->SetPosition(0, -5000, 0);
                    }
                }
            }
        
            //Send player position and velocity data to each client
            std::fill_n(sendbuffer, 500, 0);

            int msgtyp = 10;

            std::memcpy(&sendbuffer, &msgtyp, 4);
            for (size_t i = 0; i < players.size(); i++)
            {
                Helpers::CopyPlayerMovementData(players[i], &sendbuffer[0] + (i * 36) + 4);
            }
            for (size_t i = 0; i < MAX_PROJECTILES; i++)
            {
                Helpers::CopyProjectileMovementData(&projectiles[i], &sendbuffer[0] + (players.size() * 36) + (i * 48) + 4);
            }
            for (size_t i = 0; i < players.size(); i++)
            {
                Socket.SendTo(players[i]->client, sendbuffer, 500);
            }
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

        for (int i = 0; i < MAX_PROJECTILES; i++)
        {
            projectiles[i].GetTransform()->SetPosition(0, -5000, 0);
        }

        std::cout << "Server online. Port number " << PORT << std::endl;

        gameLoop.join();

        recvLoop.join();

    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

