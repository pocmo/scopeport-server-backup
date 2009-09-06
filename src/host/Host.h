// This file is part of ScopePort (Linux server).
//
// Copyright 2009 Lennart Koopmann
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

//! Sending of service warnings, check and storage of response times

#ifndef HOST_H_
#define HOST_H_

#include "../internal.h"
#include "../database/Database.h"

class Host {
	private:
	mySQLData dbData;
    int socket;

    struct sockaddr_in socketAddress;
	
    bool send(string message);
    string receive();
    hostMessage parse(string message);
    string generateQuery(hostMessage sensorData, Database db);
    int getSensorType(hostMessage sensorData);
    string generateQueryForRecentData(hostMessage sensorData);
    string generateDeletionQueryForRecentData(hostMessage sensorData);

	vector<string> buildNetworkInterfacesQuery(string data, unsigned int hostID);
	vector<string> buildCpusQuery(string data, unsigned int hostID);

  public:
	Host(mySQLData myDBData, int mySocket, struct sockaddr_in mySocketAddress);

    string getIPv4Address();
    void refuse(string reason);
    void notify(string reason);

    bool isBlacklisted(Database db);
    bool addToBlacklist(Database db);

    bool checkLogin(Database db);

    bool receiveAndStoreData(Database db);
};

#endif /*HOST_H_*/
