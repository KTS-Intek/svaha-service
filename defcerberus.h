/****************************************************************************
**
**   Copyright © 2016-2017 The KTS-INTEK Ltd.
**   Contact: http://www.kts-intek.com.ua
**
**  This file is part of svaha-service-kts.
**
**  svaha-service-kts is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  svaha-service-kts is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with svaha-service-kts.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#ifndef DEFCERBERUS_H
#define DEFCERBERUS_H


#define MATILDA_PROTOCOL_VERSION_V2        1

#define MAX_PACKET_LEN                      10000000
#define SETT_MAX_UNCOMPRSS_PACkET_SIZE  500


//network speed level
#define NET_SPEED_VERY_LOW      3
#define NET_SPEED_LOW           11
#define NET_SPEED_NORMAL        30   //if speed > NET_SPEED_NORMAL : disable compressing
#define NET_SPEED_HIGH          55//300
#define NET_SPEED_VERY_HIGH     200//1500
#define NET_SPEED_UFS_1         800//1500


#define COMMAND_ZULU                            0
#define COMMAND_AUTHORIZE                       2
#define COMMAND_I_AM_A_ZOMBIE                   6
#define COMMAND_COMPRESSED_PACKET               8


#define COMMAND_READ_HASH                       22
#define COMMAND_WRITE_KILL_CLIENT               23


#endif // DEFCERBERUS_H
