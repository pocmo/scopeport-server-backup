//// This file is part of ScopePort (Linux server).
////
//// Copyright 2008 Lennart Koopmann
////
//// ScopePort (Linux server) is free software: you can redistribute it and/or
//// modify it under the terms of the GNU General Public License as published by
//// the Free Software Foundation, either version 3 of the License, or
//// (at your option) any later version.
////
//// ScopePort (Linux server) is distributed in the hope that it will be useful,
//// but WITHOUT ANY WARRANTY; without even the implied warranty of
//// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// GNU General Public License for more details.
////
//// You should have received a copy of the GNU General Public License
//// along with ScopePort (Linux server).  If not, see <http://www.gnu.org/licenses/>.
//
////! Service checks with protocol checks
///*!
// * This class performs service checks and checks for specified protocols.
// * Response times are measured.
// */
//
//#ifndef SOFTCHECKS_H_
//#define SOFTCHECKS_H_
//
//#include "../internal.h"
//#include "Services.h"
//
//class ServiceChecks: public Services {
//	public:
//		ServiceChecks(mySQLData myDBData);
//
//		//! Fetches the checks that need to be done.
//		bool getChecks();
//
//		//! Does the checks and stores results in database.
//		void doChecks(MYSQL* init);
//
//		//! Check if there is an SMTP server behind given socket connection.
//		/*!
//		 * \param sock The socket to test on
//		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
//		 */
//		int checkSMTP(int sock);
//
//		//! Check if there is an HTTP server behind given socket connection.
//		/*!
//		 * \param sock The socket to test on
//		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
//		 */
//		int checkHTTP(int sock);
//
//		//! Check if there is an IMAP server behind given socket connection.
//		/*!
//		 * \param sock The socket to test on
//		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
//		 */
//		int checkIMAP(int sock);
//
//		//! Check if there is an POP3 server behind given socket connection.
//		/*!
//		 * \param sock The socket to test on
//		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
//		 */
//		int checkPOP3(int sock);
//
//		//! Check if there is an SSH server behind given socket connection.
//		/*!
//		 * \param sock The socket to test on
//		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
//		 */
//		int checkSSH(int sock);
//
//		//! Check if there is an FTP server behind given socket connection.
//		/*!
//		 * \param sock The socket to test on
//		 * \return 0 in case of error/wrong protocol or the response time in milliseconds if everything went fine
//		 */
//		int checkFTP(int sock);
//};
//
//#endif /*SOFTCHECKS_H_*/
