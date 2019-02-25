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

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <linux/limits.h>
#include <string.h>
#include "Pid.h"

using namespace std;

Pid::Pid(char number[]) {
	mypid = 0;
	istringstream(number) >> mypid;
	mypid_s = string("/proc/") + number;
	getcmdline();
	getcomm();
	getstat();
}

void Pid::getcmdline(void) {
	string proc_cmdline = mypid_s + "/cmdline";
	ifstream ifs(proc_cmdline.c_str(), ifstream::in);

	ifs.seekg(0, ifs.end);
	streamsize length = ifs.tellg();
	if (length > 0) {
		ifs.seekg(0, ifs.beg);

		char *buffer;
		try {
			buffer = new char [length];
		} catch(...) {
			cerr << "Error allocating " << sizeof(char)*(length) << " bytes." << endl;
			exit(1);
		}
		ifs.read(buffer, length);
		for (int i=0; i < length-1; ++i) {
			if (buffer[i] == 0) {
				buffer[i] = ' ';
			}
		}

		cmdline = buffer;
		delete[] buffer;
	}
	ifs.close();
}

void Pid::getcomm(void) {
	if (cmdline.length() == 0) {
		string proc_comm = mypid_s + "/comm";
		ifstream ifs(proc_comm.c_str(), ifstream::in);
		ifs.seekg(0, ifs.end);
		streamsize length = ifs.tellg();
		if (length > 0) {
			ifs.seekg(0, ifs.beg);

			char *buffer;
			try {
				buffer = new char [length];
			} catch(...) {
				cerr << "Error allocating " << sizeof(char)*(length) << " bytes." << endl;
				exit(1);
			}
			ifs.read(buffer, length);
			for (int i=0; i < length-1; ++i) {
				if (buffer[i] == 0) {
					buffer[i] = ' ';
				}
			}

			comm = buffer;
			delete[] buffer;
		}
		ifs.close();
		kthread = true;
	} else {
		kthread = false;
	}
}

void Pid::getstat(void) {
	string proc_stat = mypid_s + "/stat";
	ifstream ifs(proc_stat.c_str(), ifstream::in);
	ifs >> mypid >> stat_comm >> state >> ppid >> pgrp >> session >> tty_nr;
    ifs >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt >> utime;
	ifs >> stime >> cutime >> cstime >> priority >> nice >> num_threads;
}

Pid::~Pid() {
}

ostream& operator<<(ostream &os, const Pid &p) {
	os << p.mypid << " ";
	if (p.kthread == true)
		os << "[" << p.comm << "]";
	else {
		string buffer(p.cmdline);
		replace(buffer.begin(), buffer.end(), '\0', ' ');
		os << buffer;
	}
	return os;
}
