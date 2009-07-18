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
  char buffer[TALKBUFSIZE] = "";
  ssize_t len;

  // Receive.
  len = read(socket, buffer, TALKBUFSIZE-1);

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

  // Check if the messsage was received successfully.
  if(msg == "err"){
    return 0;
  }

  hostMessage sensorData = parse(msg);
  
  string query = generateQuery(sensorData);

  if(query == "err"){
    return 0;
  }

  if(getSensorType(sensorData) == SENSOR_TYPE_SENSORDATA){
    if(!db.setQuery(db.getHandle(), generateDeletionQueryForRecentData(sensorData))
        || !db.setQuery(db.getHandle(), generateQueryForRecentData(sensorData))){
      return 0;
    }
  }

  return db.setQuery(db.getHandle(), query);
}

int Host::getSensorType(hostMessage sensorData){
  return SENSOR_TYPE_SENSORDATA;
}

string Host::generateQueryForRecentData(hostMessage sensorData){
  stringstream query;
  query << "INSERT INTO recentsensorvalues(host_id, name, value, created_at) "
        << "VALUES("
        << sensorData.hostID
        << ", '"
        << Database::escapeString(sensorData.type)
        << "', '"
        << Database::escapeString(sensorData.value)
        << "', NOW())";
  return query.str();
}

string Host::generateDeletionQueryForRecentData(hostMessage sensorData){
  stringstream query;
  query << "DELETE FROM recentsensorvalues WHERE host_id = " << sensorData.hostID
        << " AND name = '" << Database::escapeString(sensorData.type) << "'";
  return query.str();
}

string Host::generateQuery(hostMessage sensorData){
  string table;
  stringstream values;

  int type = SENSOR_TYPE_SENSORDATA;

  if(type == SENSOR_TYPE_SENSORDATA){
    table = "sensorvalues(host_id, name, value, created_at)";
    values << sensorData.hostID
           << ", '"
           << Database::escapeString(sensorData.type)
           << "', '"
           << Database::escapeString(sensorData.value)
           << "', NOW()";
  }else if(type == SENSOR_TYPE_PROFILEDATA){
    
  }else{
    return "err";
  }

  return "INSERT INTO " + table + " VALUES( " + values.str() + ")";
}

