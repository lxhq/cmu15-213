#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_LINE_NUM 5
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

typedef struct {
    char *name;
    char *value;
} cache_line;

typedef struct {
    int next;
    cache_line* lines;
} cache;

typedef struct {
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
} req_line;

typedef struct {
    char name[MAXLINE];
    char value[MAXLINE];
} req_header;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void parse(int fd, req_line *line, req_header *headers, int *hds);
void parse_uri(char *uri, req_line *line);
req_header parse_header(char *buf);
int send_request(req_line *line, req_header *headers, int *hds);
void *thread(void *vargp);

void init_cache();
int reader(int fd, char *name);
void writer(char *name, char *value);
void free_cache();
cache mycache;
int readcnt;
sem_t mutex, w;

int main(int argc, char **argv)
{
    int listenfd, *connfd;
    pthread_t tid;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    init_cache();
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfd);
    }
    free_cache();
    return 0;
}

void doit(int fd)
{
    char buf[MAXLINE], name[MAXLINE], value[MAX_OBJECT_SIZE];
    rio_t rio;
    req_line line;
    req_header headers[30];
    int n, hds = 0;
    int value_size, connfd;

    /* parse the request line and header */
    parse(fd, &line, headers, &hds);

    strcpy(name, line.host);
    strcpy(name + strlen(name), line.path);
    if (reader(fd, name)) {
        fprintf(stdout, "%s from cache\n", name);
        fflush(stdout);
        return;
    }
    value_size = 0;
    connfd = send_request(&line, headers, &hds);
    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE))) {
        Rio_writen(fd, buf, n);
        strcpy(value + value_size, buf);
        value_size += n;
    }
    if (value_size < MAX_OBJECT_SIZE) {
        writer(name, value);
    }
    Close(connfd);
}

void parse(int fd, req_line *line, req_header *headers, int *hds) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) return;

    /* parse requset line */
    sscanf(buf, "%s %s %s", method, uri, version);
    parse_uri(uri, line);

    /* parse request header */
    *hds = 0;
    Rio_readlineb(&rio, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        headers[*hds] = parse_header(buf); // it is safe to return a struct
        (*hds)++;
        Rio_readlineb(&rio, buf, MAXLINE);   
    }
}

void parse_uri(char *uri, req_line *line) {
    if (strstr(uri, "http://") != uri) {
        fprintf(stderr, "Error: invalid uri\n");
        exit(0);
    }
    uri += strlen("http://");
    char *c = strstr(uri, ":");
    *c = '\0';
    strcpy(line->host, uri);
    uri = c + 1;
    c = strstr(uri, "/");
    *c = '\0';
    strcpy(line->port, uri);
    *c = '/';
    strcpy(line->path, c);
}
req_header parse_header(char *buf) {
    req_header header;
    char *c = strstr(buf, ": ");
    if (c == NULL) {
        fprintf(stderr, "Error: invalid header: %s\n", buf);
        exit(0);       
    }
    *c = '\0';
    strcpy(header.name, buf);
    strcpy(header.value, c + 2);
    return header;
}

int send_request(req_line *line, req_header *headers, int *hds) {
    int clientfd;
    char buf[MAXLINE], *buf_head = buf;
    rio_t rio;
    int i;

    clientfd = Open_clientfd(line->host, line->port);
    Rio_readinitb(&rio, clientfd);
    sprintf(buf_head, "GET %s HTTP/1.0\r\n", line->path);
    buf_head = buf + strlen(buf);
    for (i = 0; i < *hds; i++) {
        sprintf(buf_head, "%s: %s", headers[i].name, headers[i].value);
        buf_head = buf + strlen(buf);
    }
    sprintf(buf_head, "\r\n");
    Rio_writen(clientfd, buf, MAXLINE);

    return clientfd;
}

void *thread(void *vargp) {
    int connfd = *((int*)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
}

void init_cache() 
{
    int i;
    sem_init(&mutex, 0, 1);
    sem_init(&w, 0, 1);
    readcnt = 0;
    mycache.lines = (cache_line*)Malloc(sizeof(cache_line*) * 10);
    mycache.next = 0;
    for (i = 0; i < CACHE_LINE_NUM; i++) {
        mycache.lines[i].name = Malloc(sizeof(char) * MAXLINE);
        mycache.lines[i].value = Malloc(sizeof(char) * MAX_OBJECT_SIZE);
    }
}
int reader(int fd, char *name) 
{
    int success = 0;
    P(&mutex);
    readcnt++;
    if (readcnt == 1) {
        P(&w);
    }
    V(&mutex);

    int i;
    for (i = 0; i < CACHE_LINE_NUM; i++) {
        if (!strcmp(mycache.lines[i].name, name)) {
            Rio_writen(fd, mycache.lines[i].value, MAX_OBJECT_SIZE);
            success = 1;
            break;
        }
    }

    P(&mutex);
    readcnt--;
    if (readcnt == 0) {
        V(&w);
    }
    V(&mutex);
    return success;
}
void writer(char *name, char *value)
{
    P(&w);
    strcpy(mycache.lines[mycache.next].name, name);
    strcpy(mycache.lines[mycache.next].value, value);
    mycache.next = (mycache.next + 1) % CACHE_LINE_NUM;
    V(&w);
}
void free_cache() 
{
    int i = 0;
    for (i = 0; i < CACHE_LINE_NUM; i++) {
        free(mycache.lines[i].name);
        free(mycache.lines[i].value);
    }
}
