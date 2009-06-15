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

#include "../internal.h"
#include "Host.h"

Host::Host(mySQLData myDBData, int mySocket, struct sockaddr_in mySocketAddress){
  socket = mySocket;
  socketAddress = mySocketAddress;
  dbData = myDBData;
}

bool Host::send(string message){
  ssize_t len;
  len = write(socket, message.c_str(), strlen(message.c_str()));
  
  // Check if the message has been sent.
  if(len <= 0){
    return 0;
  }

  // Message has been sent successfully.
  return 1;
}

string Host::receive(){
  char buffer[TALKBUFSIZE];
  ssize_t len;

  // Receive.
  len = read(socket, buffer, TALKBUFSIZE-1);

cout << "buff: " << buffer << endl;

  // Check if something has been received.
  if(len <= 0){
    return "err";
  }

  // Terminate buffer char array.
  if(len < TALKBUFSIZE){
    buffer[len] = '\0';
  }else{
    return "err";
  }

  // Return buffer as string.
  stringstream res;
  res << buffer;
  return res.str();
}

string Host::getIPv4Address(){
  stringstream res;
  res << inet_ntoa(socketAddress.sin_addr);
  return res.str();
}

void Host::refuse(string reason){
  send("Closing connection (Reason: " + reason + ")");
}

void Host::notify(string reason){
cout << "notified: " << reason << endl;
  send(reason);
}

hostMessage Host::parse(string message){
  hostMessage result;
  string token;
  istringstream iss(message);
  unsigned int i = 0;

  while(getline(iss, token, ',')){
    switch(i){
      case 0:
        result.hostID = stringToInteger(token);
        break;
      case 1:
        result.type = token;
        break;
      case 2:
        result.value = token;
        break;
    }
    i++;
  }

  return result;
}

bool Host::isBlacklisted(Database db){
return 0;
  string query = "SELECT 1 FROM blacklisted_hosts WHERE host = '" + getIPv4Address() + "'";
  if(db.getNumOfResults(query) > 0){
    return 1;
  }else{
    return 0;
  }
}

bool Host::addToBlacklist(Database db){
  string query = "INSERT INTO blacklisted_hosts(host, created_at, updated_at) "
                 "VALUES('" + getIPv4Address() + "', NOW(), NOW())";
  return db.setQuery(db.getHandle(), query);
}

bool Host::checkLogin(Database db){
  // Get the login request message.
  string msg = receive();
cout << "checkLogin: " << msg << endl;
  hostMessage request = parse(msg);

  // Check if the data is valid.
  if(request.hostID == 0 || request.type != "login" || request.value.empty()){
    return 0;
  }

  // Check if the login is correct.
  stringstream query;
  query << "SELECT 1 FROM hosts WHERE id = " << request.hostID
        << " AND password = '" << Database::escapeString(request.value) << "'";

  if(db.getNumOfResults(query.str()) == 1){
    return 1;
  }else{
    return 0;
  }

  return 0;
}

bool Host::receiveAndStoreData(Database db){
  // Receive the sensor data.
  string msg = receive();
cout << "receiveAndStoreData: " << msg << endl;
  hostMessage sensor = parse(msg);
  return 1;
}


