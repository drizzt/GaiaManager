/*
 * Copyright (C) 2010 drizzt
 *
 * Authors:
 * drizzt <drizzt@ibeglab.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cell/http.h>
#include <netex/net.h>
#include <netex/errno.h>
#include <sysutil/sysutil_msgdialog.h>

#include "dialog.h"
#include "network.h"

#define COVERS_BASE_URI "http://74.53.27.210/~ibegcrew/drizzt/gaiamanager/gallery/photos/COVERS"

bool download_cover(const char *title_id, const char *fileName)
{
	bool ret = false;
	static char buffer[65535];
	CellHttpClientId client = 0;
	CellHttpTransId trans = 0;
	CellHttpUri uri;
	int code = 0;
	uint64_t length = 0;
	size_t localRecv = -1;
	size_t poolSize = 0;
	void *uriPool = NULL;
	void *httpPool = NULL;
	char serverName[sizeof(COVERS_BASE_URI) + 10 + 4];

	FILE *fid = NULL;

	snprintf(serverName, sizeof(serverName), "%s/%s.PNG", COVERS_BASE_URI, title_id);

	httpPool = malloc(8 * 1024);
	if (!httpPool) {
		goto quit;
	}

	if (cellHttpInit(httpPool, 8 * 1024) < 0) {
		goto quit;
	}

	if (cellHttpCreateClient(&client) < 0) {
		goto quit;
	}

	if (cellHttpUtilParseUri(NULL, serverName, NULL, 0, &poolSize) < 0) {
		goto quit;
	}

	uriPool = malloc(poolSize);
	if (!uriPool) {
		goto quit;
	}

	if (cellHttpUtilParseUri(&uri, serverName, uriPool, poolSize, NULL) < 0) {
		goto quit;
	}

	if (cellHttpCreateTransaction(&trans, client, CELL_HTTP_METHOD_GET, &uri) < 0) {
		goto quit;
	}

	free(uriPool);
	uriPool = NULL;

	if (cellHttpSendRequest(trans, NULL, 0, NULL) < 0) {
		goto quit;
	}

	if (cellHttpResponseGetStatusCode(trans, &code) < 0) {
		goto quit;
	}
	if (code == 404 || code == 403) {
		goto quit;
	}

	if (cellHttpResponseGetContentLength(trans, &length) < 0) {
		goto quit;
	}

	fid = fopen(fileName, "wb");
	if (!fid) {
		goto quit;
	}

	memset(buffer, 0x00, sizeof(buffer));

	while (localRecv != 0) {
		if (cellHttpRecvResponse(trans, buffer, sizeof(buffer) - 1, &localRecv) < 0) {
			goto quit;
		}
		if (localRecv == 0)
			break;
		fwrite(buffer, localRecv, 1, fid);
	}

	ret = true;

  quit:
	if (fid)
		fclose(fid);
	if (trans)
		cellHttpDestroyTransaction(trans);
	if (client)
		cellHttpDestroyClient(client);
	cellHttpEnd();
	if (httpPool)
		free(httpPool);
	if (uriPool)
		free(uriPool);

	return ret;
}

/* vim: set ts=4 sw=4 sts=4 tw=120 */
