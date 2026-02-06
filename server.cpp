#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

const int BACKLOG = 10;
const char* MYPORT = "3490";

void sigchild_handler(int sig) 
{
    (void)sig;

    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}
void *get_in_addr(sockaddr *sa)
{
    if(sa->sa_family == AF_INET) { 
	return &(((sockaddr_in*)sa)->sin_addr);
    }
    return &(((sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo *servinfo;
    if(0 != getaddrinfo(NULL, MYPORT, &hints, &servinfo)) {
	std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }
    int listen_sock;
    addrinfo *p;
    for(p = servinfo; p != NULL; p=p->ai_next) { 
	if(-1 == (listen_sock = socket(p->ai_family,
				       p->ai_socktype,
				       p->ai_protocol))) {
    	    std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
	    continue;
    	}
    	int yes=1;
    	if(-1 == setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) { 
    	    std::cerr << "setsockoopt() error: " << std::strerror(errno) << std::endl; 
	    continue;
	}
    	if(-1 == bind(listen_sock, p->ai_addr, p->ai_addrlen)) {
    	    std::cerr << "bind() error: " << std::strerror(errno) << std::endl; 
	    continue;
    	}
	break;
    }
    freeaddrinfo(servinfo);

    if(p == NULL) {
	std::cerr << "Server: failed to bind" << std::endl;
	exit(1);
    }
    if(-1 == listen(listen_sock, BACKLOG)) {
	std::cerr << "listen() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }
    struct sigaction sa;
    sa.sa_handler = sigchild_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
	std::cerr << "Sigaction: " << std::strerror(errno) << std::endl;
	exit(1);
    }
    std::cout << "Server: waiting for connections..." << std::endl;

    sockaddr_storage their_addr;
    socklen_t their_addr_size = sizeof(their_addr);
    char client_addrstr[INET6_ADDRSTRLEN];
    while(1) {
	int client_sock = accept(listen_sock,
				 (sockaddr*)&their_addr,
				 &their_addr_size);
	if(client_sock == -1) {
	    std::cerr << "Accept: " << std::strerror(errno) << std::endl;
	    continue;
	}
	inet_ntop(their_addr.ss_family,
		  get_in_addr((sockaddr*)&their_addr),
		  client_addrstr, sizeof client_addrstr);
	std::cout << "Server: got connection from " << client_addrstr << std::endl;
	
	if(!fork()) {
	    close(listen_sock);
	    if(send(client_sock, "Hello, world!", 13, 0) == -1)
		std::cerr << "Send: " << std::strerror(errno) << std::endl;
	    close(client_sock);
	    exit(0);
	}
	close(client_sock);
    }

    return 0;
}
