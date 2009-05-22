// This file is part of ScopePort (Linux server).
//
// Copyright 2007, 2008, 2009 Lennart Koopmann
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

#ifndef INTERNAL_H_
#define INTERNAL_H_

#include <iostream>
#include <vector>
#include <map>
#include <ostream>
#include <fstream>
#include <string.h>
#include <ctype.h>
#include <sstream>
#include <signal.h>
#include <cstdlib>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

// Socket related stuff.
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <sys/file.h>
#include <netdb.h>
#include <errno.h>

// MySQL.
#include <mysql.h>

// GnuTLS.
#include <gnutls/gnutls.h>
// Diffie Hellman prime bits.
#define DH_BITS 1024

using namespace std;


// Configuration structs.

//! Struct: Holds login and connection information of MySQL database.
struct mySQLData {
	//! The host the MySQL server is running on. (Hostname or IP)
	const char*		host;
	//! The user we want to connect with.
	const char*		user;
	//! The password of the chosen user.
	const char*		pass;
	//! The ScopePort database.
	const char*		db;
	//! The port of the MySQL server we want to connect to. (0 = Standard port)
	unsigned int	port;

};

//! Struct: Holds login, server and general SMTP information.
struct mailingData {
	//! Is mailing enabled?
	bool			doMailing;
	//! Do we need to authenticate on the SMTP server?
	bool			mailUseAuth;
	//! The SMTP server we want to connect to. (Hostname or IP)
	string			mailServer;
	//! The port we want to use.
	unsigned int	mailPort;
	//! The user we want to connect with.
	string			mailUser;
	//! The password of the user.
	string			mailPass;
	//! Our hostname. Specified in EHLO sequence.
	string			mailHostname;
	//! The "mail-from" field of sent messages.
	string			mailFrom;
	//! A receiver that is used if an error occures while fetching other addresses.
	string			fallbackReceiver;
};

//! Struct: Holds information of login, server and general XMPP information
struct XMPPData {
	//! Is XMPP enabled?
	bool			doXMPP;
	//! The XMPP server we want to connect to.
	string			xmppServer;
	//! The port of the XMPP server.
	unsigned int	xmppPort;
	//! The user we want to connect with.
	string			xmppUser;
	//! The password of the user.
	string			xmppPass;
	//! A XMPP ressource (part of the JID like /home or /notebook).
	string			xmppResource;
	//! A receiver that is used if an error occures while fetching other addresses.
	string			fallbackReceiver;
};

//! Struct: Holds information of Clickatell SMS Gateway API.
struct mobilecData {
	//! Is usage of the API enabled?
	bool	doMobileC;
	//! The API username.
	string	username;
	//! The API password.
	string	password;
	//! The API ID.
	string	apiID;
	//! A receiver that is used if an error occures while fetching other addresses.
	string	fallbackNumber;
};

//! Struct: Holds information of emergeny notification receiver.
struct enData {
	//! The type of the receiver. (e.g. JID or email address)
	string	type;
	//! The address. (e.g. JID or email)
	string	address;
	//! The emergeny ID this notification belogs to.
	int		emergencyID;
	//! The notificationList ID of this receiver.
	int		ID;
};

// Functions.
int stringToInteger(string st);
string noSpaces(string str);
char* noNewLine(char* str, int size);
string noNewLineS(string str);
unsigned long resolveName(char* server);
bool numOnly(string checkMe);

// Paths.
#define CONFIGFILE "/etc/scopeport/scopeport-server.conf"
#define LOGFILE "/var/log/scopeport-server.log"
#define CACHEFILE "/var/spool/scopeport/server/cache"
#define PIDFILE "/var/run/scopeportserver.pid"

// The buffer for client communication.
#define TALKBUFSIZE 1024

// The email adress of the Clickatell SMS gateway.
#define CLICKATELLMAIL "sms@messaging.clickatell.com"

// Types of XMPP error replies.
#define XMPP_STREAM_ERROR 1
#define XMPP_GENERAL_ERROR 2

// Service states
#define SERVICE_STATE_CONFAIL 0
#define SERVICE_STATE_OKAY 1
#define SERVICE_STATE_OKAYTIME 2
#define SERVICE_STATE_INTERR -1
#define SERVICE_STATE_TIMEOUT 4

#endif /*INTERNAL_H_*/
