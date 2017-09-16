#pragma once
#ifndef liuzianglib_SELECT
#define liuzianglib_SELECT
#include <winsock2.h>
#include <vector>
#include <queue>
#include <string>
#include "liuzianglib.h"
#include "DC_WinSock.h"
#pragma comment(lib,"ws2_32.lib")
//Version 2.4.21V30
//20170914

namespace DC {

	namespace Web {

		namespace Server {

			namespace SELECT {

				namespace SELECTSpace {

					using unique_lock_mutex_t = std::unique_lock<std::mutex>;

				}

				class Server {
				public:
					Server(const std::size_t& inputThreadNumber) :TP(nullptr), ThreadNumber(inputThreadNumber), m_listen(INVALID_SOCKET), SELECT_TV(0), cv_notified(false) {}

					Server(const Server&) = delete;

					virtual ~Server()noexcept {}

				public:
					inline bool Start()noexcept {
						if (!init()) { uninit(); return false; }
						//推送监听任务到线程池
						TP->async(&Server::ListenerThread, this);
						//开始线程池
						TP->start();
						return true;
					}

					inline void Stop() {
						uninit();
					}

					inline void SetListenAddr(const WinSock::Address& input)noexcept {
						bindAddr = input;
					}

					inline void SetListenAddr(const std::string& ip, const int32_t& port)noexcept {
						bindAddr = WinSock::MakeAddr(ip, port);
					}

					static inline void loop()noexcept {//永远不会返回
						while (true)
							std::this_thread::sleep_for(std::chrono::minutes(9710));
					}

					inline void SetSelectTime(const int32_t& input) {
						SELECT_TV.store(std::memory_order::memory_order_release);
					}

				protected:
					virtual inline const std::size_t getRecvBufferSize()noexcept {//recv 缓冲区
						return 1024;
					}

					virtual std::string DoRecv(const DC::WinSock::Socket& s) {
						std::string recvstr;
						if (!DC::WinSock::Recv(s, recvstr, 1024)) throw DC::Exception("DoRecv", "DC::WinSock::Recv ERROR");
						return recvstr;
					}

					virtual void OnRecv(const DC::WinSock::Socket& s, std::string str) {
						if (!DC::WinSock::Send(s, str)) throw DC::Exception("DoRecv", "DC::WinSock::Send ERROR");
					}

					virtual void OnError(const DC::Exception& ex) {
						std::cout << "\n\nERROR::" << ex.GetTitle() << "  " << ex.GetDescription() << "\n\n";
					}

				private:
					bool ListenerThread()noexcept {
						try {
							WinSock::SocketInitTCP(m_listen);
							if (m_listen == SOCKET_ERROR) return false;

							if (!WinSock::Bind(m_listen, bindAddr)) { closesocket(m_listen); return false; }
							if (!WinSock::Listen(m_listen, SOMAXCONN)) { closesocket(m_listen); return false; }

							SOCKET acceptSocket;
							DC::WinSock::Address ClientAddr;

							while (true) {
								acceptSocket = INVALID_SOCKET;
								if (!DC::WinSock::Accept(acceptSocket, m_listen, ClientAddr)) { closesocket(m_listen); return false; }
								if (acceptSocket == INVALID_SOCKET) { closesocket(m_listen); return false; }

								SELECTSpace::unique_lock_mutex_t lock(m_clients_mutex);
								m_clients.push_back(acceptSocket);
								cv_notified.store(true, std::memory_order::memory_order_release);
								new_client_cv.notify_one();
							}

							abort();//不应该执行到这
						}
						catch (const DC::Exception& ex) { OnError(ex); return false; }
						catch (...) { OnError(DC::Exception("ListenerThread", "unknown error")); return false; }
						return false;
					}

					inline bool init()noexcept {
						uninit();
						//新建线程池
						try {
							if (!DC::isNull(TP)) throw DC::Exception("init", "TP is not empty");
							TP = new DC::ThreadPool(ThreadNumber + 2);//ListenerThread, CheckerThread
						}
						catch (const std::bad_alloc& ex) { OnError(DC::Exception("init", std::string("std::bad_alloc:") + ex.what())); return false; }
						catch (const DC::Exception& ex) { OnError(ex); return false; }
						catch (...) { OnError(DC::Exception("init", "unknown error")); return false; }			
						return true;
					}

					void uninit()noexcept {
						//停止监听
						DC::WinSock::Close(m_listen);
						m_listen = INVALID_SOCKET;
						//停止并删除线程池
						if (!DC::isNull(TP)) {
							delete TP;
							TP = nullptr;
						}
						//清空客户端socket等待队列
						SELECTSpace::unique_lock_mutex_t lock(m_clients_mutex);
						for (const auto& p : m_clients)
							DC::WinSock::Close(p);
						m_clients.clear();
					}

					void CheckerThread()noexcept {
						fd_set fd_read;						
						int32_t ret = 0;

						while (true) {
							SELECTSpace::unique_lock_mutex_t lock(m_clients_mutex, std::defer_lock);
							new_client_cv.wait_for(lock, std::chrono::seconds(SELECT_TV.load(std::memory_order::memory_order_acquire)), [&] {return cv_notified.load(std::memory_order::memory_order_acquire); });
							cv_notified.store(false, std::memory_order::memory_order_release);

							if (m_clients.empty()) continue;

							timeval tv = { SELECT_TV.load(std::memory_order::memory_order_acquire),0 };

							FD_ZERO(&fd_read);

							for (const auto& p : m_clients)
								FD_SET(p, &fd_read);

							if (select(NULL, &fd_read, NULL, NULL, &tv) == 0)
								continue;

							SELECTSpace::unique_lock_mutex_t lock2(m_read_clients_mutex);
							for (auto i = m_clients.begin(); i != m_clients.end(); ) {
								if (FD_ISSET(*i, &fd_read)) {
									m_read_clients.push(*i);
									i = DC::vector_fast_erase(m_clients, i);
									new_read_cv.notify_one();
									continue;
								}
								i++;
							}

						}
					}

					void WorkerThread()noexcept {
						while (true) {
							SELECTSpace::unique_lock_mutex_t lock(m_read_clients_mutex, std::defer_lock), lock2(m_clients_mutex, std::defer_lock);
							new_read_cv.wait(lock, [&] {return !m_read_clients.empty(); });

							auto sockHolder = m_read_clients.front();
							m_read_clients.pop();
							lock.unlock();

							std::string recvstr;
							try {
								recvstr = DoRecv(sockHolder);//DoRecv抛异常的时候这个sock会被关闭然后抛弃
							}
							catch (const DC::Exception& ex) { OnError(ex); DC::WinSock::Close(sockHolder); continue; }
							catch (...) { OnError(DC::Exception("DoRecv", "unknown error")); DC::WinSock::Close(sockHolder); continue; }
							try {
								OnRecv(sockHolder, std::move(recvstr));
							}
							catch (const DC::Exception& ex) { OnError(ex); }
							catch (...) { OnError(DC::Exception("OnRecv", "unknown error")); }

							lock2.lock();
							m_clients.push_back(sockHolder);
						}
					}

				private:					
					ThreadPool *TP;
					const std::size_t ThreadNumber;

					SOCKET m_listen;
					std::vector<DC::WinSock::Socket> m_clients;//所有客户端
					std::queue<DC::WinSock::Socket> m_read_clients;//需要read的客户端
					WinSock::Address bindAddr;

					std::mutex m_clients_mutex, m_read_clients_mutex;
					std::condition_variable new_client_cv, new_read_cv;

					std::atomic<int32_t> SELECT_TV;
					std::atomic_bool cv_notified;
				};

			}

		}

	}

}

#endif
