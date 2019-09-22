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
#define LOCALEURI "127.0.0.1:8000" //uri for locale server
bool wc_connected = false;//websocket connected flag
static const int wc_id = 99999;// websocket connection id
int nc_id[NC_ID_MAX];//nc_id pool
struct mg_mgr mgr;

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct websocket_message *wsmsg = (struct websocket_message *) ev_data;
  int *recvcount = (int *)ev_data;
  struct mg_connection *c=NULL;
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

	  //debug statement
	  printf("ws handshake has done\n");
      break;
    }
	case MG_EV_RECV : {
	  if(nc->user_data == NULL)
	  {
		//do nothing
		;
	  }
	  else if(*((int*)(nc->user_data)) != wc_id)
	  {
		void *intp = malloc(sizeof(int)+*recvcount);
		memcpy(intp,nc->user_data,sizeof(int));
		memcpy(intp+sizeof(int),nc->recv_mbuf.buf,*recvcount);
		for(c = mg_next(nc->mgr,NULL);c != NULL;c = mg_next(nc->mgr,c))
		{
		  if(c->user_data !=NULL && *((int*)(c->user_data)) == wc_id)
		  {
			mg_send_websocket_frame(c,WEBSOCKET_OP_BINARY,intp,sizeof(int)+*recvcount);
			mbuf_remove(&(nc->recv_mbuf),*recvcount);
			//debug
			char *message = malloc(*recvcount + 1);
			memset(message,0,*recvcount + 1);
			memcpy(message,nc->recv_mbuf.buf,*recvcount);
			printf("send message to webserver:\n%s\n",message);
			free(message);
			break;
		  }
		}
		free(intp);
	  }
	  break;
	}
    case MG_EV_WEBSOCKET_FRAME: {
	  int *client_id = malloc(sizeof(int));
	  bool neednc = true;
	  memcpy(client_id,wsmsg->data,sizeof(int));
	  //debug statement
	  printf("get ws message ,client id is %d\n",*client_id);
	  
	  for(c = mg_next(nc->mgr,NULL);c !=NULL; c = mg_next(nc->mgr,c))
	  {
		if(*((int *)(c->user_data)) == *client_id)
		{
		  mg_send(c,(wsmsg->data+sizeof(int)),wsmsg->size-sizeof(int));
		  neednc = false;
		  //debug
		  printf("no need to create new connection\n");
		  char * message = malloc(wsmsg->size - sizeof(int) + 1);
		  memset(message,0,wsmsg->size - sizeof(int) + 1);
		  memcpy(message,wsmsg->data + sizeof(int),wsmsg->size - sizeof(int));
		  printf("send message:\n%s\n",message);
		  free(message);
		  break;
		}
	  }
	  if(neednc)
	  {
		struct mg_connection *newnc = mg_connect(&mgr,LOCALEURI,ev_handler);
		newnc->user_data = nc_id + *client_id;
		mg_send(newnc,(wsmsg->data+sizeof(int)),wsmsg->size-sizeof(int));
		//debug
		char * message = malloc(wsmsg->size - sizeof(int) + 1);
		memset(message,0,wsmsg->size - sizeof(int) + 1);
		memcpy(message,wsmsg->data + sizeof(int),wsmsg->size - sizeof(int));
		printf("send message:\n%s\n",message);
		free(message);
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
  struct mg_connection *nc;
  
  const char *web_server_url_tmp = argc > 1 ? argv[1] : "ws://127.0.0.1:8080";
  int web_server_url_length = strlen(web_server_url_tmp)+strlen(URI)+1;
  char *web_server_url = malloc(web_server_url_length);
  memset(web_server_url,0,web_server_url_length);
  memcpy(web_server_url,web_server_url_tmp,strlen(web_server_url_tmp));
  
  //debug statement
  printf("get web_server_url_tmp:%s\n",web_server_url);
  
  strcat(web_server_url,URI);

  //debug statement
  printf("get web_server_url:%s\n",web_server_url);
  
  for(int i = 0;i < NC_ID_MAX;i++)
  {
	nc_id[i] = i;
  }

  //debug statement
  printf("write nc_id OK\n");
  
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

  free(web_server_url);

  return 0;
}