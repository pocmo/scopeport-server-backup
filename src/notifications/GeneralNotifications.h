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

//! Sending notifications about critical failures of ScopePort
/*!
 * Sends notifications to defined emergency notification receiver if
 * e.g. the ScopePort database server has failed.
 */

#ifndef GENERALNOTIFICATIONS_H_
#define GENERALNOTIFICATIONS_H_

#include "../internal.h"
#include "../database/Database.h"

class GeneralNotifications : public Database {
	private:
		//! Holds database login information.
		mySQLData	dbData;
		
		//! Holds SMTP login information and settings.
		mailingData		mailData;
		
		//! Holds XMPP login information and settings.
		XMPPData		xmppData;

		//! Holds information of Clickatell SMS Gateway API.
		mobilecData		clickatellData;
		
		//! The ID of the general notification group defined in the web interface.
		string		gnGroup;
		
		//! The fallback mail receiver. Defined in the config file and used if no database connection can be established.
		/*!
		 * \sa fallbackXMPPReciver
		 */
		string		fallbackMailReceiver;
		
		//! The fallback XMPP receiver. Defined in the config file and used if no database connection can be established.
		/*!
		 * \sa fallbackMailReciver
		 */
		string		fallbackXMPPReceiver;
		
		//! Fetches the general notification group from the database.  
		/*!
		 * \sa gnGroup
		 * \return True if the gnGroup variable was filled, false in case of error.
		 */
		bool getGNGroup();
	public:
		GeneralNotifications(mySQLData myDBData, mailingData myMailData, XMPPData myXMPPData,
				mobilecData myClickatellData);
		//! Sends defined text and subject to all notification receivers.  
		/*! 
		 * \param subject The subject of the message.
		 * \param message The message itself.
		 * \return True if message was deliverd, false if an error occured.
		 */
		bool sendMessages(string subject, string message);
};

#endif /*GENERALNOTIFICATIONS_H_*/
