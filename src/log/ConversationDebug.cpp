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
#include "../log/ConversationDebug.h"
#include "../database/Database.h"

ConversationDebug::ConversationDebug(mySQLData myDBData, string myMethod, string myRemoteHost)
: Database(myDBData)
{
  method = myMethod;
  remoteHost = myRemoteHost;

  disabled = checkIfDisabledForThisMethod();
  create();
}


bool ConversationDebug::checkIfDisabledForThisMethod()
{
  stringstream query;
  query << "SELECT ";

  if(method == "smtp")
  {
    query << "conversation_logging_smtp";
  }
  else if(method == "xmpp")
  {
    query << "conversation_logging_xmpp";
  }
  else
  {
    return 1;
  }

  query << " FROM settings ORDER BY id DESC LIMIT 1";

  if(initConnection())
  {
    string result = sGetQuery(query.str());
    mysql_close(getHandle());
    if(result == "1") { return 0; }
  }

  return 1;
}


void ConversationDebug::create()
{
  if(disabled) { return; }

  static timeval tv;
  static timeval tv2;
  static struct timezone tz;
  gettimeofday(&tv, &tz);
  gettimeofday(&tv2, &tz);
  conversationID = tv.tv_usec * 3;
  stringstream query;

  query << "INSERT INTO conversations(conversation_id, method, remote_host, created_at) VALUES("
    << conversationID
    << ", '"
    << Database::escapeString(method)
    <<  "', '"
    << Database::escapeString(remoteHost)
    << "', NOW())";

  if(initConnection())
  {
    setQuery(getHandle(), query.str());
    mysql_close(getHandle());
  }
}


void ConversationDebug::log(unsigned int direction, string message)
{
  if(disabled) { return; }

  stringstream query;
  query << "INSERT INTO conversationmessages(conversation_id, direction, message, created_at) VALUES("
    << conversationID
    << ", "
    << direction
    <<  ", '"
    << Database::escapeString(message)
    << "', NOW())";
  if(initConnection())
  {
    setQuery(getHandle(), query.str());
    mysql_close(getHandle());
  }
}
