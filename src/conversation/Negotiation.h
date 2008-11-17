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
// You should have received a opy of the GNU General Public License
// along with ScopePort (Linux server).  If not, see <http://www.gnu.org/licenses/>.

//! Conversation between server and client
/*!
 * Responsible for the handshake and former negotiation between
 * ScopePort server and clients.
 */

#ifndef NEGOTIATION_H_
#define NEGOTIATION_H_

#include "../internal.h"

class Negotiation{
	private:
		//! The socket that has accepted the client.  
		int 				sock;
		//! Socket information
		struct sockaddr_in	address;
		//! Holds if the client has chosen to use GnuTLS or not.  
		bool				useGNUTLS;
	public:
		//! Constructor
		/*!
		 * \param mySock The socket thas has accepted the client.
		 * \param myAddress Socket information
		 */
		Negotiation(int mySock, struct sockaddr_in myAddress);
		
		//! Returns the IP of the currently connected client.
		string	getCurrentClientIP();

		//! Returns if the currently connected client wants to use GnuTLS. Correctly set after performHandshake().
		/*!
		 * \return True if client wants to use GnuTLS, false if not.
		 * \sa performHandshake()
		 */
		bool	getTLSUsage();
		
		//! Sends an unencrypted message.
		/*!
		 * \param message The message to send.
		 * \return True if message was sent and received, false in case of error.
		 */
		bool	sendMessage(string message);
		
		//! Receives an unencrypted message.
		/*!
		 * \return The received message or "err" in case of error.
		 */
		string 	recvMessage();
		
		//! Performs the handshake in which the client asks to send sensor data.
		/*!
		 * \return True if everything went fine, false in case of error.
		 */
		bool	performHandshake();
};

#endif /*NEGOTIATION_H_*/
