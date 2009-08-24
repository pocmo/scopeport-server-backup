// This file is part of ScopePort (Linux server).
//
// Copyright 2007, 2008 Lennart Koopmann
//
// ScopePort (Linux server) is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ScopePort (Linux server) is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ScopePort (Linux server).  If not, see <http://www.gnu.org/licenses/>.

//!  Event logging
/*!
 * Logs events to logfile and database based on loglevel and severity of event.
*/

#ifndef LOG_H_
#define LOG_H_

#include "../internal.h"
#include "../database/Database.h"

class Log : public Database {
	private:
		//! Path of logfile.  
		const char* logfile;
	public:
		Log(const char* myLogfile, mySQLData myDBData);
		
		//! Writes message to database and logfile.  
		/*!
		 * \param severity The severity of this event.
		 * \param errorcode The error code of this event.
		 * \param logmsg The message that should be logged.
		 */
		void putLog(int severity, string errorcode, string logmsg);

    static void debug(bool debug, string msg);
};

#endif /*LOG_H_*/
