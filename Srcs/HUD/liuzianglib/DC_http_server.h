#pragma once
#ifndef liuzianglib_http_server
#define liuzianglib_http_server
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <limits>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_set>
#include "DC_http.h"
#ifdef __SSE2__
#include <emmintrin.h>
namespace DC {
	namespace Web {
		namespace http {
			namespace serverSpace {
				inline void spin_loop_pause() noexcept { _mm_pause(); }
			}
		}
	}
}
#elif defined(_MSC_VER) && _MSC_VER >= 1800 && (defined(_M_X64) || defined(_M_IX86))
#include <intrin.h>
namespace DC {
	namespace Web {
		namespace http {
			namespace serverSpace {
				inline void spin_loop_pause() noexcept { _mm_pause(); }
			}
		}
	}
}
#else
namespace DC {
	namespace Web {
		namespace http {
			namespace serverSpace {
				inline void spin_loop_pause() noexcept {}
			}
		}
	}
}
#endif
#ifdef USE_STANDALONE_ASIO
#include <asio.hpp>
#include <asio/steady_timer.hpp>
namespace DC {
	namespace Web {
		namespace http {
			namespace serverSpace {
				using error_code = std::error_code;
				using errc = std::errc;
				namespace make_error_code = std;
			}
		}
	}
}
#else
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
namespace DC {
	namespace Web {
		namespace http {
			namespace serverSpace {
				namespace asio = boost::asio;
				using error_code = boost::system::error_code;
				namespace errc = boost::system::errc;
				namespace make_error_code = boost::system::errc;
			}
		}
	}
}
#endif
#include <regex>
//Version 2.4.21V44
//20171027

namespace DC {

	namespace Web {

		namespace http {

			namespace serverSpace {

				namespace regex = std;

				inline bool case_insensitive_equal(const std::string &str1, const std::string &str2) noexcept {
					return str1.size() == str2.size() &&
						std::equal(str1.begin(), str1.end(), str2.begin(), [](char a, char b) {
						return tolower(a) == tolower(b);
					});
				}

				class CaseInsensitiveEqual {
				public:
					bool operator()(const std::string &str1, const std::string &str2) const noexcept {
						return case_insensitive_equal(str1, str2);
					}
				};

				class CaseInsensitiveHash {
				public:
					std::size_t operator()(const std::string &str) const noexcept {
						std::size_t h = 0;
						std::hash<int> hash;
						for (auto c : str)
							h ^= hash(tolower(c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
						return h;
					}
				};

				using CaseInsensitiveMultimap = std::unordered_multimap<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;

				class Percent {
				public:
					/// Returns percent-encoded string
					static std::string encode(const std::string &value) noexcept {
						static auto hex_chars = "0123456789ABCDEF";

						std::string result;
						result.reserve(value.size()); // Minimum size of result

						for (auto &chr : value) {
							if (chr == ' ')
								result += '+';
							else if (chr == '!' || chr == '#' || chr == '$' || (chr >= '&' && chr <= ',') || (chr >= '/' && chr <= ';') || chr == '=' || chr == '?' || chr == '@' || chr == '[' || chr == ']')
								result += std::string("%") + hex_chars[chr >> 4] + hex_chars[chr & 15];
							else
								result += chr;
						}

						return result;
					}

					/// Returns percent-decoded string
					static std::string decode(const std::string &value) noexcept {
						std::string result;
						result.reserve(value.size() / 3 + (value.size() % 3)); // Minimum size of result

						for (std::size_t i = 0; i < value.size(); ++i) {
							auto &chr = value[i];
							if (chr == '%' && i + 2 < value.size()) {
								auto hex = value.substr(i + 1, 2);
								auto decoded_chr = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
								result += decoded_chr;
								i += 2;
							}
							else if (chr == '+')
								result += ' ';
							else
								result += chr;
						}

						return result;
					}
				};

				class QueryString {
				public:
					/// Returns query string created from given field names and values
					static std::string create(const CaseInsensitiveMultimap &fields) noexcept {
						std::string result;

						bool first = true;
						for (auto &field : fields) {
							result += (!first ? "&" : "") + field.first + '=' + Percent::encode(field.second);
							first = false;
						}

						return result;
					}

					/// Returns query keys with percent-decoded values.
					static CaseInsensitiveMultimap parse(const std::string &query_string) noexcept {
						CaseInsensitiveMultimap result;

						if (query_string.empty())
							return result;

						std::size_t name_pos = 0;
						auto name_end_pos = std::string::npos;
						auto value_pos = std::string::npos;
						for (std::size_t c = 0; c < query_string.size(); ++c) {
							if (query_string[c] == '&') {
								auto name = query_string.substr(name_pos, (name_end_pos == std::string::npos ? c : name_end_pos) - name_pos);
								if (!name.empty()) {
									auto value = value_pos == std::string::npos ? std::string() : query_string.substr(value_pos, c - value_pos);
									result.emplace(std::move(name), Percent::decode(value));
								}
								name_pos = c + 1;
								name_end_pos = std::string::npos;
								value_pos = std::string::npos;
							}
							else if (query_string[c] == '=') {
								name_end_pos = c;
								value_pos = c + 1;
							}
						}
						if (name_pos < query_string.size()) {
							auto name = query_string.substr(name_pos, name_end_pos - name_pos);
							if (!name.empty()) {
								auto value = value_pos >= query_string.size() ? std::string() : query_string.substr(value_pos);
								result.emplace(std::move(name), Percent::decode(value));
							}
						}

						return result;
					}
				};

				class HttpHeader {
				public:
					/// Parse header fields
					static DC::Web::http::headers parse(std::istream &stream) noexcept {
						DC::Web::http::headers result;
						std::string line;
						getline(stream, line);
						std::size_t param_end;
						while ((param_end = line.find(':')) != std::string::npos) {
							std::size_t value_start = param_end + 1;
							if (value_start < line.size()) {
								if (line[value_start] == ' ')
									value_start++;
								if (value_start < line.size())
									result.add(DC::Web::http::add_header(line.substr(0, param_end), line.substr(value_start, line.size() - value_start - 1)));
							}

							getline(stream, line);
						}
						return result;
					}
				};

				class ResponseMessage {
				public:
					/// Parse status line and header fields
					static bool parse(std::istream &stream, std::string &version, std::string &status_code, DC::Web::http::headers &header) noexcept {
						header.clear();
						std::string line;
						getline(stream, line);
						std::size_t version_end = line.find(' ');
						if (version_end != std::string::npos) {
							if (5 < line.size())
								version = line.substr(5, version_end - 5);
							else
								return false;
							if ((version_end + 1) < line.size())
								status_code = line.substr(version_end + 1, line.size() - (version_end + 1) - 1);
							else
								return false;

							header = HttpHeader::parse(stream);
						}
						else
							return false;
						return true;
					}
				};

				class ContentDisposition {
				public:
					/// Can be used to parse the Content-Disposition header field value when
					/// clients are posting requests with enctype="multipart/form-data"
					static CaseInsensitiveMultimap parse(const std::string &line) {
						CaseInsensitiveMultimap result;

						std::size_t para_start_pos = 0;
						std::size_t para_end_pos = std::string::npos;
						std::size_t value_start_pos = std::string::npos;
						for (std::size_t c = 0; c < line.size(); ++c) {
							if (para_start_pos != std::string::npos) {
								if (para_end_pos == std::string::npos) {
									if (line[c] == ';') {
										result.emplace(line.substr(para_start_pos, c - para_start_pos), std::string());
										para_start_pos = std::string::npos;
									}
									else if (line[c] == '=')
										para_end_pos = c;
								}
								else {
									if (value_start_pos == std::string::npos) {
										if (line[c] == '"' && c + 1 < line.size())
											value_start_pos = c + 1;
									}
									else if (line[c] == '"') {
										result.emplace(line.substr(para_start_pos, para_end_pos - para_start_pos), line.substr(value_start_pos, c - value_start_pos));
										para_start_pos = std::string::npos;
										para_end_pos = std::string::npos;
										value_start_pos = std::string::npos;
									}
								}
							}
							else if (line[c] != ' ' && line[c] != ';')
								para_start_pos = c;
						}
						if (para_start_pos != std::string::npos && para_end_pos == std::string::npos)
							result.emplace(line.substr(para_start_pos), std::string());

						return result;
					}
				};

				class ScopeRunner {
					/// Scope count that is set to -1 if scopes are to be canceled
					std::atomic<long> count;

				public:
					ScopeRunner() noexcept : count(0) {}

					class SharedLock {
						friend class ScopeRunner;
						std::atomic<long> &count;
						SharedLock(std::atomic<long> &count) noexcept : count(count) {}
						SharedLock &operator=(const SharedLock &) = delete;
						SharedLock(const SharedLock &) = delete;

					public:
						~SharedLock() noexcept {
							count.fetch_sub(1);
						}
					};

				public:
					std::unique_ptr<SharedLock> continue_lock() noexcept {
						long expected = count;
						while (expected >= 0 && !count.compare_exchange_weak(expected, expected + 1))
							spin_loop_pause();

						if (expected < 0)
							return nullptr;
						else
							return std::unique_ptr<SharedLock>(new SharedLock(count));
					}

					void stop() noexcept {
						long expected = 0;
						while (!count.compare_exchange_weak(expected, -1)) {
							if (expected < 0)
								return;
							expected = 0;
							spin_loop_pause();
						}
					}
				};

				template <class socket_type>
				class Server;

				template <class socket_type>
				class ServerBase {
				protected:
					class Session;

				public:
					class Response : public std::enable_shared_from_this<Response>, public std::ostream {
						friend class ServerBase<socket_type>;
						friend class Server<socket_type>;

						asio::streambuf streambuf;

						std::shared_ptr<Session> session;
						long timeout_content;

						Response(std::shared_ptr<Session> session, long timeout_content) noexcept : std::ostream(&streambuf), session(std::move(session)), timeout_content(timeout_content) {}

						template <typename size_type>
						void write_header(const CaseInsensitiveMultimap &header, size_type size) {
							bool content_length_written = false;
							bool chunked_transfer_encoding = false;
							for (auto &field : header) {
								if (!content_length_written && case_insensitive_equal(field.first, "content-length"))
									content_length_written = true;
								else if (!chunked_transfer_encoding && case_insensitive_equal(field.first, "transfer-encoding") && case_insensitive_equal(field.second, "chunked"))
									chunked_transfer_encoding = true;

								*this << field.first << ": " << field.second << "\r\n";
							}
							if (!content_length_written && !chunked_transfer_encoding && !close_connection_after_response)
								*this << "Content-Length: " << size << "\r\n\r\n";
							else
								*this << "\r\n";
						}

					public:
						/// Use this function if you need to recursively send parts of a longer message
						void send(const std::function<void(const error_code &)> &callback = nullptr) noexcept {
							session->connection->set_timeout(timeout_content);
							auto self = this->shared_from_this(); // Keep Response instance alive through the following async_write
							asio::async_write(*session->connection->socket, streambuf, [self, callback](const error_code &ec, std::size_t /*bytes_transferred*/) {
								self->session->connection->cancel_timeout();
								auto lock = self->session->connection->handler_runner->continue_lock();
								if (!lock)
									return;
								if (callback)
									callback(ec);
							});
						}

						/// Write directly to stream buffer using std::ostream::write
						void write(const char_type *ptr, std::streamsize n) {
							std::ostream::write(ptr, n);
						}

						/// Convenience function for writing status line, potential header fields, and empty content
						void write(DC::Web::http::status_code status_code = DC::Web::http::status_codes::OK, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
							*this << "HTTP/1.1 " << status_code << "\r\n";
							write_header(header, 0);
						}

						/// Convenience function for writing status line, header fields, and content
						void write(DC::Web::http::status_code status_code, const std::string &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
							*this << "HTTP/1.1 " << status_code << "\r\n";
							write_header(header, content.size());
							if (!content.empty())
								*this << content;
						}

						void write(const DC::Web::http::response& response) {
							*this << response.toStr();
						}

						/// Convenience function for writing status line, header fields, and content
						void write(DC::Web::http::status_code status_code, std::istream &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
							*this << "HTTP/1.1 " << status_code << "\r\n";
							content.seekg(0, std::ios::end);
							auto size = content.tellg();
							content.seekg(0, std::ios::beg);
							write_header(header, size);
							if (size)
								*this << content.rdbuf();
						}

						/// Convenience function for writing success status line, header fields, and content
						void write(const std::string &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
							write(DC::Web::http::status_codes::OK, content, header);
						}

						/// Convenience function for writing success status line, header fields, and content
						void write(std::istream &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
							write(DC::Web::http::status_codes::OK, content, header);
						}

						/// Convenience function for writing success status line, and header fields
						void write(const CaseInsensitiveMultimap &header) {
							write(DC::Web::http::status_codes::OK, std::string(), header);
						}

						std::size_t size() noexcept {
							return streambuf.size();
						}

						/// If true, force server to close the connection after the response have been sent.
						///
						/// This is useful when implementing a HTTP/1.0-server sending content
						/// without specifying the content length.
						bool close_connection_after_response = false;
					};

					class Content : public std::istream {
						friend class ServerBase<socket_type>;

					public:
						std::size_t size() noexcept {
							return streambuf.size();
						}
						/// Convenience function to return std::string. The stream buffer is consumed.
						std::string string() noexcept {
							try {
								std::stringstream ss;
								ss << rdbuf();
								return ss.str();
							}
							catch (...) {
								return std::string();
							}
						}

					private:
						asio::streambuf &streambuf;
						Content(asio::streambuf &streambuf) noexcept : std::istream(&streambuf), streambuf(streambuf) {}
					};

					class Request {
						friend class ServerBase<socket_type>;
						friend class Server<socket_type>;
						friend class Session;

						asio::streambuf streambuf;
						Request(std::size_t max_request_streambuf_size, const std::string &remote_endpoint_address = std::string(), unsigned short remote_endpoint_port = 0) noexcept
							: streambuf(max_request_streambuf_size), content(streambuf), remote_endpoint_address(remote_endpoint_address), remote_endpoint_port(remote_endpoint_port) {}

					public:
						DC::Web::http::request request;
						Content content;
						regex::smatch path_match;
						std::string remote_endpoint_address;
						unsigned short remote_endpoint_port;
					};

				protected:
					class Connection : public std::enable_shared_from_this<Connection> {
					public:
						template <typename... Args>
						Connection(std::shared_ptr<ScopeRunner> handler_runner, Args &&... args) noexcept : handler_runner(std::move(handler_runner)), socket(new socket_type(std::forward<Args>(args)...)) {}

						std::shared_ptr<ScopeRunner> handler_runner;

						std::unique_ptr<socket_type> socket; // Socket must be unique_ptr since asio::ssl::stream<asio::ip::tcp::socket> is not movable
															 //std::mutex socket_close_mutex;

						std::unique_ptr<asio::steady_timer> timer;

						void close() noexcept {
							error_code ec;
							//std::unique_lock<std::mutex> lock(socket_close_mutex); // The following operations seems to be needed to run sequentially
							socket->lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
							socket->lowest_layer().close(ec);
						}

						void set_timeout(long seconds) noexcept {
							if (seconds == 0) {
								timer = nullptr;
								return;
							}

							timer = std::unique_ptr<asio::steady_timer>(new asio::steady_timer(socket->get_io_service()));
							timer->expires_from_now(std::chrono::seconds(seconds));
							auto self = this->shared_from_this();
							timer->async_wait([self](const error_code &ec) {
								if (!ec)
									self->close();
							});
						}

						void cancel_timeout() noexcept {
							if (timer) {
								error_code ec;
								timer->cancel(ec);
							}
						}
					};

					class Session {
					public:
						Session(std::size_t max_request_streambuf_size, std::shared_ptr<Connection> connection) noexcept : connection(std::move(connection)) {
							try {
								auto remote_endpoint = this->connection->socket->lowest_layer().remote_endpoint();
								request = std::shared_ptr<Request>(new Request(max_request_streambuf_size, remote_endpoint.address().to_string(), remote_endpoint.port()));
							}
							catch (...) {
								request = std::shared_ptr<Request>(new Request(max_request_streambuf_size));
							}
						}

						std::shared_ptr<Connection> connection;
						std::shared_ptr<Request> request;
					};

				public:
					class Config {
						friend class ServerBase<socket_type>;

					public:
						Config(unsigned short port) noexcept : port(port) {}
					public:
						/// Port number to use. Defaults to 80 for HTTP and 443 for HTTPS.
						unsigned short port;
						/// If io_service is not set, number of threads that the server will use when start() is called.
						/// Defaults to 1 thread.
						std::size_t thread_pool_size = 1;
						/// Timeout on request handling. Defaults to 5 seconds.
						long timeout_request = 5;
						/// Timeout on content handling. Defaults to 300 seconds.
						long timeout_content = 300;
						/// Maximum size of request stream buffer. Defaults to architecture maximum.
						/// Reaching this limit will result in a message_size error code.
						std::size_t max_request_streambuf_size = std::numeric_limits<std::size_t>::max();
						/// IPv4 address in dotted decimal form or IPv6 address in hexadecimal notation.
						/// If empty, the address will be any address.
						std::string address;
						/// Set to false to avoid binding the socket to an address that is already in use. Defaults to true.
						bool reuse_address = true;
					};
					/// Set before calling start().
					Config internal_config;

				private:
					class regex_orderable : public regex::regex {
						std::string str;

					public:
						regex_orderable(const char *regex_cstr) : regex::regex(regex_cstr), str(regex_cstr) {}
						regex_orderable(std::string regex_str) : regex::regex(regex_str), str(std::move(regex_str)) {}
						bool operator<(const regex_orderable &rhs) const noexcept {
							return str < rhs.str;
						}
					};

				public:
					/// Warning: do not add or remove resources after start() is called
					std::map<regex_orderable, std::map<std::string, std::function<void(std::shared_ptr<typename ServerBase<socket_type>::Response>, std::shared_ptr<typename ServerBase<socket_type>::Request>)>>> resource;

					std::map<std::string, std::function<void(std::shared_ptr<typename ServerBase<socket_type>::Response>, std::shared_ptr<typename ServerBase<socket_type>::Request>)>> default_resource;

					std::function<void(std::shared_ptr<typename ServerBase<socket_type>::Request>, const error_code &)> on_error;

					std::function<void(std::unique_ptr<socket_type> &, std::shared_ptr<typename ServerBase<socket_type>::Request>)> on_upgrade;

					/// If you have your own asio::io_service, store its pointer here before running start().
					std::shared_ptr<asio::io_service> io_service;

					virtual void start() {
						if (!io_service) {
							io_service = std::make_shared<asio::io_service>();
							internal_io_service = true;
						}

						if (io_service->stopped())
							io_service->reset();

						asio::ip::tcp::endpoint endpoint;
						if (internal_config.address.size() > 0)
							endpoint = asio::ip::tcp::endpoint(asio::ip::address::from_string(internal_config.address), internal_config.port);
						else
							endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), internal_config.port);

						if (!acceptor)
							acceptor = std::unique_ptr<asio::ip::tcp::acceptor>(new asio::ip::tcp::acceptor(*io_service));
						acceptor->open(endpoint.protocol());
						acceptor->set_option(asio::socket_base::reuse_address(internal_config.reuse_address));
						acceptor->bind(endpoint);
						acceptor->listen();

						accept();

						if (internal_io_service) {
							// If thread_pool_size>1, start m_io_service.run() in (thread_pool_size-1) threads for thread-pooling
							threads.clear();
							for (std::size_t c = 1; c < internal_config.thread_pool_size; c++) {
								threads.emplace_back([this]() {
									this->io_service->run();
								});
							}

							// Main thread
							if (internal_config.thread_pool_size > 0)
								io_service->run();

							// Wait for the rest of the threads, if any, to finish as well
							for (auto &t : threads)
								t.join();
						}
					}

					/// Stop accepting new requests, and close current connections.
					void stop() noexcept {
						if (acceptor) {
							error_code ec;
							acceptor->close(ec);

							{
								std::unique_lock<std::mutex> lock(*connections_mutex);
								for (auto &connection : *connections)
									connection->close();
								connections->clear();
							}

							if (internal_io_service)
								io_service->stop();
						}
					}

					virtual ~ServerBase() noexcept {
						handler_runner->stop();
						stop();
					}

				protected:
					bool internal_io_service = false;

					std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
					std::vector<std::thread> threads;

					std::shared_ptr<std::unordered_set<Connection *>> connections;
					std::shared_ptr<std::mutex> connections_mutex;

					std::shared_ptr<ScopeRunner> handler_runner;

					ServerBase(unsigned short port) noexcept : internal_config(port), connections(new std::unordered_set<Connection *>()), connections_mutex(new std::mutex()), handler_runner(new ScopeRunner()) {}

					virtual void accept() = 0;

					template <typename... Args>
					std::shared_ptr<Connection> create_connection(Args &&... args) noexcept {
						auto connections = this->connections;
						auto connections_mutex = this->connections_mutex;
						auto connection = std::shared_ptr<Connection>(new Connection(handler_runner, std::forward<Args>(args)...), [connections, connections_mutex](Connection *connection) {
							{
								std::unique_lock<std::mutex> lock(*connections_mutex);
								auto it = connections->find(connection);
								if (it != connections->end())
									connections->erase(it);
							}
							delete connection;
						});
						{
							std::unique_lock<std::mutex> lock(*connections_mutex);
							connections->emplace(connection.get());
						}
						return connection;
					}

					void read_request_and_content(const std::shared_ptr<Session> &session) {
						session->connection->set_timeout(internal_config.timeout_request);
						asio::async_read_until(*session->connection->socket, session->request->streambuf, "\r\n\r\n", [this, session](const error_code &ec, std::size_t bytes_transferred) {
							session->connection->cancel_timeout();
							auto lock = session->connection->handler_runner->continue_lock();
							if (!lock)
								return;
							if ((!ec || ec == asio::error::not_found) && session->request->streambuf.size() == session->request->streambuf.max_size()) {
								auto response = std::shared_ptr<Response>(new Response(session, this->internal_config.timeout_content));
								response->write(DC::Web::http::status_codes::BadRequest);
								response->send();
								if (this->on_error)
									this->on_error(session->request, make_error_code::make_error_code(errc::message_size));
								return;
							}
							if (!ec) {
								// request->streambuf.size() is not necessarily the same as bytes_transferred, from Boost-docs:
								// "After a successful async_read_until operation, the streambuf may contain additional data beyond the delimiter"
								// The chosen solution is to extract lines from the stream directly when parsing the header. What is left of the
								// streambuf (maybe some bytes of the content) is appended to in the async_read-function below (for retrieving content).
								std::size_t num_additional_bytes = session->request->streambuf.size() - bytes_transferred;

								try {
									session->request->request = DC::Web::http::request_deserialization(session->request->content.string());
									for (auto &p : session->request->request.headers())
										p.SetName(DC::STR::toLower(p.GetName()));
								}
								catch (...) {
									auto response = std::shared_ptr<Response>(new Response(session, this->internal_config.timeout_content));
									response->write(DC::Web::http::status_codes::BadRequest);
									response->send();
									if (this->on_error)
										this->on_error(session->request, make_error_code::make_error_code(errc::protocol_error));
									return;
								}

								// If content, read that as well
								DC::Web::http::httpSpace::header it;
								try {
									it = session->request->request.headers().get_header("content-length");
									unsigned long long content_length = 0;
									try {
										content_length = stoull(it.GetValue());
									}
									catch (const std::exception) {
										if (this->on_error)
											this->on_error(session->request, make_error_code::make_error_code(errc::protocol_error));
										return;
									}
									if (content_length > num_additional_bytes) {
										session->connection->set_timeout(internal_config.timeout_content);
										asio::async_read(*session->connection->socket, session->request->streambuf, asio::transfer_exactly(content_length - num_additional_bytes), [this, session](const error_code &ec, std::size_t /*bytes_transferred*/) {
											session->connection->cancel_timeout();
											auto lock = session->connection->handler_runner->continue_lock();
											if (!lock)
												return;
											if (!ec) {
												if (session->request->streambuf.size() == session->request->streambuf.max_size()) {
													auto response = std::shared_ptr<Response>(new Response(session, this->internal_config.timeout_content));
													response->write(DC::Web::http::status_codes::BadRequest);
													response->send();
													if (this->on_error)
														this->on_error(session->request, make_error_code::make_error_code(errc::message_size));
													return;
												}
												this->find_resource(session);
											}
											else if (this->on_error)
												this->on_error(session->request, ec);
										});
									}
									else
										this->find_resource(session);
								}
								catch (...) {
									try {
										this->find_resource(session);
									}
									catch (...) {
										if (this->on_error)
											this->on_error(session->request, ec);
									}
								}
							}
						});
					}

					void find_resource(const std::shared_ptr<Session> &session) {
						// Upgrade connection
						if (on_upgrade) {
							auto it = session->request->request.headers().get_header("upgrade");
							{
								std::unique_lock<std::mutex> lock(*connections_mutex);
								auto it = connections->find(session->connection.get());
								if (it != connections->end())
									connections->erase(it);
							}

							on_upgrade(session->connection->socket, session->request);
							return;
						}
						// Find path- and method-match, and call write_response
						for (auto &regex_method : resource) {
							auto it = regex_method.second.find(session->request->request.get_method());
							if (it != regex_method.second.end()) {
								regex::smatch sm_res;
								std::string path = session->request->request.get_uri();
								if (regex::regex_match(path, sm_res, regex_method.first)) {
									session->request->path_match = std::move(sm_res);
									write_response(session, it->second);
									return;
								}
							}
						}
						auto it = default_resource.find(session->request->request.get_method());
						if (it != default_resource.end())
							write_response(session, it->second);
					}

					void write_response(const std::shared_ptr<Session> &session,
						std::function<void(std::shared_ptr<typename ServerBase<socket_type>::Response>, std::shared_ptr<typename ServerBase<socket_type>::Request>)> &resource_function) {
						session->connection->set_timeout(internal_config.timeout_content);
						auto response = std::shared_ptr<Response>(new Response(session, internal_config.timeout_content), [this](Response *response_ptr) {
							auto response = std::shared_ptr<Response>(response_ptr);
							response->send([this, response](const error_code &ec) {
								if (!ec) {
									if (response->close_connection_after_response)
										return;

									std::string connection_value;
									try {										
										connection_value = response->session->request->request.headers().get_value("connection");
										connection_value = DC::STR::toLower(connection_value);

										if (connection_value == "close")
											return;
										if (connection_value == "keep-alive") {
											auto new_session = std::make_shared<Session>(this->internal_config.max_request_streambuf_size, response->session->connection);
											this->read_request_and_content(new_session);
											return;
										}
									}
									catch (...) {
										if (response->session->request->request.get_version() >= "1.1") {
											auto new_session = std::make_shared<Session>(this->internal_config.max_request_streambuf_size, response->session->connection);
											this->read_request_and_content(new_session);
											return;
										}
									}
								}
								else if (this->on_error)
									this->on_error(response->session->request, ec);
							});
						});

						try {
							resource_function(response, session->request);
						}
						catch (const std::exception &) {
							if (on_error)
								on_error(session->request, make_error_code::make_error_code(errc::operation_canceled));
							return;
						}
					}
				};

				template <class socket_type>
				class Server : public ServerBase<socket_type> {};

				using HTTP = asio::ip::tcp::socket;

				template <>
				class Server<HTTP> : public ServerBase<HTTP> {
				public:
					Server() noexcept : ServerBase<HTTP>::ServerBase(80) {}

				protected:
					void accept() override {
						auto session = std::make_shared<Session>(internal_config.max_request_streambuf_size, create_connection(*io_service));

						acceptor->async_accept(*session->connection->socket, [this, session](const error_code &ec) {
							auto lock = session->connection->handler_runner->continue_lock();
							if (!lock)
								return;

							// Immediately start accepting a new connection (unless io_service has been stopped)
							if (ec != asio::error::operation_aborted)
								this->accept();

							if (!ec) {
								asio::ip::tcp::no_delay option(true);
								error_code ec;
								session->connection->socket->set_option(option, ec);

								this->read_request_and_content(session);
							}
							else if (this->on_error)
								this->on_error(session->request, ec);
						});
					}
				};

				template <typename server_type>
				struct http_server_base :private serverSpace::Server<server_type> {
					std::string server_name = "liuzianglib_http_server";
					serverSpace::Server<server_type>::Config& config = this->internal_config;

					template <typename ...ARGS>
					http_server_base(ARGS&&... args) :serverSpace::Server<server_type>(std::forward<ARGS>(args)...) {}

					virtual void start()override {
						serverSpace::Server<server_type>::start();
					}

					void stop()noexcept {
						serverSpace::Server<server_type>::stop();
					}

					void set_resource(const std::string& regular, const std::string& method, const std::function<DC::Web::http::response(const DC::Web::http::request&)>& callback) {
						this->resource[regular][method] = std::bind(&http_server_base::resource_forward, this, std::placeholders::_1, std::placeholders::_2, callback);
					}

					void set_default_resource(const std::string& method, const std::function<DC::Web::http::response(const DC::Web::http::request&)>& callback) {
						this->default_resource[method] = std::bind(&http_server_base::resource_forward, this, std::placeholders::_1, std::placeholders::_2, callback);
					}

					void set_on_error(const std::function<void(const DC::Web::http::request&)>& callback) {
						this->on_error = [callback](std::shared_ptr<Request> request, const boost::system::error_code&) {
							callback(request->request);
						};
					}

				private:
					void resource_forward(std::shared_ptr<serverSpace::Server<server_type>::Response> response, std::shared_ptr<serverSpace::Server<server_type>::Request> request, const std::function<DC::Web::http::response(const DC::Web::http::request&)>& callback) {
						auto http_response = callback(request->request);

						if (!http_response.headers().has_key("Content-Length"))
							http_response.headers().add(DC::Web::http::add_header("Content-Length", DC::STR::toString(http_response.body().size())));
						if (!http_response.headers().has_key("Connection"))
							http_response.headers().add(DC::Web::http::add_header("Connection", "keep-alive"));
						if (!http_response.headers().has_key("Server"))
							http_response.headers().add(DC::Web::http::add_header("Server", this->server_name));

						response->write(http_response);
					}
				};

			}

			class http_server final :public serverSpace::http_server_base<serverSpace::HTTP> {};

			/*
			resource function: DC::Web::http::response(const DC::Web::http::request&)
			on_error function: void(const DC::Web::http::request&)
			*/

		}

	}

}

#endif
