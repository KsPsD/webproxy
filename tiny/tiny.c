/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);                                                       //HTTP 트랜잭션을 처리하는 함수
void read_requesthdrs(rio_t *rp);                                        // 요청 헤더를 읽어주는 함수
int parse_uri(char *uri, char *filename, char *cgiargs);                 //uri 를 파싱해서 원하는 값들을 가져옴
void serve_static(int fd, char *filename, int filesize, char *method);   // 정적 컨텐츠 받아옴
void get_filetype(char *filename, char *filetype);                       //파일 타입별로 받아옴
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method); //동적 컨텐츠 처리
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

void *thread(void *vargp);
int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    pthread_t tid;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]); //듣기 식별자는 지속적으로 듣고 있는다.
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr,
                        &clientlen); // 클라이언트 연결 식별자를 만들어 준다.
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                    0);
        Pthread_create(&tid, NULL, thread, (void *)connfd); //각각의 연결 식별자를 통해 실행해 줍니다.
        // connfd를 포인터로 받으면 race 가 일어날 수 있기 때문에 연결 식별자를 그대로 넘긴다
        printf("Accepted connection from (%s, %s)\n", hostname, port);
    }
    return 0;
}

void *thread(void *vargp) //쓰레드에서 실행해야할 함수
{
    int connfd = (int)vargp;
    //쓰레드를 원래 프로세스와 연결을 끊어 줍니다.
    Pthread_detach(pthread_self());
    doit(connfd);
    Close(connfd);
}

void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];

    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers: \n");
    printf("%s", buf);
    //헤더에서 값들을 뽑아 옵니다.
    sscanf(buf, "%s %s %s", method, uri, version);

    /// 밑에는 에러처리
    //head method를 위해 head도 받을 수 있게 해줬습니다.
    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD"))
    {
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0)
    {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }
    //정적 컨텐츠일 때
    if (is_static)
    {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }

        serve_static(fd, filename, sbuf.st_size, method);
    }
    //동적 컨텐츠일때
    else
    {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
            return;
        }

        serve_dynamic(fd, filename, cgiargs, method);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title> Tiny Error</title>");
    sprintf(body, "%s<body bgcolor="
                  "ffffff"
                  "> \r\n",
            body);

    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em> The Tiny Web server</em> \r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html \r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d \r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
    {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }

    return;
}
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    if (!strstr(uri, "cgi-bin")) // cgi-bin 없을시 정적 컨텐츠로 처리해줌
    {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri) - 1] == '/')
            strcat(filename, "home.html");
        else if (!strcmp(uri, "/adder"))
        {
            strcat(filename, ".html");
        }
        return 1;
    }
    else //adder에서 필요한 값들 가져옴
    {
        ptr = index(uri, '?');
        if (ptr)
        {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }
        else
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}
//정적 컨텐츠 처리
void serve_static(int fd, char *filename, int filesize, char *method)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    get_filetype(filename, filetype); // 파일 타입 가져옴
    sprintf(buf, "HTTP/1.0 200 OK \r\n");
    sprintf(buf, "%sServer: Tiny Web Server \r\n", buf);
    sprintf(buf, "%sConnection: close \r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    printf("Response headers: \n");
    printf("%s", buf);
    //HEAD 메소드 일때는 그냥 헤더만 보내주게 하기 위해서 조건문으로 GET인지 체크해줌
    if (!strcasecmp(method, "GET"))
    {
        srcfd = Open(filename, O_RDONLY, 0);
        srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
        // srcp = (char *)Malloc(filesize);  //주석 처리 된 코드는 malloc 사용해서 mmap대신 malloc 사용
        // Rio_readn(srcfd, srcp, filesize);
        Close(srcfd);
        Rio_writen(fd, srcp, filesize);
        Munmap(srcp, filesize);
        // free(srcp);
    }
}
//파일 타입을 가져오는 함수
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}
//동적 컨텐츠를 제공하는 함수
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
    char buf[MAXLINE], *emptylist[] = {NULL};
    //성공을 알리는 응답라인
    sprintf(buf, "HTTP/1.0 200 OK \r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server \r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (!strcasecmp(method, "GET"))
    { //자식 프로세스를 만들어 실행 해줌
        if (Fork() == 0)
        {
            setenv("QUERY_STRING", cgiargs, 1);
            Dup2(fd, STDOUT_FILENO); //출력을 클라이언트에 해줌
            Execve(filename, emptylist, environ);
        }
        Wait(NULL);
    }
}