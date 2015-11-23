#pragma once

/*

The AcceptEx function accepts a new connection, returns the local and remote address,
and receives the first block of data sent by the client application.

BOOL AcceptEx(
_In_  SOCKET       sListenSocket,
_In_  SOCKET       sAcceptSocket,
_In_  PVOID        lpOutputBuffer,
_In_  DWORD        dwReceiveDataLength,
_In_  DWORD        dwLocalAddressLength,
_In_  DWORD        dwRemoteAddressLength,
_Out_ LPDWORD      lpdwBytesReceived,
_In_  LPOVERLAPPED lpOverlapped
);

Parameters

sListenSocket [in]
    A descriptor identifying a socket that has already been called with the listen function.
    A server application waits for attempts to connect on this socket.
sAcceptSocket [in]
    A descriptor identifying a socket on which to accept an incoming connection. This socket must not be bound or connected.
    lpOutputBuffer [in]
    A pointer to a buffer that receives the first block of data sent on a new connection, 
    the local address of the server, and the remote address of the client.
    The receive data is written to the first part of the buffer starting at offset zero,
    while the addresses are written to the latter part of the buffer. This parameter must be specified.
dwReceiveDataLength [in]
    The number of bytes in lpOutputBuffer that will be used for actual receive data at the beginning of the buffer.
    This size should not include the size of the local address of the server, nor the remote address of the client; they are appended to the output buffer. If dwReceiveDataLength is zero, accepting the connection will not result in a receive operation. Instead, AcceptEx completes as soon as a connection arrives, without waiting for any data.
dwLocalAddressLength [in]
    The number of bytes reserved for the local address information. 
    This value must be at least 16 bytes more than the maximum address length for the transport protocol in use.
dwRemoteAddressLength [in]
    The number of bytes reserved for the remote address information.
    This value must be at least 16 bytes more than the maximum address length for the transport protocol in use. Cannot be zero.
lpdwBytesReceived [out]
    A pointer to a DWORD that receives the count of bytes received.
    This parameter is set only if the operation completes synchronously.
    If it returns ERROR_IO_PENDING and is completed later,
    then this DWORD is never set and you must obtain the number of bytes read from the completion notification mechanism.
lpOverlapped [in]
    An OVERLAPPED structure that is used to process the request. This parameter must be specified; it cannot be NULL.
Return value

    If no error occurs, the AcceptEx function completed successfully and a value of TRUE is returned.
    If the function fails, AcceptEx returns FALSE.
    The WSAGetLastError function can then be called to return extended error information.
    If WSAGetLastError returns ERROR_IO_PENDING, then the operation was successfully initiated and is still in progress.
    If the error is WSAECONNRESET, an incoming connection was indicated,
    but was subsequently terminated by the remote peer prior to accepting the call.
Remarks

    The AcceptEx function combines several socket functions into a single API/kernel transition.
    The AcceptEx function, when successful, performs three tasks:
    A new connection is accepted.
    Both the local and remote addresses for the connection are returned.
    The first block of data sent by the remote is received.

A single output buffer receives the data, the local socket address (the server), and the remote socket address (the client).
When using AcceptEx, the GetAcceptExSockaddrs function must be called to parse the buffer into its
three distinct parts (data, local socket address, and remote socket address). 
On Windows XP and later, 
once the AcceptEx function completes and the SO_UPDATE_ACCEPT_CONTEXT option is set on the accepted socket,
the local address associated with the accepted socket can also be retrieved using the getsockname function.
Likewise, the remote address associated with the accepted socket can be retrieved using the getpeername function.

The buffer size for the local and remote address must be 16 bytes more than the size of the sockaddr structure
for the transport protocol in use because the addresses are written in an internal format. 
For example, the size of a sockaddr_in (the address structure for TCP/IP) is 16 bytes. 
Therefore, a buffer size of at least 32 bytes must be specified for the local and remote addresses.






The GetAcceptExSockaddrs function parses the data obtained from a call to the AcceptEx function 
and passes the local and remote addresses to a sockaddr structure.

void GetAcceptExSockaddrs(
_In_  PVOID      lpOutputBuffer,
_In_  DWORD      dwReceiveDataLength,
_In_  DWORD      dwLocalAddressLength,
_In_  DWORD      dwRemoteAddressLength,
_Out_ LPSOCKADDR *LocalSockaddr,
_Out_ LPINT      LocalSockaddrLength,
_Out_ LPSOCKADDR *RemoteSockaddr,
_Out_ LPINT      RemoteSockaddrLength
);

Parameters

lpOutputBuffer [in]
    A pointer to a buffer that receives the first block of data sent on a connection resulting from an AcceptEx call. 
    Must be the same lpOutputBuffer parameter that was passed to the AcceptEx function.
dwReceiveDataLength [in]
    The number of bytes in the buffer used for receiving the first data. 
    This value must be equal to the dwReceiveDataLength parameter that was passed to the AcceptEx function.
dwLocalAddressLength [in]
    The number of bytes reserved for the local address information. 
    This value must be equal to the dwLocalAddressLength parameter that was passed to the AcceptEx function.
dwRemoteAddressLength [in]
    The number of bytes reserved for the remote address information. 
    This value must be equal to the dwRemoteAddressLength parameter that was passed to the AcceptEx function.
LocalSockaddr [out]
    A pointer to the sockaddr structure that receives the local address of 
    the connection (the same information that would be returned by the getsockname function). 
    This parameter must be specified.
LocalSockaddrLength [out]
    The size, in bytes, of the local address. This parameter must be specified.
RemoteSockaddr [out]
    A pointer to the sockaddr structure that receives the remote address of 
    the connection (the same information that would be returned by the getpeername function). 
    This parameter must be specified.
RemoteSockaddrLength [out]
    The size, in bytes, of the local address. This parameter must be specified.
Return value

This function does not return a value.
Remarks

The GetAcceptExSockaddrs function is used exclusively with the AcceptEx function to
parse the first data that the socket receives into local and remote addresses. 
The AcceptEx function returns local and remote address information in an internal format. 
Application developers need to use the GetAcceptExSockaddrs function if there is a need for 
the sockaddr structures containing the local or remote addresses.

*/