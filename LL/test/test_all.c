void fun()
{
    free(fd->readBuffer);
    free(fd->method);
    free(fd->uri);
    freeHeaders(fd->headers);
    free(fd->ver);
    fd->readBufferIdx = 0;
    fd->readBufferLen = 0;
    fd->methodIdx = 0;
    fd->uriIdx = 0;
    fd->verIdx = 0;
    fd->headersIdx = 0;
    fd->withinHeaderIdx = 0;
}

/* Free thy mallocs */
void freeHeaders(char **headers)
{
    int i = 0;
    while (headers[i] != NULL) {
        free(headers[i]);
        i++;
    }
    free(headers);
}

//long dbgprintf(char *format, ...)
long dbgprintf(char *format)
{
    long done = 0;
    int len;
    va_start(arg, format);
    done = vprintf(format, arg);
    va_end(arg);
    return done;
}

int open_listenfd(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   ( void *)&optval, sizeof(int)) < 0)
        return -1;

    /* TCP_CORK: If set, don't send out partial frames. */
    if (setsockopt(listenfd, IPPROTO_TCP, TCP_CORK,
                   ( void *)&optval, sizeof(int)) < 0)
        return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr *) & serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void accept_connection(int listenfd, char *remote_ip, int *fdmax)
{
    struct sockaddr_storage remoteaddr; // client address
    int newfd;                  // newly accept()ed socket descriptor

    /* handle new connections */
    addrlen = sizeof remoteaddr;
    newfd = accept(listenfd, (struct sockaddr *)&remoteaddr, &addrlen);
    if (newfd > MAX_NO_FDS) {
        fprintf(stderr, "Max FD Size reached. Can't continue");
        return;
    }

    if (newfd == -1) {
        perror("accept");
        exit(1);
    } else {
        FD_SET(newfd, master);  // add to master set
        if (newfd > *fdmax) {   // keep track of the max
            *fdmax = newfd;
        }
        dbgprintf("selectserver: new connection from %s on\ 
                  socket %d\n",
                  inet_ntop(remoteaddr.ss_family,
                            get_in_addr((struct sockaddr *)&remoteaddr),
                            remote_ip, INET6_ADDRSTRLEN), newfd);
        fds[newfd].state = STATE_PRE_REQUEST;
        new_fd_data(&fds[newfd]);
    }
}

void shutdown_connection(void * fds, int i, int nbytes, void * pMaster)
{
    /* got error or connection closed by client */
    if (nbytes == 0) {
        /* connection closed */
        dbgprintf("selectserver: socket %d hung up\n", i);
    } else {
        perror("recv");
    }
    if (fds[i].state != STATE_PRE_REQUEST)
        free_fd_data(&fds[i]);
    fds[i].state = STATE_PRE_REQUEST;

    FD_CLR(i, pMaster);         // remove from master set

    close(i);
}

void clear_connection_baggage(void * fdDataList, int fd, void * pMaster)
{
    /*Todo: Merge with shutdown_connection() */
    if (fdDataList[fd].state != STATE_PRE_REQUEST)
        free_fd_data(&fdDataList[fd]);
    fdDataList[fd].state = STATE_PRE_REQUEST;

    int result;
    if (pMaster != NULL) {
        dbgprintf("Clearing connection baggage for %d\n", fd);
        FD_CLR(fd, pMaster);    /*In Select mode only */
        result = shutdown(fd, 2);
        if (result == -1) {
            perror("shutdown");
        }
        dbgprintf("Shutdown for %d is %d\n", fd, result);
    }
    result = close(fd);
    if (result == -1) {
        perror("close");
    }

}

int state_machine(void * fdDataList, int i, int nbytes, void * pMaster)
{
    int idx, j, len;
    fdDataList[i].readBufferLen += nbytes;

    int done = false;
    /* we got some data from a client */
    if (fdDataList[i].state == STATE_PRE_REQUEST) {
        fdDataList[i].state = STATE_METHOD;
    }

    if (fdDataList[i].state == STATE_METHOD) {
        idx = fdDataList[i].methodIdx;
        j = fdDataList[i].readBufferIdx;
        len = fdDataList[i].readBufferLen;

        while (j < len && idx < MAX_METHOD_SIZE) ;

        fdDataList[i].methodIdx = idx;
        fdDataList[i].readBufferIdx = j;

        if (idx == MAX_METHOD_SIZE) {   /*We don't like really long methods. Cut em off */
            fdDataList[i].method[idx] = 0;
            fdDataList[i].state = STATE_URI;
        }
    }

    if (fdDataList[i].state == STATE_URI) {
        idx = fdDataList[i].uriIdx;
        j = fdDataList[i].readBufferIdx;
        len = fdDataList[i].readBufferLen;

        while (j < len && idx < MAX_REQUEST_SIZE);

        fdDataList[i].uriIdx = idx;
        fdDataList[i].readBufferIdx = j;

        if (idx == MAX_REQUEST_SIZE) {  /*We don't like really long URIs either. Cut em off */
            fdDataList[i].uri[idx] = 0;
            fdDataList[i].state = STATE_VERSION;
        }
    }

    if (fdDataList[i].state == STATE_VERSION) {
        idx = fdDataList[i].verIdx;
        j = fdDataList[i].readBufferIdx;
        len = fdDataList[i].readBufferLen;

        while (j < len && idx < MAX_VER_SIZE) ;
        fdDataList[i].verIdx = idx;
        fdDataList[i].readBufferIdx = j;

        /*We don't like really long version either. Cut em off */
        if (idx == MAX_VER_SIZE) {      /*We don't like really long URIs either. Cut em off */
            fdDataList[i].ver[idx] = 0;
            fdDataList[i].state = STATE_HEADER;
        }
    }

    if (fdDataList[i].state == STATE_HEADER) {
        idx = fdDataList[i].withinHeaderIdx;
        j = fdDataList[i].readBufferIdx;
        len = fdDataList[i].readBufferLen;

        while (j < len) {
            if (fdDataList[i].readBuffer[j] == '\n') {
                if (idx == 0) {
                    fdDataList[i].state = STATE_COMPLETE_READING;
                    j++;
                    break;      /* The last of headers */
                }
                fdDataList[i].headers[fdDataList[i].headersIdx][idx] = 0;
                if (fdDataList[i].headersIdx < MAX_HEADERS)
                    fdDataList[i].headersIdx++;
                else {
                    /* OK, buddy, you have sent us MAX_HEADERS headers. That's all yer get */
                    fdDataList[i].state = STATE_COMPLETE_READING;
                    j++;
                    break;
                }
                fdDataList[i].headers[fdDataList[i].headersIdx] = NULL;
                idx = 0;
            } else if (fdDataList[i].readBuffer[j] == '\r') {
                /*Skip over \r */
            } else {
                if (idx == 0)
                    fdDataList[i].headers[fdDataList[i].headersIdx] =
                        malloc(MAX_BUFFER_SIZE * sizeof(char));
                fdDataList[i].headers[fdDataList[i].headersIdx][idx] =
                    fdDataList[i].readBuffer[j];
                idx++;
            }
            j++;
        }

        fdDataList[i].withinHeaderIdx = idx;
        fdDataList[i].readBufferIdx = j;
    }

    if (fdDataList[i].state == STATE_COMPLETE_READING) {

        request.client = i;
        request.reqStr = fdDataList[i].uri;
        request.method = fdDataList[i].method;
        request.ver = fdDataList[i].ver;
        request.headers = fdDataList[i].headers;

        td.fd = i;
        td.fdDataList = fdDataList;
        td.pMaster = pMaster;
        td.request = request;
        farmer_thread(td);
        done = false;
        server(request);
        done = true;
    }
    return done;
}

void initialize_threads()
{


    LOG_ERROR_ON_NULL(fifo = queueInit(), "main: Clean queue Init failed.\n");

    LOG_ERROR_ON_NULL(cleaner_fifo = queueInit(), "main: Clean queue Init failed.\n");

    dbgprintf("Creating threads\n");
    int i;
    fd_mutex = (void *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(fd_mutex, NULL);
    for (i = 0; i < NOPE_THREADS; i++) {
        pthread_create(&con, NULL, &worker_thread, fifo);
        //con+=sizeof(pthread_t);
    }
    socketpair(AF_UNIX, SOCK_STREAM, 0, socketpair_fd);
    if (FCNTL_NONBLOCK(socketpair_fd[0]) < 0) perror("fcntl");
    if (FCNTL_NONBLOCK(socketpair_fd[1]) < 0) perror("fcntl");
    dbgprintf("Socketpair 1 %d and 2 %d \n", socketpair_fd[0], socketpair_fd[1]);
}

void select_loop(int listenfd)
{

    /* Common vars */

    char socket_pair_buffer[2];
    int nbytes;

    char remote_ip[INET6_ADDRSTRLEN];

    int fd, fdmax;
    int done = 0;

    LOG_ERROR_ON_NULL(fdDataList =
                      malloc(sizeof(void) * MAX_NO_FDS),
                      "Can't malloc() on fdDataList");

    struct timeval tv;
    int poll_timeout = POLL_TIMEOUT;
    tv.tv_sec = 0;
    tv.tv_usec = POLL_TIMEOUT * 1000;

    /* keep track of the biggest file descriptor */
    fdmax = listenfd;           /* so far, it's this one */

    for (fd = 0; fd < fdmax; fd++) {
        fdDataList[fd].state = STATE_PRE_REQUEST;
    }

    int eventfd;
    struct epoll_event event;
    struct epoll_event *events;

    eventfd = epoll_create(1234);       /*Number is ignored */

    if (eventfd == -1) {
        perror("epoll_create");
        return;
    }

    event.data.fd = listenfd;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(eventfd, EPOLL_CTL_ADD, listenfd, &event)) {
        perror("epoll_ctl");
        return;
    }
    /* Socket Pair */
    event.data.fd = socketpair_fd[1];
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(eventfd, EPOLL_CTL_ADD, socketpair_fd[1], &event)) {
        perror("epoll_ctl_socketpair");
        return;
    }

    events = calloc(MAX_EVENTS, sizeof event);
    /* Epoll main loop */
    while (1) {
        cleaner_thread();       /*Run the thread clearer */
        //int n, e;
        n = epoll_wait(eventfd, events, MAX_EVENTS, poll_timeout);
        for (e = 0; e < n; e++) {
            if ((events[e].events & EPOLLERR) ||
                (events[e].events & EPOLLHUP) || (!(events[e].events & EPOLLIN))) {
                fprintf(stderr, "epoll error detected in line %d\n", __LINE__);
                close(events[e].data.fd);
                continue;
            }

            else if (listenfd == events[e].data.fd) {
                while (1) {
                    struct sockaddr in_addr;
                    int newfd;
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                    in_len = sizeof in_addr;

                    newfd = accept(listenfd, &in_addr, &in_len);

                    if (newfd == -1) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            /* We have processed all incoming  connections. */
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    } else if (newfd >= MAX_NO_FDS) {
                        /* Process some events before accepting more */
                        fprintf(stderr, "Reached MAX_NO_FDS at %d\n", newfd);
                        break;
                    }

                    if (getnameinfo(&in_addr, in_len,
                                    hbuf, sizeof hbuf,
                                    sbuf, sizeof sbuf,
                                    NI_NUMERICHOST | NI_NUMERICSERV) == 0) {

                        /*dbgprintf("accept()ed connection on  %d (host=%s, port=%s)\n",
                           newfd, hbuf, sbuf); */

                        fdDataList[newfd].state = STATE_PRE_REQUEST;
                        new_fd_data(&fdDataList[newfd]);
                    }

                    /* Make the incoming socket non-blocking and add it to the list of fds to monitor. */
                    if (FCNTL_NONBLOCK(newfd) < 0) {
                        perror("fcntl");
                        return;
                    }

                    event.data.fd = newfd;
                    event.events = EPOLLIN | EPOLLET;
                    if (epoll_ctl(eventfd, EPOLL_CTL_ADD, newfd, &event) < 0) {
                        perror("epoll_ctl");
                        return;
                    }
                }
                continue;
            }
            else if (socketpair_fd[1] == events[e].data.fd) {
                nbytes = read(events[e].data.fd, socket_pair_buffer, 1);
                if (nbytes == -1) {
                    if (errno != EAGAIN) {      /* EAGAINs we have read all data */
                        perror("read");
                    }
                }
            }
            else {
                fd = events[e].data.fd;
                while (1) {
                    nbytes =
                        read(fd, fdDataList[fd].readBuffer + fdDataList[fd].readBufferIdx,
                             MAX_REQUEST_SIZE - fdDataList[fd].readBufferLen);
                    if (nbytes == -1) {
                        if (errno != EAGAIN) {  /* EAGAINs we have read all data */
                            perror("read");
                            done = true;
                        }
                        break;
                    } else if (nbytes == 0) {
                        done = true;
                        break;
                    }
                    if ((done = state_machine(fdDataList, fd, nbytes, NULL))) {
                        break;
                    }
                }

                if (done) {
                    clear_connection_baggage(fdDataList, fd, NULL);
                }

            }
        }
    }
    /* Select stuff
     * Thank you Brian "Beej Jorgensen" Hall */
    void master;              // pMaster file descriptor list
    void read_fds;            // temp file descriptor list for select()

    FD_ZERO(&master);           /* clear the pMaster and temp sets */
    FD_ZERO(&read_fds);

    /* add the listener to the master set */
    FD_SET(listenfd, &master);

    /* Select main loop */
    while (1) {

        cleaner_thread();       /*Run the thread clearer */

        read_fds = master;      /* copy it */

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        /* run through the existing connections looking for data to read */
        for (fd = 0; fd <= fdmax; fd++) {
            if (!FD_ISSET(fd, &read_fds))       // we got one!!
                continue;
            if (fd == listenfd) {
                accept_connection(fdDataList, listenfd, remote_ip, &fdmax, &master);
                break;
            }
            nbytes =
                recv(fd, fdDataList[fd].readBuffer + fdDataList[fd].readBufferIdx,
                     MAX_REQUEST_SIZE - fdDataList[fd].readBufferLen, 0);
            /* read failure */
            if (nbytes <= 0) {
                shutdown_connection(fdDataList, fd, nbytes, &master);
                break;
            }

            /* State Machine */
            done = state_machine(fdDataList, fd, nbytes, &master);
            if (done) {
                clear_connection_baggage(fdDataList, fd, &master);
            }
        }                       // END looping through file descriptors
    }                           // END for(;;)--and you thought it would never end!
    return;
}


void notify_parent()
{
    dbgprintf("Notifying parent\n");

    int written = write(socketpair_fd[0], "C", 1);
    if (written == -1) {
        if (errno != EAGAIN) {  /* EAGAINs we have read all data */
            perror("notify_parent_write");
        }
    }
    dbgprintf("Wrote socketpair %d : %d\n", socketpair_fd[0], written);
}

void farmer_thread(void td)
{
    pthread_mutex_lock(fifo->mut);
    while (fifo->full) {
        pthread_cond_wait(fifo->notFull, fifo->mut);
    }
    queueAdd(fifo, td);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);
}

void cleaner_thread(void)
{
    pthread_mutex_lock(cleaner_fifo->mut);
    if (cleaner_fifo->empty) {
        pthread_mutex_unlock(cleaner_fifo->mut);
    } else {
        while (!cleaner_fifo->empty) {
            queueDel(cleaner_fifo, &td);
            clear_connection_baggage(td.fdDataList, td.fd, td.pMaster);
        }
        pthread_mutex_unlock(cleaner_fifo->mut);
        pthread_cond_signal(cleaner_fifo->notFull);
    }
}

void *worker_thread(void *arg)
{
    struct timespec tim, tim2;

    dbgprintf("Creating consumer\n");
    for (;;) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty) {
            dbgprintf("Worker: task queue EMPTY.\n");
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        queueDel(fifo, &td);
        dbgprintf("td.fdDataList %d, td.fd %d, td.pMaster %d, td.request.headers0 %s\n",
                  td.fdDataList, td.fd, td.pMaster, td.request.headers[0]);
        server(td.request);
        //clear_connection_baggage(td.fdDataList, td.fd, td.pMaster);
        pthread_mutex_lock(cleaner_fifo->mut);
        while (cleaner_fifo->full) {
            pthread_cond_wait(cleaner_fifo->notFull, cleaner_fifo->mut);
        }
        queueAdd(cleaner_fifo, td);
        pthread_mutex_unlock(cleaner_fifo->mut);
        pthread_cond_signal(cleaner_fifo->notEmpty);
        //notify_parent();

        /*
           tim.tv_sec = 0;
           tim.tv_nsec = 1000;
           nanosleep(&tim , &tim2);
         */
    }
}

int *queueInit(void)
{

    q = (void *) malloc(sizeof(queue));
    if (q == NULL)
        return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;

    q->buf = malloc(sizeof(THREAD_DATA) * MAX_NO_FDS);
    q->mut = (void *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mut, NULL);
    pthread_cond_init(q->notFull, NULL);
    pthread_cond_init(q->notEmpty, NULL);

    return (q);
}

void queueDelete(void* q)
{
    free(q->buf);
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    pthread_cond_destroy(q->notFull);
    free(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->notEmpty);
    free(q);
}

void queueAdd(void * q, int in)
{
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel(void * q, int * out)
{
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}

int main(void)
{
    default_port = DEFAULT_PORT;

    int listenfd;

    char *pPort = getenv("PORT");

    if (pPort != NULL)
        default_port = (int) strtol(pPort, (char **)NULL, 10);

    listenfd = open_listenfd(default_port);
    if (listenfd > 0) {
        dbgprintf("listen on port %d, fd is %d\n", default_port, listenfd);
    } else {
        perror("ERROR");
        exit(listenfd);
    }
    /*Ignore SIGPIPE signal, so if browser cancels the request, it won't kill the whole process. */
    signal(SIGPIPE, SIG_IGN);

    struct rlimit limit;

    limit.rlim_cur = MAX_NO_FDS * 4;
    limit.rlim_max = MAX_NO_FDS * 4;

    setrlimit(RLIMIT_NOFILE, &limit);

    /* Get max number of files. */
    getrlimit(RLIMIT_NOFILE, &limit);

    printf("The soft limit is %llu\n", limit.rlim_cur);
    printf("The hard limit is %llu\n", limit.rlim_max);
    initialize_threads();

    for (i = 0; i < NOPE_PROCESSES; i++) {
        int pid = fork();
        if (pid == 0) {         //  child
            select_loop(listenfd);
        } else if (pid > 0) {   //  parent
            dbgprintf("child pid is %d\n", pid);
        } else {
            perror("fork");
        }
    }
    if (FCNTL_NONBLOCK(listenfd) < 0)
        perror("fcntl");
}
