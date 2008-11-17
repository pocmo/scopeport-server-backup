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

//! Sending of service warnings, check and storage of response times

#ifndef SERVICES_H_
#define SERVICES_H_

#include "../internal.h"
#include "../database/Database.h"

class Services : public Database{
	protected:
		//! The list of rudechecks to perform.
		/*!
		 * \sa RudeChecks
		 */
		map<int,string> rudeList;

		//! The list of softchecks to perform.
		/*!
		 * \sa SoftChecks
		 */
		map<int,string> softList;

		//! Holds the last error message.
		string error;

		//! Holds login and connection information of MySQL database.
		mySQLData dbData;
	public:
		Services(mySQLData myDBData);

		//! Returns the last error message.
		/*!
		 * \sa error
		 */
		string getError(){ return error; }

		//! Checks if the response time of a service is in range.
		/*!
		 * \param checkID The ID of the soft- or rudecheck
		 * \param ms The response time
		 * \returns True if the response time is in range, false if not.
		 */
		bool checkResponseTime(string checkID, int ms);

		//! Sends warning if service has failed
		/*!
		 * \param checkID The ID of the soft- or rudecheck
		 * \param recvGroup The ID of the receiver group
		 * \param hostname The name of the host this service runs on
		 * \param port The port of the checked service
		 * \param ms The response time of the service
		 */
		void sendWarning(string checkID, string recvGroup,
				string hostname, int port, int ms);

		//! Stores response time in database.
		/*!
		 * \param checkid The ID of the soft- or rudecheck
		 * \param ms The response time to store
		 */
		bool storeResponseTime(string checkid, int ms);
};

#endif /*SERVICES_H_*/
