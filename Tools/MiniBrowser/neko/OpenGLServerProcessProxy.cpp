/*
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "OpenGLServerProcessProxy.h"

// Because this file doesn't include any webkit header, we cannot use HAVE() macro.
#if HAVE_OPENGL_SERVER_SUPPORT
#include <errno.h>
#include <fcntl.h>
#include <piglet/piglet_vsh.h>
#include <process-launcher.h>
#include <slimglipc_setup.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

std::unique_ptr<OpenGLServerProcessProxy> OpenGLServerProcessProxy::create()
{
    auto newProcess = std::unique_ptr<OpenGLServerProcessProxy>(new OpenGLServerProcessProxy());
    if (newProcess->launch())
        return newProcess;

    return nullptr;
}

bool OpenGLServerProcessProxy::launch()
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1)
        return false;

    // FIXME: What if these settings are failed?
    if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC) == -1)
        printf("fctl error %d\n", errno);

    int sendBufSize = 32 * 1024;
    if (setsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &sendBufSize, 4) == -1)
        printf("setsockopt error %d\n", errno);
    if (setsockopt(sockets[1], SOL_SOCKET, SO_SNDBUF, &sendBufSize, 4) == -1)
        printf("setsockopt error %d\n", errno);

    int recvBufSize = 32 * 1024;
    if (setsockopt(sockets[0], SOL_SOCKET, SO_RCVBUF, &recvBufSize, 4) == -1)
        printf("setsockopt error %d\n", errno);
    if (setsockopt(sockets[1], SOL_SOCKET, SO_RCVBUF, &recvBufSize, 4) == -1)
        printf("setsockopt error %d\n", errno);

    PlayStation::LaunchParam param { sockets[1], -1 };
    char* argv[] = { 0 };
    int32_t appLocalPid = PlayStation::launchProcess(
        "/app0/SlimGLRenderServer_vsh.self", argv, param);
    ::close(sockets[1]);

    if (appLocalPid < 0) {
        ::close(sockets[0]);
        return false;
    }

    m_processSocket = sockets[0];
    m_processIdentifier = appLocalPid;

    receiveServerSocket();

    return true;
}

OpenGLServerProcessProxy::OpenGLServerProcessProxy()
{
}

OpenGLServerProcessProxy::~OpenGLServerProcessProxy()
{
    if (m_processIdentifier)
        PlayStation::terminateProcess(m_processIdentifier);
}

bool OpenGLServerProcessProxy::receiveServerSocket()
{
    // wait to receive the socket to the server process
    int sock2server = sceSlimglIPCSetupRecvFd(m_processSocket);
    if (sock2server <= 0)
        return false;

    m_serverSocket = sock2server;
    return true;
}

int OpenGLServerProcessProxy::serverConnection()
{
    return m_serverSocket;
}

#endif
