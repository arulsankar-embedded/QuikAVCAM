/*
 * webserver.h
 *
 *  Created on: 20-Jul-2017
 *      Author: arulsankar
 */

#ifndef _WEBSERVER_
#define _WEBSERVER_

//-----------------------------------------------------------------------------
//
//	webserver.h
//
//	C++ embedded web server class wrapper for libmicrohttpd
//
//	Copyright (c) 2010 Greg Satz <satz@iranger.com>
//	All rights reserved.
//
// SOFTWARE NOTICE AND LICENSE
// This work (including software, documents, or other related items) is being
// provided by the copyright holders under the following license. By obtaining,
// using and/or copying this work, you (the licensee) agree that you have read,
// understood, and will comply with the following terms and conditions:
//
// Permission to use, copy, and distribute this software and its documentation,
// without modification, for any purpose and without fee or royalty is hereby
// granted, provided that you include the full text of this NOTICE on ALL
// copies of the software and documentation or portions thereof.
//
// THIS SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS
// MAKE NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO, WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR
// PURPOSE OR THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE
// ANY THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
//
// COPYRIGHT HOLDERS WILL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL OR
// CONSEQUENTIAL DAMAGES ARISING OUT OF ANY USE OF THE SOFTWARE OR
// DOCUMENTATION.
//
// The name and trademarks of copyright holders may NOT be used in advertising
// or publicity pertaining to the software without specific, written prior
// permission.  Title to copyright in this software and any associated
// documentation will at all times remain with copyright holders.
//-----------------------------------------------------------------------------

typedef enum { COL_NODE, COL_TYPE, COL_NAME, COL_LOCATION } coltype_t;
typedef enum { CON_UNK, CON_GET, CON_POST } conntype_t;

class Webserver {
	public:
		Webserver(int const);
		~Webserver();
		bool isReady() { return ready; }
	private:
		static int HandlerEP(void *cls, struct MHD_Connection *conn, const char *url, const char *method,
				const char *version, const char *up_data, size_t *up_data_size, void **ptr);
		int Handler(struct MHD_Connection *conn, const char *url, const char *method,
				const char *version, const char *up_data, size_t *up_data_size, void **ptr);
		static void FreeEP(void *cls, struct MHD_Connection *conn, void **ptr, enum MHD_RequestTerminationCode code);
		void Free(struct MHD_Connection *conn, void **ptr, enum MHD_RequestTerminationCode code);

		int SendPollResponse(struct MHD_Connection *conn);
		int SendDeviceListResponse(struct MHD_Connection *conn);
		int SendPollJsonResponse(struct MHD_Connection *conn);
		int SentInstanceResponse(struct MHD_Connection *conn);
		//int SendDeviceID_userdata(struct MHD_Connection *conn);
		int SendUpgradeResponse(struct MHD_Connection *conn, int status);/* ANANDARAJ */
		int SendDeviceIDResponse (struct MHD_Connection *conn, char *id);/*ANANDARAJ*/
		int SendDeviceID_userdata (struct MHD_Connection *conn);
		int SendDeviceID_useraction(struct MHD_Connection *conn);


		static bool ready;
		coltype_t sortcol;
		unsigned long logbytes;
		bool adminstate;
		void *wdata;
};

#endif
