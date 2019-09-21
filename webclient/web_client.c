/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * This is a WebSocket client example.
 * Prints WS frames to stdout and forwards stdin to server.
 */

#include "mongoose.h"

#define NC_ID_MAX 4096 //support 4096 connection
#define URI "/web_server/null/magic" //uri for webserver
#define LOCALEURI "127.0.0.1:9091" //uri for locale server
bool wc_connected = false;//websocket connected flag
static const int wc_id = 99999;// websocket connection id
int nc_id[NC_ID_MAX];//nc_id pool

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
	struct websocket_message *wsmsg = (struct websocket_message *) ev_data;
	int *recvcount = (int *)ev_data;
  switch (ev) {
    case MG_EV_CONNECT: {
      int status = *((int *) ev_data);
      if (status != 0) {
        printf("-- Connection error: %d\n", status);
      }
      break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
			wc_connected = true;
			nc->user_data = &wc_id;
      break;
    }
		case MG_EV_RECV : {
			if(*((int*)(nc->user_data)) != wc_id)
			{
				;
			}
			break;
		}
    case MG_EV_WEBSOCKET_FRAME: {
			int *client_id = malloc(sizeof(int));
			bool neednc = true;
			memcpy(client_id,wsmsg->data,sizeof(int));
			for(c = mg_next(nc->mgr,NULL);c !=NULL; c = mg_next(nc->mgr,c))
			{
				if(*((int *)(c->user_data)) == *client_id)
				{
					mg_send(c,(wsmsg->data+sizeof(int)),wsmsg->size-sizeof(int));
					neednc = false;
					break;
				}
			}
			if(neednc)
			{
				mg_connection *newnc = mg_connect(mgr,LOCALEURI,ev_handler);
				newnc->user_data = nc_id + *client_id;
				mg_send(newnc,(wsmsg->data+sizeof(int)),wsmsg->size-sizeof(int));
			}
			free(client_id);
			break;
		}
    case MG_EV_CLOSE: {
      printf("-- Disconnected\n");
      break;
    }
  }
}

int main(int argc, char **argv) {
  struct mg_mgr mgr;
  struct mg_connection *nc;
  const char *web_server_url = argc > 1 ? argv[1] : "ws://127.0.0.1:8000";
	strcat(web_server_url,URI);
	for(int i = 0;i < NC_ID_MAX;i++)
	{
		nc_id[i] = i;
	}

  mg_mgr_init(&mgr, NULL);

  nc = mg_connect_ws(&mgr, ev_handler, web_server_url, NULL, NULL);
  if (nc == NULL) {
    fprintf(stderr, "Invalid address\n");
    return 1;
  }

  while (1) {
    mg_mgr_poll(&mgr, 100);
  }
  mg_mgr_free(&mgr);

  return 0;
}