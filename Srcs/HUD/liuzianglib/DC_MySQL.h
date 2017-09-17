#pragma once
#ifndef liuzianglib_MySQL
#define liuzianglib_MySQL
#include <vector>
#include <mysql.h>
//Version 2.4.21V30
//20170914
//注意:DC_WinSock.h必须先于DC_MySQL.h包含

namespace DC {

	class DC_MySQL {//拷贝构造函数和=运算符仅仅是拷贝了服务器参数，而不是连接句柄，拷贝之后需要重新连接到服务器；
	public:
		DC_MySQL() {
			mysql_init(&SQL);
		}

		~DC_MySQL() {
			mysql_close(&SQL);
		}

		DC_MySQL(const DC_MySQL& input) {
			mysql_init(&SQL);
			IP = input.IP;
			username = input.username;
			password = input.password;
			DBname = input.DBname;
			port = input.port;
		}

	public:
		const DC_MySQL& operator=(const DC_MySQL& input) {
			mysql_close(&SQL);
			mysql_init(&SQL);
			IP = input.IP;
			username = input.username;
			password = input.password;
			DBname = input.DBname;
			port = input.port;
			return *this;
		}

		void SetServer(const std::string& inputIP, const std::string& inputusername, const std::string& inputpassword, const std::string& inputDBname, const std::size_t& inputport) {
			IP = inputIP;
			username = inputusername;
			password = inputpassword;
			DBname = inputDBname;
			port = inputport;
		}

		bool VerConnection() {//判断连接是否可用。
							  //如果有未取回的结果也会返回连接不可用
			if (mysql_ping(&SQL) == 0) return true;
			return false;
		}

		bool Connect() {
			mysql_close(&SQL);
			mysql_init(&SQL);
			if (mysql_real_connect(&SQL, IP.c_str(), username.c_str(), password.c_str(), DBname.c_str(), port, NULL, 0) == NULL) return false;
			return true;
		}

		void disconnect() {
			mysql_close(&SQL);
		}

		bool Query(const std::string& que) {//如果查询语句有结果，而且此结果没有通过GetResult()获取，那么会无法再次执行类似insert这样“有结果的语句”。直到你通过GetResult()把之前滞留的结果取走了，才能再次执行“有结果的语句”。
			if (mysql_query(&SQL, que.c_str()) == 0) return true;
			return false;
		}

		std::vector<std::string> GetResult() {
			MYSQL_RES *SQL_RES;
			MYSQL_ROW SQL_ROW;
			std::vector<std::string> result;
			if ((SQL_RES = mysql_store_result(&SQL)) == NULL) { mysql_free_result(SQL_RES); return result; }
			while ((SQL_ROW = mysql_fetch_row(SQL_RES)) != NULL) {
				for (std::size_t i = 0; i < mysql_num_fields(SQL_RES); i++)
					result.push_back(SQL_ROW[i]);
			}
			mysql_free_result(SQL_RES);
			return result;
		}

		MYSQL* GetHandle() {
			return &SQL;
		}

	private:
		MYSQL SQL;
		std::string IP, username, password, DBname;
		std::size_t port;
	};

}

#endif
