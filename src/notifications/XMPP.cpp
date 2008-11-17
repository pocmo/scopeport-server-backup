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

#include "../internal.h"
#include "XMPP.h"
#include "../log/Log.h"

XMPP::XMPP(XMPPData myXMPPData, mySQLData myDBData){
	xmppData = myXMPPData;
	dbData = myDBData;
}

void XMPP::updateSettings(XMPPData myXMPPData){
	xmppData = myXMPPData;
}

#define XMPPBUFSIZE 1024

char xmppBuf[XMPPBUFSIZE];
int xmppSock;

bool sendXMPP(string msg){
	ssize_t len;
	len = write(xmppSock,msg.c_str(),strlen(msg.c_str()));
	if(len <= 0)
		return 0;
	return 1;
}

bool readXMPP(){
	ssize_t len;
	len = read(xmppSock,xmppBuf,XMPPBUFSIZE-1);
	if(len <= 0)
		return 0;
	return 1;
}

string XMPP::getErrorReplyMessage(string message, int errortype){
	if(errortype == XMPP_STREAM_ERROR){
		return "Stream error.";
	}

	if(errortype == XMPP_GENERAL_ERROR){
		// Cut out the error information (<error></error>).
		unsigned int start = message.find("<error");
		unsigned int stop = message.find("</error>");
		unsigned int length = stop-start;
		string error = message.substr(start, length);

		// We now have the error information in "string error".

		// Get the name of the error.
		start = error.find(">")+2;
		string temp = error.substr(start);
		stop = temp.find("xmlns")-1;
		string result = error.substr(start, stop);
		if(result.length() <= 0)
			return "Unknown";
		return result;
	}

	return "Unknown";
}

bool XMPP::sendMessage(string myMessage, string receiver){

	Log log(LOGFILE, dbData);

	struct sockaddr_in address;
	if((xmppSock = socket(AF_INET, SOCK_STREAM,0)) > 0){
		// Socket created.
		address.sin_family = AF_INET;
		address.sin_port = htons(xmppData.xmppPort);
		address.sin_addr.s_addr = resolveName((char*) xmppData.xmppServer.c_str());
		if(connect(xmppSock,(struct sockaddr *) &address, sizeof(address)) == 0){
			// Connected to server.

			stringstream message;
			stringstream reply;

			// Build message for stream initialization.
			message	<< "<stream:stream to='"
					<< xmppData.xmppServer
					<< "' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'>" << endl;

			// Send stream initialization.
			if(!sendXMPP(message.str())){
				close(xmppSock);
				return 0;
			}

			// Clear message.
			message.str("");

			// Build message for iq request.
			message << "<iq type='get' to='"
					<< xmppData.xmppServer
					<< "' id='auth1'><query xmlns='jabber:iq:auth'/></iq>" << endl;

			// Send iq request.
			if(!sendXMPP(message.str())){
				close(xmppSock);
				return 0;
			}

			// Read reply.
			if(!readXMPP()){
				close(xmppSock);
				return 0;
			}

			// See if an error occured.
			reply << xmppBuf;
			if(reply.str().find("<stream:error>", 0) != string::npos){

				// There was an error. Abort.
				stringstream error;
				error	<< "Could not send XMPP message to \""
						<< receiver
						<< "\" - Server reply: \""
						<< getErrorReplyMessage(reply.str(), XMPP_STREAM_ERROR) << "\"";
				log.putLog(1, "011", error.str());
				close(xmppSock);
				return 0;
			}

			// Clear reply.
			reply.str("");

			// Clear message.
			message.str("");

			// Build login message.
			message	<< "<iq type='set' id='auth2'><query xmlns='jabber:iq:auth'><username>"
					<< xmppData.xmppUser
					<< "</username><password>"
					<< xmppData.xmppPass
					<< "</password><resource>"
					<< xmppData.xmppResource
					<< "</resource></query></iq>"
					<< endl;

			// Send login message.
			if(!sendXMPP(message.str())){
				close(xmppSock);
				return 0;
			}

			// Read reply.
			if(!readXMPP()){
				close(xmppSock);
				return 0;
			}

			// See if an error occured.
			reply << xmppBuf;

			if(reply.str().find("<error", 0) != string::npos){
				// There was an error. Abort.
				stringstream error;
				error	<< "Could not send XMPP message to \""
						<< receiver
						<< "\" - Server reply: \""
						<< getErrorReplyMessage(reply.str(), XMPP_GENERAL_ERROR) << "\"";
				log.putLog(1, "012", error.str());
				close(xmppSock);
				return 0;
			}
			// Clear reply.
			reply.str("");

			// Clear message.
			message.str("");

			// Build the message to send alarm message to user.
			message	<< "<message to='"
					<< receiver
					<< "' from='"
					<< xmppData.xmppUser
					<< "@"
					<< xmppData.xmppServer
					<< "' type='chat' xml:lang='en'><body>"
					<< myMessage
					<< "</body></message>"
					<< endl;

			if(!sendXMPP(message.str())){
				close(xmppSock);
				return 0;
			}

			if(!readXMPP()){
				close(xmppSock);
				return 0;
			}

			// See if an error occurred.
			reply << xmppBuf;

			if(reply.str().find("<error", 0) != string::npos){
				// There was an error. Abort.
				stringstream error;
				error	<< "Could not send XMPP message to \""
						<< receiver
						<< "\" - Server reply: \""
						<< getErrorReplyMessage(reply.str(), XMPP_GENERAL_ERROR) << "\"";
				log.putLog(1, "013", error.str());
				close(xmppSock);
				return 0;
			}

			// Close this session.
			sendXMPP("</stream>");

			close(xmppSock);
			return 1;
		}else{
			// Could not connect to server.
			close(xmppSock);
			stringstream error;
			error	<< "Could not XMPP connect to \""
					<< xmppData.xmppServer
					<< "\"";
			log.putLog(1, "014", error.str());
			return 0;
		}
		log.putLog(1, "015", "Could not bind to socket for XMPP messaging.");
		close(xmppSock);
	}
	close(xmppSock);

	return 1;
}
