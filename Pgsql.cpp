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

extern "C" {
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
}

#include <sstream>
#include <iostream>
#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/fips.h>
#include "Pgsql.h"
#include "Clock.h"

using namespace std;

PGconn *Pgsql::conn = NULL;
map<string, PGresult *> Prepare::existing_prepares;

Pgsql::Pgsql(const char dbhost[], const char dbname[], const char dbuser[],
		const char dbpass[], bool debug_value) {
	debug = debug_value;

	stringstream ss;
	ss << "host=" << dbhost << " dbname=" << dbname << " user=" << dbuser
			<< " password=" << dbpass << " sslmode=verify-full"
			<< " sslrootcert=server.crt";

	if (conn == NULL || PQstatus(conn) != CONNECTION_OK) {
		conn = PQconnectdb(ss.str().c_str());
		if (PQstatus(conn) != CONNECTION_OK) {
			stringstream errormsg;
			cerr << PQerrorMessage(conn) << endl;
			throw new Error();
		}

		if (PQsetnonblocking(conn, 1) == -1) {
			cerr << "Unable to set pgsql connection non-blocking\n"
					<< PQerrorMessage(conn) << endl;
		}
	}

}

Pgsql::~Pgsql() {
	PQfinish(conn);
}
void Pgsql::enableDebug(void) {
	debug = true;
}

void Pgsql::disableDebug(void) {
	debug = false;
}

void Pgsql::processqueue(void) {
	PQconsumeInput(conn);

	if (PQisBusy(conn) == false) {
		PGresult *res = PQgetResult(conn);
		if (res == NULL) {
			// We're ready to process a new SQL statement

			if (execqueue.size() > 0) {
				Prepare p = execqueue.front();
				execqueue.pop();

				p.exec();
			}
		} else {
			int status = PQresultStatus(res);
			if (status != PGRES_COMMAND_OK) {
				cerr << "Error in result found in pgsql::processqueue(): "
						<< status << endl;
			}

			PQclear(res);
		}
	}
}

uint64_t Pgsql::lastval(void) {
	PGresult *res = PQexec(conn, "SELECT lastval()");
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		cerr << "Error occurred: " << PQresultErrorMessage(res) << endl;
		PQclear(res);
		return EXIT_FAILURE;
	}

	if (PQntuples(res) < 1) {
		cerr << "Error: PQntuples(res) < 1" << endl;
		PQclear(res);
		return EXIT_FAILURE;
	}

	char *buffer = PQgetvalue(res, 0, 0);
	if (buffer == NULL || PQgetlength(res, 0, 0) < 1) {
		cerr
				<< "Error occurred getting set_id from pid_sets table. Buffer == NULL or PQgetlength(res, 0, 0) < 1"
				<< endl;
		PQclear(res);
		return 0;
	}

	uint64_t value = atoi(buffer);
	PQclear(res);

	return value;
}

Prepare Pgsql::createPrepare(string prepareID) {
return Prepare(Pgsql::conn, prepareID, debug);
}

void Pgsql::begin(void) {
	// if query fails to send
	if (PQsendQuery(conn, "BEGIN") == 0) {
		cerr << "error: failed to send BEGIN;" << endl;
		cerr << PQerrorMessage(conn);
	}
}

void Pgsql::commit(void) {
	if (PQconsumeInput(conn) == 0) {
		cerr << "error: failed to PQconsumeinput while in commit()" << endl;
		cerr << PQerrorMessage(conn);
	}

	int8_t rc = 0;
	while ((rc = PQflush(conn)) == 1) {
		if (debug) {
			cerr << "PQflush in commit() had to wait" << endl;
		}
		usleep(100);
	}

	// PQflush: 0 == successful
	if (rc != 0) {
		cerr << "error: failed to PQflush while in commit()" << endl;
		cerr << PQerrorMessage(conn);
	}

	PGresult *res;
	while ((res = PQgetResult(conn)) != NULL) {
		PQclear(res);
	}

	// if query fails to send
	if (PQsendQuery(conn, "COMMIT") == 0) {
		cerr << "error: failed to send COMMIT;" << endl;
		cerr << PQerrorMessage(conn);
	}
}


Prepare::Prepare(PGconn *conn, string prepareID, bool debug_value) :
		conn(conn), prepareID(prepareID), debug(debug_value), lastResult(0)  {

}

void Prepare::setTableName(string tableName) {
	this->tableName = tableName;
}

void Prepare::setConn(PGconn *conn) {
	this->conn = conn;
}

void Prepare::setPrepareID(string prepareID) {
	this->prepareID = prepareID;
}

void Prepare::addCol(string colName, int32_t value) {
	if (tableName.empty())
		throw emptyName();

	if (prepareID.empty())
		throw emptyPrepareID();

	stringstream ss;
	ss << value;

	current_prepare.push_back(make_pair(colName, ss.str().c_str()));
}

void Prepare::addCol(string colName, uint32_t value) {
	if (tableName.empty())
		throw emptyName();

	if (prepareID.empty())
		throw emptyPrepareID();

	stringstream ss;
	ss << value;

	current_prepare.push_back(make_pair(colName, ss.str().c_str()));
}

void Prepare::addCol(string colName, char value) {
	if (tableName.empty())
		throw emptyName();

	if (prepareID.empty())
		throw emptyPrepareID();

	stringstream ss;
	ss << value;

	current_prepare.push_back(make_pair(colName, ss.str().c_str()));
}

void Prepare::addCol(string colName, uint64_t value) {
	if (tableName.empty())
		throw emptyName();

	if (prepareID.empty())
		throw emptyPrepareID();

	stringstream ss;
	ss << value;

	current_prepare.push_back(make_pair(colName, ss.str().c_str()));
}

void Prepare::addCol(string colName, int64_t value) {
	if (tableName.empty())
		throw emptyName();

	if (prepareID.empty())
		throw emptyPrepareID();

	stringstream ss;
	ss << value;

	current_prepare.push_back(make_pair(colName, ss.str().c_str()));
}

void Prepare::addCol(string colName, char *value) {
	if (tableName.empty())
		throw emptyName();

	if (prepareID.empty())
		throw emptyPrepareID();

	stringstream ss;
	ss << value;

	current_prepare.push_back(make_pair(colName, ss.str().c_str()));
}

void Prepare::addCol(string colName, string &value) {
	if (tableName.empty())
		throw emptyName();

	if (prepareID.empty())
		throw emptyPrepareID();

	current_prepare.push_back(make_pair(colName, value));
}

void Prepare::where(string ws, int wd) {
	whereString = ws;

	stringstream ss;
	ss << wd;
	whereData = ss.str();
}

void Prepare::where(string ws, char *wd) {
	whereString = ws;

	stringstream ss;
	ss << wd;
	whereData = ss.str();
}

void Prepare::exec(void) {
	// Number of parameters in the current prepare
	int nParams = current_prepare.size();
	if (whereString.size() > 0) {
		nParams++;
	}

	// Check if prepare exists already
	PGresult *res = Prepare::existing_prepares[prepareID];

	// If not or existing one is bad, create a new one
	if ((res == 0) || (PQresultStatus(res) != PGRES_COMMAND_OK)) {
		// Perform update
		string query;
		if (whereString.size() > 0) {
			query = generateUpdateQuery();
		} else {
			query = generateInsertQuery();
		}

		res = PQprepare(conn, prepareID.c_str(), query.c_str(), nParams, NULL);
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			cerr << "Error occurred trying to prepare SQL: "
					<< PQresultErrorMessage(res) << endl;
			PQclear(res);
			return;
		}

		Prepare::existing_prepares[prepareID] = res;
	}

	const char * *paramValues = new const char *[nParams];
	int i = 0;
	for (PrepareVector::iterator j = current_prepare.begin();
			j != current_prepare.end(); j++) {
		paramValues[i++] = j->second.c_str();
	}

	// Perform update
	if (whereString.size() > 0) {
		paramValues[i] = whereData.c_str();
	}

	if (PQconsumeInput(conn) == 0) {
		cerr << "Error in Prepare::exec() with PQconsumeInput" << endl;
		cerr << PQerrorMessage(conn);
	}

	if (lastResult) {
		PQclear(lastResult);
	}
	lastResult = PQgetResult(conn);
	if (lastResult != NULL && PQresultStatus(lastResult) != PGRES_COMMAND_OK) {
				cerr << "Error occurred trying to prepare SQL: "
						<< PQresultErrorMessage(lastResult) << endl;
	}
	PQclear(lastResult);

	lastResult = PQgetResult(conn);
	while (lastResult != NULL) {
		if (PQresultStatus(lastResult) != PGRES_COMMAND_OK) {
					cerr << "Error occurred trying to prepare SQL: "
							<< PQresultErrorMessage(lastResult) << endl;
		}
		PQclear(lastResult);
		usleep(1);
	}

	//lastResult = res = PQexecPrepared(this->conn, prepareID.c_str(), nParams, paramValues, NULL, NULL, 0);
	int status = PQsendQueryPrepared(this->conn, prepareID.c_str(), nParams,
			paramValues, NULL, NULL, 0);

	if (status == 0) {
		cerr << status << " Error occurred trying to prepare SQL: "
				<< PQerrorMessage(conn) << endl;
	}

	delete[] paramValues;
}

Oid Prepare::getResult(void) {
	Oid o;

	if (lastResult)
		PQclear(lastResult);

	while ((lastResult = PQgetResult(conn)) != NULL) {
		o = PQoidValue(lastResult);
		if (PQresultStatus(lastResult) != PGRES_COMMAND_OK) {
							cerr << "Error occurred trying to prepare SQL: "
									<< PQresultErrorMessage(lastResult) << endl;
				}
		PQclear(lastResult);
		usleep(3);
	}

	return o;
}

string Prepare::generateInsertQuery(void) {
	stringstream sql;
	sql << "INSERT INTO " << tableName << " (";

	PrepareVector::iterator index;
	for (index = current_prepare.begin(); index != current_prepare.end();
			index++) {
		sql << index->first;
		if (index + 1 != current_prepare.end())
			sql << ", ";
	}

	sql << ") VALUES (";
	int numCols = current_prepare.size();
	for (int j = 0; j < numCols; j++) {
		sql << "$" << j + 1;
		if (j + 1 != numCols)
			sql << ", ";
	}

	sql << ")";

	if (debug) {
		cerr << sql.str().c_str() << endl;
	}
	return sql.str().c_str();
}

string Prepare::generateUpdateQuery(void) {
	stringstream sql;
	sql << "UPDATE " << tableName << " SET ";

	int j = 1;
	for (PrepareVector::iterator index = current_prepare.begin();
			index != current_prepare.end(); index++) {
		sql << index->first << " = $" << j++;
		if (index + 1 != current_prepare.end())
			sql << ", ";
	}

	sql << " WHERE " << whereString << " = $" << j;

	if (debug) {
		cerr << sql.str().c_str() << endl;
	}
	return sql.str().c_str();
}

Prepare::~Prepare() {
	if (lastResult)
		PQclear(lastResult);
}
