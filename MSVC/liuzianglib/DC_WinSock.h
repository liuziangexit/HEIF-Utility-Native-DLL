#pragma once
#ifndef liuzianglib_TCP
#define liuzianglib_TCP
#include <winsock2.h>
#include <string>
#include <memory>
#pragma comment(lib, "ws2_32.lib")
//Version 2.4.21V30
//20170914
//注意:DC_WinSock.h必须先于DC_MySQL.h包含

namespace DC {

	namespace WinSock {

		using Address = sockaddr_in;
		using Socket = SOCKET;

		Address MakeAddr(const std::string& address, std::size_t port) {
			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.S_un.S_addr = inet_addr(address.c_str());
			return addr;
		}

		inline std::string GetAddrString(const Address& input) {
			return inet_ntoa(input.sin_addr);
		}

		inline bool Startup(int a, int b) {
			WORD SockVersion = MAKEWORD(a, b);
			WSADATA WsaData;
			if (WSAStartup(SockVersion, &WsaData) != 0) {
				return false;
			}
			return true;
		}

		inline void Cleanup() {
			WSACleanup();
		}

		inline void SocketInit(SOCKET& s, const int& af, const int& type, const int& protocol) {
			s = socket(af, type, protocol);
		}

		inline void SocketInitOverlapped(Socket& s) {
			s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		}

		inline void SocketInitTCP(SOCKET& s) {
			SocketInit(s, AF_INET, SOCK_STREAM, IPPROTO_TCP);
		}

		inline bool Bind(SOCKET s, const sockaddr_in& addr) {
			if (bind(s, (LPSOCKADDR)&addr, sizeof(addr)) != 0) {
				return false;
			}
			return true;
		}

		inline void UnBind(SOCKET s) {
			closesocket(s);
		}

		inline bool Listen(SOCKET s, int QueueSize) {
			if (listen(s, QueueSize) != 0) {
				return false;
			}
			return true;
		}

		inline void UnListen(SOCKET s) {
			closesocket(s);
		}

		inline bool Accept(SOCKET& accSock, SOCKET SockListen, sockaddr_in& RemoteAddr) {
			int RemoteAddrLen = sizeof(RemoteAddr);
			accSock = accept(SockListen, (sockaddr*)&RemoteAddr, &RemoteAddrLen);
			if (accSock == INVALID_SOCKET) {
				return false;
			}
			return true;
		}

		inline bool Accept(SOCKET& accSock, SOCKET SockListen) {
			Address temp;
			int RemoteAddrLen = sizeof(temp);
			accSock = accept(SockListen, reinterpret_cast<sockaddr*>(&temp), &RemoteAddrLen);
			if (accSock == INVALID_SOCKET) {
				return false;
			}
			return true;
		}

		inline bool Connect(SOCKET s, sockaddr_in addr) {
			if (connect(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
				return false;
			}
			return true;
		}

		inline bool Send(SOCKET s, const std::string& str) {
			return send(s, str.c_str(), str.size(), 0) != SOCKET_ERROR;
		}

		bool Recv(SOCKET s, std::string& str, std::size_t len) {
			bool returnvalue = false;
			std::unique_ptr<char[]> recvMessagech(new char[len + 1]);
			int ret = recv(s, recvMessagech.get(), len, 0);
			if (ret > 0) {
				str = std::string(recvMessagech.get(), ret);
				returnvalue = true;
			}
			return returnvalue;
		}

		inline void Close(SOCKET s) {
			closesocket(s);
		}

		inline bool SetRecvTimeOut(SOCKET s, std::size_t limit) {
			if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&limit, sizeof(int)) == SOCKET_ERROR) return false;
			return true;
		}

		inline bool SetSendTimeOut(SOCKET s, std::size_t limit) {
			if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&limit, sizeof(int)) == SOCKET_ERROR) return false;
			return true;
		}

		bool GetAddressByHost(const std::string& Host, std::vector<std::string>& AddressOutPut) {
			auto result = gethostbyname(Host.c_str());
			if (result == NULL) return false;

			for (int i = 0; result->h_addr_list[i] != NULL; i++)
				AddressOutPut.emplace_back(inet_ntoa(*(reinterpret_cast<in_addr *>(result->h_addr_list[i]))));

			return true;
		}

	}

}

#endif
