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

//! Send emails via SMTP
/*!
 * Sends emails using SMTP on a defined server, optionally with defined username
 * and password
 */

#ifndef MAIL_H_
#define MAIL_H_

#include "../internal.h"

class Mail{
	private:
		//! Holds login, server and general SMTP information.
		mailingData mailData;
	public:
		Mail(mailingData myMailData);
		//! Updates mailData
		/*!
		 * \sa mailData
		 */
		void updateSettings(mailingData myMailData);
		
		//! Sends email
		/*!
		 * \param toMail The receiver of this email
		 * \param subject The subject of this email
		 * \param mailText The text in this email
		 * \return True on success, false in case of error.
		 */
		bool sendMail(string toMail, string subject, string mailText);
};

#endif /*MAIL_H_*/
