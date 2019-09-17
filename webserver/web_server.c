// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

#include "mongoose.h"
#define URI "/web_server/null/magic" //uri for webclient connection
#define NC_ID_MAX 4096 //support 4096 connection

bool wc_connected = false;//webclient connected flag
static const int wc_id = 99999;// webclient connection id
int nc_id_cur = 0;//current avalible id 
int nc_id[NC_ID_MAX];//nc_id pool
struct mg_str wc_uri; 
static const char *s_http_port = "8000";

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
	struct http_message *hmsg = (struct http_message *) p;
	struct websocket_message *wsmsg = (struct websocket_message *) p;
	int *recvcount = (int *) p;
	struct mg_connection *c;
	switch (ev)
	{
		//deal with handshark to get webclint connection
		case MG_EV_WEBSOCKET_HANDSHAKE_DONE :
			if(mg_strstr(hmsg->uri,wc_uri)!=NULL)
			{
				wc_connected = true;
				nc->user_data = &wc_id; 
			}
			break;
		//deal with websocket data from webclient
		case MG_EV_WEBSOCKET_FRAME :
			if(wc_connected == true && *((int*)(nc->user_data)) == wc_id)
			{
				;
			}
			break;
		//deal with recv data from client
		case MG_EV_RECV :
			if(wc_connected == true)
			{
				if(nc->user_data == NULL)
				{
					nc->user_data = nc_id + nc_id_cur;
					nc_id_cur = (nc_id_cur+1) % NC_ID_MAX;

					//malloc buf for message
					void *intp = malloc(sizeof(int)+*recvcount);
					//create message
					memcpy(intp,nc->user_data,sizeof(int));
					memcpy(intp+sizeof(int),nc->recv_mbuf.buf,*recvcount);
					//send to webclient
					for(c = mg_next(nc->mgr,NULL);c != NULL;c = mg_next(nc->mgr,c))
					{
						if(c->user_data != NULL && *((int *)(c->user_data)) == wc_id)
						{
							mg_send_websocket_frame(c,WEBSOCKET_OP_BINARY,intp,sizeof(int)+*recvcount);
							mbuf_remove(&(nc->recv_mbuf),*recvcount);
							break;
						}
					}
					free(intp);
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
							break;
						}
					}
					free(intp);
				}
				;
			}
			break;
	}
}

int main(void) {
  struct mg_mgr mgr;
  struct mg_connection *nc;
  wc_uri = mg_mk_str(URI);
  for(int i = 0;i<NC_ID_MAX;i++)
  {
	  nc_id[i]=i;
  }

  mg_mgr_init(&mgr, NULL);
  printf("Starting web server on port %s\n", s_http_port);
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  if (nc == NULL) {
    printf("Failed to create listener\n");
    return 1;
  }

  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}
