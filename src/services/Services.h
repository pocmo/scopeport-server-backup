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

class Services : public Database {
	protected:
		//! Holds login and connection information of MySQL database.
		mySQLData dbData;

	private:
		unsigned int	handlerID;
		unsigned int	serviceID;
		unsigned int	port;
		unsigned int	allowedFails;
		unsigned int	responseTime;
		unsigned int	maximumResponse;
		string 			host;
		string 			serviceType;
		string			notiGroup;
		string			hostname;

	public:
		Services(mySQLData myDBData, unsigned int myHandlerID);


		void setServiceID(unsigned int serviceID);
		void setAllowedFails(unsigned int allowedFails);
		void setPort(unsigned int port);
		void setMaximumResponse(unsigned int maxres);
		void setHost(string host);
		void setServiceType(string serviceType);
		void setNotiGroup(string notiGroup);
		void setHostname(string hostname);

		unsigned int getServiceID();
		unsigned int getHandlerID();

		int checkService();

		void updateStatus(int status);

		//! Checks if the response time of a service is in range.
		/*!
		 * \returns True if the response time is in range, false if not.
		 */
		bool checkResponseTime();

		//! Sends warning if service has failed
		void sendWarning();

		//! Stores response time in database.
		/*!
		 * \param checkid The ID of the soft- or rudecheck
		 * \param ms The response time to store
		 */
		bool storeResponseTime();

		//! Check if there is an SMTP server behind given socket connection.
		/*!
		 * \param sock The socket to test on
		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
		 */
		int checkSMTP(int sock);

		//! Check if there is an HTTP server behind given socket connection.
		/*!
		 * \param sock The socket to test on
		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
		 */
		int checkHTTP(int sock);

		//! Check if there is an IMAP server behind given socket connection.
		/*!
		 * \param sock The socket to test on
		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
		 */
		int checkIMAP(int sock);

		//! Check if there is an POP3 server behind given socket connection.
		/*!
		 * \param sock The socket to test on
		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
		 */
		int checkPOP3(int sock);

		//! Check if there is an SSH server behind given socket connection.
		/*!
		 * \param sock The socket to test on
		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
		 */
		int checkSSH(int sock);

		//! Check if there is an FTP server behind given socket connection.
		/*!
		 * \param sock The socket to test on
		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
		 */
		int checkFTP(int sock);
};

#endif /*SERVICES_H_*/
