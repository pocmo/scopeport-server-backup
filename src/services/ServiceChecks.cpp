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
//#include "ServiceChecks.h"
//#include "../internal.h"
//#include "../database/Database.h"
//#include "../log/Log.h"
//#include "../misc/Timer.h"
//
//ServiceChecks::ServiceChecks(mySQLData myDBData)
//	: Services(myDBData){
//
//}
//
//bool softRun = 1;
//
//void killSoftConnect(int sig){
//	softRun = 0;
//}
//
//void ServiceChecks::doChecks(MYSQL* init){
//		int i = 0;
//		int softSize = softList.size();
//		while(i < softSize){
//			if(softList[i].length() > 0 ){
//
//				time_t rawtime;
//				time(&rawtime);
//
//				string checkid;
//				string hostname;
//				int port = 0;
//				string type;
//				string recpGroup;
//				int timeout = 0;
//
//				// Split softList entry to port and hostname.
//				istringstream iss(softList[i]);
//				int j = 0;
//				string token;
//				while(getline(iss, token, ':')){
//					switch(j){
//						case 0:
//							checkid = token;
//							break;
//						case 1:
//							hostname = token;
//							break;
//						case 2:
//							port = atoi(token.c_str());
//							break;
//						case 3:
//							type = token;
//							break;
//						case 4:
//							recpGroup = token;
//							break;
//						case 5:
//							istringstream isst(token);
//							isst >> timeout;
//							break;
//					}
//					j++;
//				}
//
//				if(timeout < 1)
//					timeout = 1;
//
//				if(port == 0 || hostname.empty())
//					continue;
//
//				int sock;
//				struct sockaddr_in server;
//				struct hostent *host;
//
//				stringstream query;
//				query << "UPDATE services SET state = ";
//
//				// Create socket.
//				sock = socket(AF_INET, SOCK_STREAM, 0);
//				if(sock < 0){
//					// Creating socket failed.
//					sendWarning(checkid, recpGroup, hostname, port, 0);
//					close(sock);
//					i++;
//					query << "0, lastcheck = " << rawtime << " WHERE id = " << checkid;
//					// Query and check if query was successful.
//					if(!setQuery(init, query.str())){
//						continue;
//					}
//					continue;
//				}
//
//				// Set socket to non-blocking mode, so we can use our SIGALRM timeout.
//				if(fcntl(sock, F_SETFL, O_NONBLOCK) < 0){
//					// Setting the socket to non-blocking mode failed.
//					close(sock);
//					continue;
//				}
//
//				// Verify host.
//				server.sin_family = AF_INET;
//				host = gethostbyname(hostname.c_str());
//				if(host==(struct hostent *) 0){
//					// Host unknown.
//					sendWarning(checkid, recpGroup, hostname, port, 0);
//					close(sock);
//					i++;
//					query << "0, lastcheck = " << rawtime << " WHERE id = " << checkid;
//					// Query and check if query was successful.
//					if(!setQuery(init, query.str())){
//						continue;
//					}
//					continue;
//				}
//
//				// Connect to defined port.
//				memcpy((char *) &server.sin_addr, (char *) host->h_addr, host->h_length);
//				server.sin_port=htons(port);
//
//				softRun = 1;
//
//				// This kills our connection after 10 seconds.
//				signal(SIGALRM, killSoftConnect);
//				alarm(timeout);
//
//				Log log(LOGFILE, dbData);
//
//				while(softRun){
//					int timeToConnect = 1;
//					Timer t;
//
//					t.startTimer();
//
//					connect(sock, (struct sockaddr *) &server, sizeof server);
//
//					timeToConnect = t.stopTimer();
//
//					sleep(3);
//
//					if(type != "none"){
//						int res = connect(sock, (struct sockaddr *) &server, sizeof server);
//						if(res < 0) {
//							// Could not connect. - Service not running.
//							sendWarning(checkid, recpGroup, hostname, port, 0);
//							query << "0";
//							softRun = 0;
//						}else{
//							// Connection successful! Service is running.
//							// Check for validity.
//							if(type == "smtp"){
//								int ms = 1;
//								if((ms = checkSMTP(sock)) > 0){
//									// Service is running. Check response time.
//									if(!storeResponseTime(checkid, ms))
//										log.putLog(2, "003", "Could not update service response time.");
//									// Check if the response time is higher than the defined maximum.
//									if(!checkResponseTime(checkid, ms)){
//										// This service has a too high response time.
//										sendWarning(checkid, recpGroup, hostname, port, ms);
//										query << "2";
//									}else{
//										// Everything okay.
//										query << "1";
//									}
//								}else{
//									query << "0";
//									sendWarning(checkid, recpGroup, hostname, port, 0);
//								}
//							}
//
//							if(type == "http"){
//								int ms = 1;
//								if((ms = checkHTTP(sock)) > 0){
//									// Service is running. Check response time.
//									if(!storeResponseTime(checkid, ms))
//										log.putLog(2, "004", "Could not update service response time.");
//									// Check if the response time is higher than the defined maximum.
//									if(!checkResponseTime(checkid, ms)){
//										// This service has a too high response time.
//										sendWarning(checkid, recpGroup, hostname, port, ms);
//										query << "2";
//									}else{
//										// Everything okay.
//										query << "1";
//									}
//								}else{
//									query << "0";
//									sendWarning(checkid, recpGroup, hostname, port, 0);
//								}
//							}
//
//							if(type == "imap"){
//								int ms = 1;
//								if((ms = checkIMAP(sock)) > 0){
//									// Service is running. Check response time.
//									if(!storeResponseTime(checkid, ms))
//										log.putLog(2, "005", "Could not update service response time.");
//									// Check if the response time is higher than the defined maximum.
//									if(!checkResponseTime(checkid, ms)){
//										// This service has a too high response time.
//										sendWarning(checkid, recpGroup, hostname, port, ms);
//										query << "2";
//									}else{
//										// Everything okay.
//										query << "1";
//									}
//								}else{
//									query << "0";
//									sendWarning(checkid, recpGroup, hostname, port, 0);
//								}
//							}
//
//							if(type == "pop3"){
//								int ms = 1;
//								if((ms = checkPOP3(sock)) > 0){
//									// Service is running. Check response time.
//									if(!storeResponseTime(checkid, ms))
//										log.putLog(2, "006", "Could not update service response time.");
//									// Check if the response time is higher than the defined maximum.
//									if(!checkResponseTime(checkid, ms)){
//										// This service has a too high response time.
//										sendWarning(checkid, recpGroup, hostname, port, ms);
//										query << "2";
//									}else{
//										// Everything okay.
//										query << "1";
//									}
//								}else{
//									query << "0";
//									sendWarning(checkid, recpGroup, hostname, port, 0);
//								}
//							}
//
//							if(type == "ssh"){
//								int ms = 1;
//								if((ms = checkSSH(sock)) > 0){
//									// Service is running. Check response time.
//									if(!storeResponseTime(checkid, ms))
//										log.putLog(2, "007", "Could not update service response time.");
//									// Check if the response time is higher than the defined maximum.
//									if(!checkResponseTime(checkid, ms)){
//										// This service has a too high response time.
//										sendWarning(checkid, recpGroup, hostname, port, ms);
//										query << "2";
//									}else{
//										// Everything okay.
//										query << "1";
//									}
//								}else{
//									query << "0";
//									sendWarning(checkid, recpGroup, hostname, port, 0);
//								}
//							}
//
//							if(type == "ftp"){
//								int ms = 1;
//								if((ms = checkFTP(sock)) > 0){
//									// Service is running. Check response time.
//									if(!storeResponseTime(checkid, ms))
//										log.putLog(2, "008", "Could not update service response time.");
//									// Check if the response time is higher than the defined maximum.
//									if(!checkResponseTime(checkid, ms)){
//										// This service has a too high response time.
//										sendWarning(checkid, recpGroup, hostname, port, ms);
//										query << "2";
//									}else{
//										// Everything okay.
//										query << "1";
//									}
//								}else{
//									query << "0";
//									sendWarning(checkid, recpGroup, hostname, port, 0);
//								}
//							}
//							softRun = 0;
//						}
//					}else{
//						// This check has no protocol to check for.
//						// Service is running. Check response time.
//						if(!storeResponseTime(checkid, timeToConnect))
//							log.putLog(2, "008", "Could not update service response time.");
//						// Check if the response time is higher than the defined maximum.
//						if(!checkResponseTime(checkid, timeToConnect)){
//							// This service has a too high response time.
//							sendWarning(checkid, recpGroup, hostname, port, timeToConnect);
//							query << "2";
//						}else{
//							// Everything okay.
//							query << "1";
//						}
//					}
//				}
//
//				// Reset the alarm timer.
//				alarm(0);
//
//				close(sock);
//
//				query	<< ", lastcheck = "
//						<< rawtime
//						<< " WHERE id = "
//						<< checkid;
//
//				// Query and check if query was successful.
//				if(!setQuery(init, query.str())){
//					log.putLog(1, "054", "Could not update state and time of last check of service.");
//					i++;
//					continue;
//				}
//			}
//			i++;
//		}
//}
//
