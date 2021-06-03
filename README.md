## Description

This is an example of VPN client and server on the UWP platform. This repo contains 2 sub-projects
1. The server program, the path is in the `./Server` directory, and the compilation command line is
>g++ VpnServer.cpp VpnServer`
2. UWP program, the path is in `./UWPClient/VPNSample.sln`, the solution contains two projects, first compile `VpnDll`, then compile `VPNSample`

## Configuration
1. The server port can be modified in `VpnServer.cpp`, the specific code is
>#define PORT 10000`

2. The IP and port that the client is connected to can be modified in the sample project
