#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libwebsockets.h>
#include <sys/time.h>


/*
 * This demo server shows how to use libwebsockets to aysnc send data
 *
 */

const static int PROTOCOL_HTTP = 0;

static int callback_http(struct libwebsocket_context *context,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len) {
	switch (reason) {
		case LWS_CALLBACK_SERVER_WRITEABLE: {
			void *universal_response = "{ \"Hello\": \"World!\" }";
			printf("sending: %s\n", (char* ) universal_response);
			libwebsocket_write(wsi, universal_response, strlen(universal_response), LWS_WRITE_TEXT);
			break;
		}
	}

    return 0;
}

struct per_session_data__http {
	int fd;
};

// list of supported protocols and callbacks
static struct libwebsocket_protocols protocols[] = {
    // first protocol must always be HTTP handler
    {
        "http-only",        // name
        callback_http,      // callback
        0                   // per_session_data_size
    },
    {
        NULL, NULL, 0, 0      // end of list
    }
};

int main(void) {
    // server url will be http://localhost:7681
    int port = 8080;
    const char *interface = NULL;
    struct libwebsocket_context *context;
    uint oldus = 0;


    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
	info.port = 7681;
	info.iface = NULL;
	info.protocols = protocols;
	#ifndef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
	#endif
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.gid = -1;
	info.uid = -1;
	info.options = 0;     // no special options

    // create libwebsocket context representing this server
	context = libwebsocket_create_context(&info);
    //ourSocket = context.libwebsocket_fd_hashtable[0];

    if (context == NULL) {
        fprintf(stderr, "libwebsocket init failed\n");
        return -1;
    }
    
    printf("starting server...\n");
    
	int n = 0;
	while (n >= 0 ) {
		struct timeval tv;
		gettimeofday(&tv, NULL);

		/*
		 * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
		 * live websocket connection using the DUMB_INCREMENT protocol,
		 * as soon as it can take more packets (usually immediately)
		 */

		if (((unsigned int)tv.tv_usec - oldus) > 50000) {
			libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_HTTP]);
			oldus = tv.tv_usec;
		}

		/*
		 * If libwebsockets sockets are all we care about,
n		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */

		n = libwebsocket_service(context, 50);
	}

    libwebsocket_context_destroy(context);
    
    return 0;
}