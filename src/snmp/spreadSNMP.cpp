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

#include "spreadSNMP.h"
#include "../log/Log.h"
#include "../database/Database.h"
//
//spreadSNMP::spreadSNMP(int myPort, mySQLData myDBData,
//		gnutls_anon_server_credentials_t myAnoncred){
//	dbData = myDBData;
//	port = myPort;
//	anoncred = myAnoncred;
//}
//
//bool spreadSNMP::openSocket(){
//	if((servSock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
//		// Socket could not be created.  
//		return 0;
//	}else{
//		// Socket created.  
//		setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, 0, sizeof(int));
//		address.sin_family = AF_INET;
//		// On which IPs to listen (INADDR_ANY -> any IP).  
//		address.sin_addr.s_addr = INADDR_ANY;
//		// On which port to listen.  
//		int port = 12201;
//		address.sin_port = htons(port);
//		if(bind(servSock,(struct sockaddr *) &address,sizeof (address)) != 0) {
//			// Could not bind to socket.  
//			return 0;
//		}else{
//			// Bound to socket! Listen.  
//			listen(servSock, 50);
//			return 1;
//		}
//	}
//}
//
//int spreadSNMP::getServerSocket(){
//	return servSock;
//}
//
//struct sockaddr_in spreadSNMP::getServerAddress(){
//	return address;
//}
//
//socklen_t spreadSNMP::getServerAddrlen(){
//	return addrlen;
//}
//
//const char* spreadSNMP::fetchMessage(int snmpid){
//	// Prepare database connection.  
//	Database db(dbData);
//	// Initialize database connection.  
//	if(db.initConnection()){
//		MYSQL* init = db.getHandle();
//		// Get all enabled rudechecks.  
//		stringstream query;
//		query << "SELECT snmpdevices.address, snmpitems.ID, snmpitems.item "
//				"FROM snmpitems "
//				"LEFT JOIN snmpdevices ON snmpdevices.ID = snmpitems.device "
//				"LEFT JOIN snmpclients ON snmpclients.ID = snmpdevices.snmpid "
//				"WHERE snmpclients.ID = " << snmpid;
//
//		if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) != 0){
//			// Query was not successful.  
//			mysql_close(init);
//			return "err";
//		}else{
//			// Query successful.  
//			MYSQL_RES* res = mysql_store_result(init);
//			MYSQL_ROW row;
//			stringstream message;
//			if(mysql_num_rows(res) > 0){
//				while((row = mysql_fetch_row(res))){
//					message		<< row[0]
//					       		<< "+"
//					       		<< row[1]
//					        	<< ":"
//					        	<< row[2]
//					        	<< "|";
//				}
//			}else{
//				// There are no services in the database.  
//				mysql_free_result(res);
//				mysql_close(init);
//				return "err";
//			}
//			mysql_free_result(res);
//			mysql_close(init);
//			// Return result.  
//			return message.str().c_str();
//		}
//	}
//	return "err";
//}
//
//bool spreadSNMP::serveSNMP(int clntSock){
//	
//	Log log(LOGFILE, dbData);
//
//	char buffer[1024] = "";
//	
//	// Init TLS session.  
//	gnutls_session_t session;
//	gnutls_init(&session, GNUTLS_SERVER);
//	gnutls_priority_set_direct(session, "NORMAL:+ANON-DH", NULL);
//	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
//	gnutls_dh_set_prime_bits(session, DH_BITS);
//	gnutls_transport_set_ptr(session, (gnutls_transport_ptr_t) clntSock);
//	
//	// Do TLS handshake.  
//	int ret = gnutls_handshake(session);
//			
//	// Did the handshake succeed?  
//	if(ret < 0){
//		// No.  
//		gnutls_deinit(session);
//		stringstream shakeError;
//		shakeError	<< "SNMP: TLS handshake with host "
//					<< inet_ntoa(address.sin_addr)
//					<< " failed.";
///////		log.putLog(shakeError.str());
//		close(clntSock);
//		return 0;
//	}
//			
//	// TLS handshake completed.  
//	
//	// Get request.  
//	
//	ret = gnutls_record_recv(session, buffer, 1024);
//	
//	istringstream iss;
//	int snmpid = 0;
//	iss.str(buffer);
//	iss >> snmpid;
//	
//	const char* message = fetchMessage(snmpid);
//	
//	if(strcmp(message,"err" ) == 0){
//		close(clntSock);
//		return 0;
//	}
//	
//	// Send reply.  
//	gnutls_record_send(session, message, strlen(message));
//		
//	gnutls_bye (session, GNUTLS_SHUT_RDWR);
//	gnutls_deinit(session);
//	gnutls_anon_free_server_credentials(anoncred);
//
//	close(clntSock);
//
//	return 0;
//}
