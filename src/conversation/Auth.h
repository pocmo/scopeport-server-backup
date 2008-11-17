// This file is part of ScopePort (Linux server).
//
// Copyright 2008 Lennart Koopmann
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

//!  Client authentication
/*!
 * Checks if received sensor data contains correct password of submitting host.
*/

#ifndef AUTH_H_
#define AUTH_H_

#include "../internal.h"
#include "../database/Database.h"

class Auth: public Database {
	private:
		//! Holds passwords of hosts.
		map<string,string> hostList;
		//! Holds an error message in case of error.
		string error;
		//! Holds database credentials.
		mySQLData dbData;
	public:
		//! Constructor.
		/*!
		 * \param myDBData database credentials that is passed
		 * to class Database.
		 */
		Auth(mySQLData myDBData);
		
		//! Reads hosts and their passwords into hostList map.  
		/*!
		 * \sa hostList
		 * \return True or false in case of error.  
		 */
		bool loadHosts();
		
		//! Checks if a host submitted the correct password.
		/*!
		 * \param hostid The host ID of submitting host.
		 * \param pass Password of submitting host.
		 * \return True if the password is correct, false in case of error of wrong password.
		 */		
		bool checkHost(string hostid, string pass);
		
		//! Gets the last error message.
		/*!
		 * \return The error message.
		 */		
		string getError() { return error; }
};

#endif /*AUTH_H_*/
