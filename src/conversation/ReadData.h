// This file is part of ScopePort (Linux server).
//
// Copyright 2007 Lennart Koopmann
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

//! Checks, splits and holds received sensor data
/*!
 * Checks incoming sensor data for validity. If the data is okay it gets splitted
 * and can be fetched from the object later.
 */ 

#ifndef READDATA_H_
#define READDATA_H_

#include "../internal.h"

class ReadData {
	private:
		//! The timestamp of the package.
		string timestamp;
		
		//! The host that sent this package. (Host ID)
		string host;
		
		//! The password of this host. Must have been checked earlier.
		string pass;
		
		//! The sensor type ID of this package.
		string st;
		
		//! The sensor value.
		string sv;
	public:
		//! Checks if the package is valid.
		/*!
		 * \param clientput The raw sensor data sent by the client.
		 * \return True if the sensor data is okay, false in case of error.
		 */
		bool inspectStream(string clientput);
		
		//! Return the timestamp of the current package.
		string getTimestamp() { return timestamp; }
	
		//! Return the host ID of current package.
		string getHost() { return host; }
		
		//! Return the password of the current package.
		string getPass() { return pass; }
		
		//! Return the sensor type ID of the current package.
		string getSt() { return st; }
		
		//! Return the sensor value of the current package.
		string getSv() { return sv; }
};

#endif /*READDATA_H_*/
