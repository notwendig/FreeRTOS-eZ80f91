/*
 * FreeRTOS V202011.00
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* UDP command console, one serialized FreeRTOS+CLI interpreter. */
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_CLI.h"
#include "UDPCommandInterpreter.h"

#define cmdMAX_INPUT_SIZE 60
#define cmdMAX_OUTPUT_SIZE 1250

static void vUDPCommandInterpreterTask(void *parameter)
{
    static char input[cmdMAX_INPUT_SIZE];
    static char output[cmdMAX_OUTPUT_SIZE];
    static char received[ipconfigNETWORK_MTU];
    struct freertos_sockaddr server, client, owner;
    socklen_t addressLength;
    Socket_t socket;
    BaseType_t count, i, more;
    size_t used = 0;
    BaseType_t overflow = pdFALSE, haveOwner = pdFALSE;

    while(FreeRTOS_IsNetworkUp() == pdFALSE)
        vTaskDelay(pdMS_TO_TICKS(100));
    socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM,
                             FREERTOS_IPPROTO_UDP);
    configASSERT(socket != FREERTOS_INVALID_SOCKET);
    memset(&server, 0, sizeof(server));
    server.sin_family = FREERTOS_AF_INET;
    server.sin_port = FreeRTOS_htons((uint16_t)(uint24_t)parameter);
    configASSERT(FreeRTOS_bind(socket, &server, sizeof(server)) == 0);

    for(;;)
    {
        memset(&client, 0, sizeof(client));
        addressLength = sizeof(client);
        count = FreeRTOS_recvfrom(socket, received, sizeof(received), 0,
                                  &client, &addressLength);
        if(count <= 0)
            continue;
        /* A newly attached terminal must not inherit another peer's line. */
        if(!haveOwner || memcmp(&owner, &client, sizeof(client)) != 0)
        {
            owner = client;
            haveOwner = pdTRUE;
            used = 0;
            overflow = pdFALSE;
        }
        for(i = 0; i < count; ++i)
        {
            char c = received[i];
            if(c == '\r')
                continue;
            if(c == '\n')
            {
                input[used] = 0;
                if(overflow)
                {
                    const char *error = "Command too long (maximum 59 characters).\r\n";
                    FreeRTOS_sendto(socket, error, strlen(error), 0,
                                    &client, addressLength);
                }
                else if(used)
                {
                    do
                    {
                        output[0] = 0;
                        more = FreeRTOS_CLIProcessCommand(input, output, sizeof(output));
                        FreeRTOS_sendto(socket, output, strlen(output), 0,
                                        &client, addressLength);
                    } while(more != pdFALSE);
                }
                FreeRTOS_sendto(socket, "\r\n", 2, 0, &client, addressLength);
                used = 0;
                overflow = pdFALSE;
            }
            else if(c == '\b' || c == 127)
            {
                if(used && !overflow)
                    --used;
            }
            else if(c >= ' ' && !overflow)
            {
                if(used < sizeof(input) - 1)
                    input[used++] = c;
                else
                    overflow = pdTRUE;
            }
        }
    }
}

void vStartUDPCommandInterpreterTask(uint16_t stackSize, uint32_t port,
                                      UBaseType_t priority)
{
    configASSERT(xTaskCreate(vUDPCommandInterpreterTask, "CLI", stackSize,
                              (void *)(uint24_t)port, priority, NULL) == pdPASS);
}
