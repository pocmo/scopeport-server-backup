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
#include "GeneralNotifications.h"
#include "../database/Database.h"
#include "../database/Information.h"
#include "../notifications/Mail.h"
#include "../notifications/XMPP.h"

GeneralNotifications::GeneralNotifications(mySQLData myDBData, mailingData myMailData,
XMPPData myXMPPData, mobilecData myClickatellData)
: Database(myDBData)
{
  dbData = myDBData;
  mailData = myMailData;
  xmppData = myXMPPData;
  clickatellData = myClickatellData;
}


bool GeneralNotifications::getGNGroup()
{
  // Initialize database connection.
  if(initConnection())
  {
    MYSQL* init = getHandle();
    // Get receivers.
    const char* gnQuery = "SELECT gnotigroup FROM settings LIMIT 1";
    if(mysql_real_query(init, gnQuery, strlen(gnQuery)) != 0)
    {
      // Query was not successful.
      mysql_close(init);
      return 0;
    }
    else
    {
      // Query successful.
      MYSQL_RES* res = mysql_store_result(init);
      MYSQL_ROW row;
      if(mysql_num_rows(res) > 0)
      {
        // Got the general notification group.
        if((row = mysql_fetch_row(res)) == NULL)
          return 0;
        gnGroup = row[0];
        mysql_free_result(res);
        mysql_close(init);
        return 1;
      }
      else
      {
        // Nothing fetched.
        mysql_free_result(res);
        mysql_close(init);
        return 0;
      }
    }
  }
  return 0;
}


bool GeneralNotifications::sendMessages(string subject, string message)
{

  bool fallback = 0;

  // Get the defined notification group for general notifications.
  if(!getGNGroup())
  {
    // Could not get receivers from DB.
    fallback = 1;
  }

  // Initialize database connection.
  if(!fallback && initConnection())
  {
    MYSQL* init = getHandle();

    Mail mailing(mailData, dbData);

    XMPP xmpp(xmppData, dbData);

    // Fetch mail receivers.
    vector<string> mailRecvList;
    if(mailData.doMailing)
    {
      mailRecvList = Information::getMailWarningReceivers(init, gnGroup, "3");
    }

    // Send a warning mail to every mail receiver.
    int mailRecvCount = 0;
    int mailRecvListSize = mailRecvList.size();
    while(mailRecvCount < mailRecvListSize)
    {
      if(!mailRecvList[mailRecvCount].empty())
        mailing.sendMail(mailRecvList[mailRecvCount], subject, message);
      mailRecvCount++;
    }

    // Get XMPP receivers.
    vector<string> xmppRecvList;
    if(xmppData.doXMPP)
    {
      xmppRecvList = Information::getXMPPWarningReceivers(init, gnGroup, "3");
    }

    // Send a warning message to every XMPP receiver.
    int xmppRecvCount = 0;
    int xmppRecvListSize = xmppRecvList.size();
    while(xmppRecvCount < xmppRecvListSize)
    {
      if(!xmppRecvList[xmppRecvCount].empty())
        xmpp.sendMessage(message, xmppRecvList[xmppRecvCount]);
      xmppRecvCount++;
    }

    // Clickatell SMS notifications.
    if(clickatellData.doMobileC && mailData.doMailing)
    {
      vector<string> mobilecRecvList = Information::getMobileCWarningReceivers(init,
        gnGroup, "3");

      int mobilecRecvCount = 0;
      int mobilecRecvListSize = mobilecRecvList.size();
      while(mobilecRecvCount < mobilecRecvListSize)
      {
        if(!mobilecRecvList[mobilecRecvCount].empty())
        {
          // Build the message that fits to the API.
          stringstream newWarningMsg;
          newWarningMsg << "user:" << clickatellData.username << endl
            << "password:" << clickatellData.password << endl
            << "api_id:" << clickatellData.apiID << endl
            << "to:" << mobilecRecvList[mobilecRecvCount] << endl
            << "text:" << "[ScopePort] " << message;
          mailing.sendMail(CLICKATELLMAIL, "", newWarningMsg.str());
        }
        mobilecRecvCount++;
      }
    }

    mysql_close(init);
    return 1;
  }
  else
  {
    // Send in fallback mode.

    message = message + " (This message has been sent in fallback mode!)";
    if(mailData.doMailing)
    {
      Mail mailing(mailData, dbData);
      mailing.sendMail(mailData.fallbackReceiver, subject, message);
    }

    if(xmppData.doXMPP)
    {
      XMPP xmpp(xmppData, dbData);
      xmpp.sendMessage(message, xmppData.fallbackReceiver);
    }

    if(clickatellData.doMobileC && mailData.doMailing)
    {
      Mail mailing(mailData, dbData);
      // Build the message that fits to the API.
      stringstream newWarningMsg;
      newWarningMsg << "user:" << clickatellData.username << endl
        << "password:" << clickatellData.password << endl
        << "api_id:" << clickatellData.apiID << endl
        << "to:" << clickatellData.fallbackNumber << endl
        << "text:" << "[ScopePort] " << message;
      mailing.sendMail(CLICKATELLMAIL, subject, newWarningMsg.str());
    }

    return 1;
  }
  return 0;
}
