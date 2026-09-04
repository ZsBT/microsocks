#include "server.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <poll.h>

int resolve(const char *host, unsigned short port, struct addrinfo** addr) {
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_PASSIVE,
	};
	char port_buf[8];
	snprintf(port_buf, sizeof port_buf, "%u", port);
	return getaddrinfo(host, port_buf, &hints, addr);
}

int resolve_sa(const char *host, unsigned short port, union sockaddr_union *res) {
	struct addrinfo *ainfo = 0;
	int ret;
	SOCKADDR_UNION_AF(res) = AF_UNSPEC;
	if((ret = resolve(host, port, &ainfo))) return ret;
	memcpy(res, ainfo->ai_addr, ainfo->ai_addrlen);
	freeaddrinfo(ainfo);
	return 0;
}

int bindtoip(int fd, union sockaddr_union *bindaddr) {
	socklen_t sz = SOCKADDR_UNION_LENGTH(bindaddr);
	if(sz)
		return bind(fd, (struct sockaddr*) bindaddr, sz);
	return 0;
}

int server_waitclient(struct server *server, struct client* client) {
	struct pollfd *pollfds = calloc(server->count, sizeof(*pollfds));
	if(!pollfds) return -1;
	size_t i;
	for(i = 0; i < server->count; i++) {
		pollfds[i].fd = server->fds[i];
		pollfds[i].events = POLLIN;
	}
	int ret = poll(pollfds, server->count, -1);
	if(ret > 0) {
		ret = -1;
		for(i = 0; i < server->count; i++) {
			if(pollfds[i].revents & POLLIN) {
				socklen_t clen = sizeof client->addr;
				client->fd = accept(pollfds[i].fd, (void*)&client->addr, &clen);
				ret = client->fd == -1 ? -1 : 0;
				break;
			}
		}
	}
	free(pollfds);
	return ret;
}

static int add_listener(struct server *server, const struct sockaddr *addr, socklen_t addrlen) {
	int fd = socket(addr->sa_family, SOCK_STREAM, 0);
	if(fd < 0) return -1;
	int yes = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if(bind(fd, addr, addrlen) < 0 || listen(fd, SOMAXCONN) < 0) {
		close(fd);
		return -1;
	}
	int *fds = realloc(server->fds, (server->count + 1) * sizeof(*fds));
	if(!fds) {
		close(fd);
		return -1;
	}
	server->fds = fds;
	server->fds[server->count++] = fd;
	if(server->listener_log)
		server->listener_log(addr);
	return 0;
}

int server_setup(struct server *server, const char* listenip, const char* interface, unsigned short port,
	void (*listener_log)(const struct sockaddr *addr)) {
	server->fds = 0;
	server->count = 0;
	server->listener_log = listener_log;
	if(interface) {
		struct ifaddrs *iflist, *ifa;
		int failed = 0;
		if(getifaddrs(&iflist) == -1) return -1;
		for(ifa = iflist; ifa; ifa = ifa->ifa_next) {
			if(!ifa->ifa_addr || strcmp(ifa->ifa_name, interface)) continue;
			if(ifa->ifa_addr->sa_family == AF_INET) {
				struct sockaddr_in addr = *(struct sockaddr_in*)ifa->ifa_addr;
				addr.sin_port = htons(port);
				if(add_listener(server, (struct sockaddr*)&addr, sizeof(addr)))
					failed = 1;
			} else if(ifa->ifa_addr->sa_family == AF_INET6) {
				struct sockaddr_in6 addr = *(struct sockaddr_in6*)ifa->ifa_addr;
				addr.sin6_port = htons(port);
				if(add_listener(server, (struct sockaddr*)&addr, sizeof(addr)))
					failed = 1;
			}
		}
		freeifaddrs(iflist);
		if(failed || !server->count) {
			server_close(server);
			return -2;
		}
		return 0;
	}
	struct addrinfo *ainfo = 0;
	if(resolve(listenip, port, &ainfo)) return -1;
	struct addrinfo* p;
	for(p = ainfo; p; p = p->ai_next) {
		if(!add_listener(server, p->ai_addr, p->ai_addrlen)) break;
	}
	freeaddrinfo(ainfo);
	if(!server->count) {
		server_close(server);
		return -2;
	}
	return 0;
}

void server_close(struct server *server) {
	size_t i;
	for(i = 0; i < server->count; i++) close(server->fds[i]);
	free(server->fds);
	server->fds = 0;
	server->count = 0;
}
