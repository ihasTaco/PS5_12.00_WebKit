/*
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
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

#include <JavaScriptCore/RemoteInspectorSocketEndpoint.h>
#include <chrono>
#include <memory>
#include <thread>

using namespace Inspector;
using namespace std::chrono_literals;

namespace TestWebKitAPI {

namespace SocketTest {

/* =======================================================
 *  Mock Objects
 * ======================================================= */

class SampleClient : public RemoteInspectorSocketEndpoint::Client {
    WTF_MAKE_FAST_ALLOCATED;
public:
    void didReceive(RemoteInspectorSocketEndpoint&, ConnectionID, Vector<uint8_t>&&) final { }
    void didClose(RemoteInspectorSocketEndpoint&, ConnectionID) final { }

    Vector<char> buffer;
};

class NullClient : public RemoteInspectorSocketEndpoint::Client {
    WTF_MAKE_FAST_ALLOCATED;
public:
    void didReceive(RemoteInspectorSocketEndpoint&, ConnectionID, Vector<uint8_t>&&) final { }
    void didClose(RemoteInspectorSocketEndpoint&, ConnectionID) final { }
};

class EchoClient : public RemoteInspectorSocketEndpoint::Client {
    WTF_MAKE_FAST_ALLOCATED;
public:
    void didReceive(RemoteInspectorSocketEndpoint& endpoint, ConnectionID cid, Vector<uint8_t>&& packet) final
    {
        endpoint.send(cid, packet);
    }

    void didClose(RemoteInspectorSocketEndpoint&, ConnectionID) final { }
};

template <class ServerClient = NullClient>
class SampleServer : public RemoteInspectorSocketEndpoint::Listener {
public:
    SampleServer()
    {
        auto& endpoint = RemoteInspectorSocketEndpoint::singleton();

        connectionID = endpoint.listenInet(nullptr, 0, *this);
        if (connectionID) {
            if (auto port = endpoint.getPort(*connectionID))
                this->port = *port;
        }
    }

    ~SampleServer()
    {
        auto& endpoint = RemoteInspectorSocketEndpoint::singleton();

        if (connectionID)
            endpoint.disconnect(*connectionID);
    }

    std::optional<ConnectionID> doAccept(RemoteInspectorSocketEndpoint& endpoint, PlatformSocketType sock) final
    {
        connectCount += 1;
        if (rejecting)
            return std::nullopt;

        auto client = makeUnique<ServerClient>();
        auto connectionID = endpoint.createClient(sock, *client);
        if (!connectionID)
            return std::nullopt;

        clients.set(*connectionID, WTFMove(client));
        return connectionID;
    }

    void didChangeStatus(RemoteInspectorSocketEndpoint&, ConnectionID, RemoteInspectorSocketEndpoint::Listener::Status) final { }

    std::optional<ConnectionID> connectionID;
    uint16_t port { };
    bool rejecting { };
    HashMap<ConnectionID, std::unique_ptr<ServerClient>> clients { };
    int connectCount { };
};

/* =======================================================
 *  Test Suite
 * ======================================================= */

class SocketTest : public testing::Test {
public:
    void SetUp() final
    {
    }

    void TearDown() final
    {
    }

protected:
    void shortBreak() { std::this_thread::sleep_for(100ms); }
};

/* =======================================================
 *  Test cases
 * ======================================================= */

TEST_F(SocketTest, Launch)
{
    SampleServer server;

    EXPECT_TRUE(server.connectionID);
    EXPECT_GT(server.port, 0);
}

TEST_F(SocketTest, ListenerAcceptMultipleConnections)
{
    SampleServer server;

    auto sock = Socket::connect("127.0.0.1", server.port);
    EXPECT_TRUE(sock);

    shortBreak();
    EXPECT_EQ(server.connectCount, 1);
    EXPECT_EQ(server.clients.size(), 1);

    Socket::connect("127.0.0.1", server.port);
    Socket::connect("127.0.0.1", server.port);

    // try connecting with different timing
    shortBreak();
    Socket::connect("127.0.0.1", server.port);

    shortBreak();
    EXPECT_EQ(server.connectCount, 4);
    EXPECT_EQ(server.clients.size(), 4);
}

TEST_F(SocketTest, ListenerReject)
{
    SampleServer server;
    server.rejecting = true;

    auto sock = Socket::connect("127.0.0.1", server.port);
    // client doesn't know that the connection is rejecting at this moment.
    EXPECT_TRUE(sock);

    shortBreak();
    EXPECT_EQ(server.connectCount, 1);
    EXPECT_TRUE(server.clients.isEmpty());
}

TEST_F(SocketTest, ListenerReadWrite)
{
    SampleServer<EchoClient> server;

    auto sockOrNull = Socket::connect("127.0.0.1", server.port);
    ASSERT_TRUE(sockOrNull);

    shortBreak();
    EXPECT_TRUE(!server.clients.isEmpty());

    auto sock = *sockOrNull;
    {
        auto result = Socket::write(sock, "Hello", 6); // including null byte.
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, 6);
    }

    shortBreak();
    {
        char buffer[32];
        auto result = Socket::read(sock, buffer, sizeof(buffer));
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, 6);

        EXPECT_STREQ(buffer, "Hello");
    }
}

TEST_F(SocketTest, ClientConnect)
{
    SampleServer server;

    auto& endpoint = RemoteInspectorSocketEndpoint::singleton();
    SampleClient client;
    auto clientID = endpoint.connectInet("127.0.0.1", server.port, client);
    EXPECT_TRUE(clientID);

    shortBreak();
    EXPECT_EQ(server.clients.size(), 1);
}

}

}
