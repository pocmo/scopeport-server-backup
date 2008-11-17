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

//! Fetching of ScopePort health values
/*!
 * Fetches health value of calling ScopePort process.
 */

#ifndef HEALTH_H_
#define HEALTH_H_

#include "../internal.h"

class Health{
	public:
		//! Returns the PID of the calling process.
		static	string	getPID();
		
		//! Returns the virtual memory size of the calling process.
		static	string	getVMSize();
		
		//! Returns the number of threads of the calling process.  
		static	string	getThreads();
		
		//! Returns the size of some database tables or the whole database.  
		/*!
		 * \param dbData Database login information. 
		 * \param type What size to get. 1: Total database size, 2: Size of table "sensordata", 3: Size of table "servicedata".
		 * \return Size in Megabyte
		 */ 
		static	double	getDBSize(mySQLData dbData, int type);
};

#endif /*HEALTH_H_*/
