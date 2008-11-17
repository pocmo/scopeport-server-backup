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

#include "Negotiation.h"
#include "../internal.h"

Negotiation::Negotiation(int mySock, struct sockaddr_in myAddress){
	sock = mySock;
	address = myAddress;
	useGNUTLS = 0;
}

string Negotiation::getCurrentClientIP(){
	stringstream returnage;
	returnage << inet_ntoa(address.sin_addr);
	return returnage.str();
}

bool Negotiation::getTLSUsage(){
	if(useGNUTLS)
		return 1;
	return 0;
}

bool Negotiation::sendMessage(string message){
	// Send this message unencrypted.  
	ssize_t len;
	len = write(sock, message.c_str(), strlen(message.c_str()));
	if(len <= 0)
		return 0;
		
	// Message sent successfully.  
	return 1;
}

string Negotiation::recvMessage(){
	
	// This will hold the received message.  
	char buffer[TALKBUFSIZE];

	// The message to receive is not TLS encrypted.  
	ssize_t len;
	len = read(sock, buffer, TALKBUFSIZE-1);
	
	// Was something received?  
	if(len <= 0)
		return "err";
	
	if(len < TALKBUFSIZE){
		buffer[len] = '\0';
	}else{
		return "err";
	}
	
	// Received. Return message.  
	stringstream returnage;
	returnage << buffer;
	return returnage.str();
}

bool Negotiation::performHandshake(){
	string message;
	message = recvMessage();
	if(message == "SENSORSEND_GNUTLS"){
		// This client wants to use GNUTLS.  
		sendMessage("TLS-OK");
		useGNUTLS = 1;
		return 1;
	}
	
	if(message == "SENSORSEND_NOTLS"){
		// Client does not want to use TLS.  
		// Okay.  
		sendMessage("NOTLS-OK");
		return 1;
	}
	return 0;
}
