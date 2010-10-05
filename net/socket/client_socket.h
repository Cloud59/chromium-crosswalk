// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_CLIENT_SOCKET_H_
#define NET_SOCKET_CLIENT_SOCKET_H_
#pragma once

#include "net/socket/socket.h"

namespace net {

class AddressList;
class BoundNetLog;

class ClientSocket : public Socket {
 public:
  virtual ~ClientSocket() {}

  // Called to establish a connection.  Returns OK if the connection could be
  // established synchronously.  Otherwise, ERR_IO_PENDING is returned and the
  // given callback will run asynchronously when the connection is established
  // or when an error occurs.  The result is some other error code if the
  // connection could not be established.
  //
  // The socket's Read and Write methods may not be called until Connect
  // succeeds.
  //
  // It is valid to call Connect on an already connected socket, in which case
  // OK is simply returned.
  //
  // Connect may also be called again after a call to the Disconnect method.
  //
  virtual int Connect(CompletionCallback* callback) = 0;

  // Called to disconnect a socket.  Does nothing if the socket is already
  // disconnected.  After calling Disconnect it is possible to call Connect
  // again to establish a new connection.
  //
  // If IO (Connect, Read, or Write) is pending when the socket is
  // disconnected, the pending IO is cancelled, and the completion callback
  // will not be called.
  virtual void Disconnect() = 0;

  // Called to test if the connection is still alive.  Returns false if a
  // connection wasn't established or the connection is dead.
  virtual bool IsConnected() const = 0;

  // Called to test if the connection is still alive and idle.  Returns false
  // if a connection wasn't established, the connection is dead, or some data
  // have been received.
  virtual bool IsConnectedAndIdle() const = 0;

  // Copies the peer address to |address| and returns a network error code.
  // ERR_SOCKET_NOT_CONNECTED will be returned if the socket is not connected.
  virtual int GetPeerAddress(AddressList* address) const = 0;

  // Gets the NetLog for this socket.
  virtual const BoundNetLog& NetLog() const = 0;

  // Set the annotation to indicate this socket was created for speculative
  // reasons.  This call is generally forwarded to a basic TCPClientSocket*,
  // where a UseHistory can be updated.
  virtual void SetSubresourceSpeculation() = 0;
  virtual void SetOmniboxSpeculation() = 0;

  // Returns true if the underlying transport socket ever had any reads or
  // writes.  ClientSockets layered on top of transport sockets should forward
  // this call to the transport socket.
  virtual bool WasEverUsed() const = 0;

 protected:
  // The following class is only used to gather statistics about the history of
  // a socket.  It is only instantiated and used in basic sockets, such as
  // TCPClientSocket* instances.  Other classes that are derived from
  // ClientSocket should forward any potential settings to their underlying
  // transport sockets.
  class UseHistory {
   public:
    UseHistory();
    ~UseHistory();

    void set_was_ever_connected();
    void set_was_used_to_convey_data();

    // The next two setters only have any impact if the socket has not yet been
    // used to transmit data.  If called later, we assume that the socket was
    // reused from the pool, and was NOT constructed to service a speculative
    // request.
    void set_subresource_speculation();
    void set_omnibox_speculation();

    bool was_used_to_convey_data() const;

   private:
    // Summarize the statistics for this socket.
    void EmitPreconnectionHistograms() const;
    // Indicate if this was ever connected.
    bool was_ever_connected_;
    // Indicate if this socket was ever used to transmit or receive data.
    bool was_used_to_convey_data_;

    // Indicate if this socket was first created for speculative use, and
    // identify the motivation.
    bool omnibox_speculation_;
    bool subresource_speculation_;
    DISALLOW_COPY_AND_ASSIGN(UseHistory);
  };
};

}  // namespace net

#endif  // NET_SOCKET_CLIENT_SOCKET_H_
