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

#ifndef PID_H_
#define PID_H_
extern "C" {
#include <stdint.h>
}

#include <string>
#include <iostream>
#include <ostream>


class Pid {
public:
	Pid(char number[]);
	virtual ~Pid();
	friend std::ostream& operator<<(std::ostream &os, const Pid &p);
	friend int main(int argc, char *argv[]);
private:
	std::string mypid_s;
	bool kthread;

	// /proc/#/cmdline
	void getcmdline(void);
	std::string cmdline;

	// /proc/#/comm
	void getcomm(void);
	std::string comm;

	// /proc/#/stat
	void getstat(void);
	pid_t mypid;
	std::string stat_comm;
	char state;
	pid_t ppid;
	pid_t pgrp;
	int session;
	int tty_nr;
	int tpgid;
	unsigned flags;
	unsigned long minflt;
	unsigned long cminflt;
	unsigned long majflt;
	unsigned long cmajflt;
	unsigned long utime;
	unsigned long stime;
	long cutime;
	long cstime;
	long priority;
	long nice;
	long num_threads;
};

std::ostream& operator<<(std::ostream &os, const Pid &p);

#endif /* PID_H_ */
