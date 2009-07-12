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

//! Send messages via XMPP
/*!
 * Sends messages using XMPP on a defined server with defined username
 * and password
 */

#ifndef XMPP_H_
#define XMPP_H_

#include "../internal.h"
#include "../log/ConversationDebug.h"

class XMPP{
	private:
		//! Holds information of login, server and general XMPP information
		XMPPData xmppData;

		//! Holds login and connection information of MySQL database.
		mySQLData dbData;

    ConversationDebug* p_debug;
    
    bool sendSocket(string msg);
    bool readSocket();

	public:
		XMPP(XMPPData xmppData, mySQLData myDBData);

		//! Updates xmppData
		/*!
		 * \sa xmppData
		 */
		void updateSettings(XMPPData xmppData);

		//! Extracts the error message out of an XML/XMPP error reply.
		/*!
		 * \param message The error reply to extract the error message from
		 * \param errortype Type of error (e.g. stream error)
		 * \return The extracted error message
		 */
		string getErrorReplyMessage(string message, int errortype);

		//! Sends XMPP message
		/*!
		 * \param message The text to send
		 * \param receiver JID of receiver
		 * \return True on success, false in case of error.
		 */
		bool sendMessage(string message, string receiver);

		//! Returns XMPPData struct with settings from database.
		static XMPPData fetchSettings(mySQLData dbData);
};

#endif /*XMPP_H_*/
