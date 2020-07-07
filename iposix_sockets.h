#pragma once

	struct ns_fd_set
	{
			unsigned int cnt;
			int s[FD_SETSIZE];   /* an array of SOCKETs */
	};
	struct iposix_sockets
	{
		virtual int accept(int, struct sockaddr *, int *)=0;
		virtual int bind(int, const struct sockaddr *, int)=0;
		virtual int connect(int, const struct sockaddr *, int)=0;
		virtual int getpeername(int, struct sockaddr *, int *)=0;
		virtual int getsockname(int, struct sockaddr *, int *)=0;
		virtual int getsockopt(int, int, int, void *, int *)=0;
		virtual int listen(int, int)=0;
		virtual int recv(int, void *, size_t, int)=0;
		virtual int recvfrom(int, void *, size_t, int, struct sockaddr *, int *)=0;
		virtual int send(int, const void *, size_t, int)=0;
		virtual int sendto(int, const void *,
					size_t, int, const struct sockaddr *, int)=0;
		virtual int setsockopt(int, int, int, const void *, int)=0;
		virtual int shutdown(int, int)=0;
		virtual int socket(int, int, int)=0;
		virtual int close(int)=0;
		virtual int fcntl(int, int, ...)=0;
		virtual int select(int, ns_fd_set *, ns_fd_set *, ns_fd_set *, struct timeval *)=0;
	};
