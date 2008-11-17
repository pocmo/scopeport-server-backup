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

//!  Client blacklist
/*!
 * Checks and blacklists hosts that want to send sensor data.
*/

#ifndef BLACKLIST_H_
#define BLACKLIST_H_

#include "../internal.h"
#include "../database/Database.h"

class Blacklist: public Database {
	private:
		//! Holds database login information.
		mySQLData dbData;
	public:
		Blacklist(mySQLData myDBData);
		
		//! Checks if a given host (IP) is blacklisted.
		/*!
		 * \param host The IP that should be checked.
		 * \return False if blacklisted, true if not blacklisted
		 */
		bool checkHost(string host);
		
		//! Blacklists a host (IP).
		/*!
		 * \param host The IP that should be blacklisted.  
		 * \return False in case of error, true if blacklisting succeeded.
		 */		
		bool blackHost(string host);
};

#endif /*BLACKLIST_H_*/
