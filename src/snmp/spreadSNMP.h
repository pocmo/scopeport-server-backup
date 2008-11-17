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

//#ifndef SPREADSNMP_H_
//#define SPREADSNMP_H_
//
//#include "../internal.h"
//
//class spreadSNMP {
//	private:
//		int									port;
//		int									servSock;
//		struct sockaddr_in					address;
//		socklen_t							addrlen;
//		mySQLData							dbData;
//		gnutls_anon_server_credentials_t	anoncred;
//	public:
//		spreadSNMP(int myPort, mySQLData myDBData,
//				gnutls_anon_server_credentials_t myAnoncred);
//		bool				openSocket();
//		int					getServerSocket();
//		struct sockaddr_in	getServerAddress();
//		socklen_t			getServerAddrlen();
//		const char*			fetchMessage(int snmpid);
//		bool				serveSNMP(int clntSock);
//};
//
//#endif /*SPREADSNMP_H_*/
