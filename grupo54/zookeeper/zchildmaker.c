#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <zookeeper/zookeeper.h>

/* ZooKeeper Znode Data Length (1MB, the max supported) */ 
#define ZDATALEN 1024 * 1024
static char *host_port;
static char *root_path = "/children";
static zhandle_t *zh;
static int is_connected;

/**
* Watcher function for connection state change events
*/
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	}
}

int main(int argc, char *argv[]) {
	char zdata_buf[128];
	struct tm *local;
	time_t t;
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s host:port\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	host_port = argv[1];
	zh = zookeeper_init(host_port, connection_watcher, 2000, 0, 0, 0);

	if (zh == NULL) {
		fprintf(stderr,"Error connecting to ZooKeeper server[%d]!\n", errno);
		exit(EXIT_FAILURE);
	}
	sleep(3); /* Sleep a little for connection to complete */
    if (is_connected) {
		if (ZNONODE == zoo_exists(zh, root_path, 0, NULL)) {
			fprintf(stderr, "%s doesn't exist! \
				Please start ZChildMaker.\n", root_path);
			exit(EXIT_FAILURE);
		}
		char node_path[120] = "";
		strcat(node_path,root_path); 
		strcat(node_path,"/node"); 
		int new_path_len = 1024;
		char* new_path = malloc (new_path_len);
		
		for (int i = 0; i < 5; i++) {
			if (ZOK != zoo_create(zh, node_path, "node data", 10, & ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, new_path, new_path_len)) {
				fprintf(stderr, "Error creating znode from path %s!\n", node_path);
			    exit(EXIT_FAILURE);
			}
			fprintf(stderr, "Ephemeral Sequencial ZNode created! ZNode path: %s\n", new_path); 
			sleep(5);
		}
		free (new_path);
	}
	return 0; 
}