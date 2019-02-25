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
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/utsname.h>
}

#include <iostream>
#include <vector>
#include <chrono>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

using namespace std::chrono;
using namespace std;
namespace po = boost::program_options;

#include "Pid.h"
#include "Pgsql.h"
#include "Clock.h"

// To setup PostgreSQL database do the following in pgsql as postgres user.
// CREATE ROLE piduser WITH LOGIN PASSWORD 'yter4Fk3';
// CREATE DATABASE piddb;
// \c piddb
// create table pid_sets ( set_id serial primary key, pgserver_time timestamp with time zone DEFAULT CURRENT_TIMESTAMP, node_time timestamp with time zone, nodename text);
// CREATE TABLE pids ( set_id integer references pid_sets, pid INTEGER, comm TEXT, cmdline TEXT, state TEXT, ppid INTEGER, pgrp INTEGER, session INTEGER, tty_nr INTEGER, tpgid INTEGER, flags INTEGER, minflt INTEGER, cminflt INTEGER, majflt INTEGER, cmajflt INTEGER, utime INTEGER, stime INTEGER, cutime INTEGER, priority INTEGER, nice INTEGER, num_threads INTEGER);
// GRANT INSERT ON pids,pid_sets TO piduser;
// grant ALL on pid_sets_set_id_seq TO piduser;
//

int main(int argc, char *argv[]) {
	bool debug = false;
	try {
		po::options_description desc("Allowed options");
		desc.add_options()("help,h", "produce_help_message");
		desc.add_options()("debug,d", "enable debug messages");
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
		if (vm.count("help")) {
			cout << desc << endl;
			return EXIT_SUCCESS;
		}
		if (vm.count("debug")) {
			cerr << "Debug enabled." << endl;
			debug = true;
		}
	} catch(...) {
		return EXIT_FAILURE;
	}

	struct utsname utsbuffer;
	if (uname(&utsbuffer) == -1) {
		perror("uname");
		return EXIT_FAILURE;
	}

	DIR *dir;
	if ((dir = opendir("/proc")) == NULL) {
		perror("opendir");
		return EXIT_FAILURE;
	}

	int readdir_error = 0;
	dirent entry;
	dirent *result = NULL;
	vector<Pid> pids;
	while ((readdir_error = readdir_r(dir, &entry, &result)) == 0) {
		if (result == NULL) {
			break;
		}
		if (entry.d_name[0] >= '0' && entry.d_name[0] <= '9') {
			Pid p(entry.d_name);
			pids.push_back(p);
		}
	}

	closedir(dir);

	for (int attempt=0; attempt < 1; ++attempt) {
		try {
			Pgsql piddb("ender", "piddb", "piduser", "yter4Fk3", debug);
			piddb.begin();
			Prepare pid_sets_insert = piddb.createPrepare("pid_sets_insert");
			pid_sets_insert.setTableName("pid_sets");
			pid_sets_insert.addCol("nodename", utsbuffer.nodename);
			string node_time = Clock().str;
			pid_sets_insert.addCol("node_time", node_time);
			pid_sets_insert.exec();
			pid_sets_insert.getResult();
			uint64_t set_id = piddb.lastval();

			for (vector<Pid>::iterator i = pids.begin(); i != pids.end(); ++i) {
				Prepare pid_insert = piddb.createPrepare("pid_insert");
				pid_insert.setTableName("pids");
				pid_insert.addCol("set_id", set_id);
				pid_insert.addCol("cmdline", (*i).cmdline);
				pid_insert.addCol("pid", (*i).mypid);
				pid_insert.addCol("comm", (*i).comm); // from /proc/#/comm
				pid_insert.addCol("state", (*i).state);
				pid_insert.addCol("ppid", (*i).ppid);
				pid_insert.addCol("pgrp", (*i).pgrp);
				pid_insert.addCol("session", (*i).session);
				pid_insert.addCol("tty_nr", (*i).tty_nr);
				pid_insert.addCol("tpgid", (*i).tpgid);
				pid_insert.addCol("flags", (*i).flags);
				pid_insert.addCol("minflt", (*i).minflt);
				pid_insert.addCol("cminflt", (*i).cminflt);
				pid_insert.addCol("majflt", (*i).majflt);
				pid_insert.addCol("cmajflt", (*i).cmajflt);
				pid_insert.addCol("utime", (*i).utime);
				pid_insert.addCol("stime", (*i).stime);
				pid_insert.addCol("cutime", (*i).cutime);
				pid_insert.addCol("priority", (*i).priority);
				pid_insert.addCol("nice", (*i).nice);
				pid_insert.addCol("num_threads", (*i).num_threads);
				pid_insert.exec();
			}
			piddb.commit();

			// Empty results now that we don't need them
			for (map<string, PGresult *>::iterator i=Prepare::existing_prepares.begin();
					i != Prepare::existing_prepares.end(); ++i) {
				PQclear((*i).second);
			}
			Prepare::existing_prepares.clear();
		} catch(...) {
			cerr << "Unknown Exception caught." << endl;
			abort();
		}
		if (attempt % 400 == 0) {
			cerr << ".";
		}
	}
	return EXIT_SUCCESS;
}


