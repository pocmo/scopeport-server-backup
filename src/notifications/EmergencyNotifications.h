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

//! Sending notifications if an emergency was declared.
/*!
 * Sends notifications if an emergency was declared in the web interface.
 */

#ifndef EMERGENCYNOTIFICATIONS_H_
#define EMERGENCYNOTIFICATIONS_H_

#include "../internal.h"
#include "../database/Database.h"

class EmergencyNotifications : public Database {
	private:
		//! Holds login and connection information of MySQL database.  
		mySQLData dbData;
		
		//! Holds SMTP login information and settings.
		mailingData		mailData;
		
		//! Holds XMPP login information and settings.
		XMPPData		xmppData;
		
		//! Struct: Holds information of Clickatell SMS Gateway API.
		mobilecData clickatellData;
		
		//! Holds information about every receiver that needs to be informed.  
		map<int,string>	notificationList;
	public:
		//! Constructor
		/*!
		 * \sa dbData
		 * \sa mailData
		 * \sa xmppData
		 */
		EmergencyNotifications(mySQLData myDBData, mailingData myMailData,
								XMPPData myXMPPData, mobilecData myMobilecData);
		
		//! Fetch e.g. email adresses or JIDs that need to be informed. Fills notificationList.
		/*!
		 * \sa notificationList
		 * \return 1: Emergency notifications to send fetched, 0: Nothing to send, -1: Error
		 */
		int		fetchTasks();
		
		//! Return the number of receivers in notificationList.
		/*!
		 * \sa notificationList
		 */
		int		getListSize();
		
		//! Sends a message to the e.g. email address of JID with given notificatioList ID.
		/*!
		 * \param recvNum The index of receiver in notificationList
		 * \return 1: Message was sent, 0: Message could not be send, -1: Syntax error of receiver. No attempt to send message was made.
		 * \sa notificationList
		 */
		int		sendMessage(int recvNum);
		
		//! Splits an notificationList entry to its relevant parts.
		/*!
		 * \sa notificationList
		 */
		enData	splitEmergencyInformation(int recvNum);
		
		//! Marks an receiver as notified or not.  
		/*!
		 * \param recvNum The index of receiver in notificationList
		 * \param status The status this receiver should be set to.
		 * \sa notificationList
		 */
		bool	markReceiver(int recvNum, int status);	
};

#endif /*EMERGENCYNOTIFICATIONS_H_*/
