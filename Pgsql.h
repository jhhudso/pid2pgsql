/*
 * Copyright (C) 2014,2019 Jared H. Hudson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef PGSQL_H_
#define PGSQL_H_

extern "C" {
#include <pgsql/libpq-fe.h>
#include <pgsql/sql3types.h>
#include <stdint.h>
#include <pgsql/postgres_ext.h>
}

#include <vector>
#include <map>
#include <queue>

using namespace std;

typedef vector<pair<string, string> > PrepareVector;

class Prepare {
private:
	string generateInsertQuery(void);
	string generateUpdateQuery(void);
	static map<string, PGresult *> existing_prepares;
	PGconn *conn;

	string tableName;
	string prepareID;
	string whereString;
	string whereData;
	PrepareVector current_prepare;
	bool debug;

public:
	Prepare(PGconn *conn, string prepare_id, bool debug_value = false);
	~Prepare();
	void setTableName(string tableName);
	void setConn(PGconn *conn);
	void setPrepareID(string prepareID);
	void addCol(string colName, char * value);
	void addCol(string colName, string &value);
	void addCol(string colName, char value);
	void addCol(string colName, uint64_t value);
	void addCol(string colName, int64_t value);
	void addCol(string colName, uint32_t value);
	void addCol(string colName, int32_t value);
	void where(string ws, int wd);
	void where(string ws, char * wd);
	void exec(void);
	Oid getResult(void);
	void enableDebug(void);
	void disableDebug(void);
	bool getDebug(void);
	PGresult *lastResult;

	// Exceptions
	class emptyName {
	};
	class emptyPrepareID {
	};

	friend int main(int argc, char *argv[]);
};


class Pgsql {
private:
	static PGconn *conn;
	queue<Prepare> execqueue;
	bool debug;
public:
	Pgsql(const char dbhost[], const char dbname[], const char dbuser[], const char dbpass[], const bool debug_value);
	Prepare createPrepare(string prepare_id);
	void begin(void);
	void commit(void);
	void processqueue(void);
	uint64_t lastval(void);
	void enableDebug(void);
	void disableDebug(void);
	bool getDebug(void);
	virtual ~Pgsql();

	// Exceptions
	class Error {};
};



#endif /* PGSQL_H_ */
