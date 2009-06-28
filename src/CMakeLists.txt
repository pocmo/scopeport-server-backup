include_directories(/usr/include/mysql)
link_directories(/usr/lib/)

add_executable(scopeport-server core.cc host/Host.cpp database/Database.cpp database/Information.cpp health/Health.cpp log/Log.cpp misc/Timer.cpp notifications/Clickatell.cpp notifications/EmergencyNotifications.cpp notifications/GeneralNotifications.cpp notifications/Mail.cpp notifications/Warning.cpp notifications/XMPP.cpp services/Services.cpp cloud/Cloud.cpp log/ConversationDebug.cpp)

add_definitions(-O3 -Wall)

target_link_libraries(scopeport-server mysqlclient gnutls pthread)
