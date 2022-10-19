#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

const char *get_cmd = "GET";
const char *post_cmd = "POST";
void read_file(char *text, char *filename) {
    char buf[512];
    memset(buf, 0, sizeof(buf));
    FILE *headerfd = fopen(filename, "r");
    printf("open %s success ", filename);
    while(fgets(buf, 512, headerfd) != NULL) {
        //strcat(buf, "\r\n\0");
        strcat(text, buf);
    }
    //strcat(text, "\r\n");
    printf("%s success\n", filename);
    fclose(headerfd);
}

void write_text(char *text, char *filename) {
    FILE *wdfd = fopen(filename, "a");
    fprintf(wdfd, "%s\n", text);
    fclose(wdfd);
}


void sendhtml(int newsock) {
    char text[10000];
    memset(text, 0, sizeof(text));
    read_file(text, "header");
    strcat(text, "\r\n");
    read_file(text, "webpage.html");
    int k;
    if((k = send(newsock, text, strlen(text), 0)) < 0) {
        fprintf(stderr, "can't send\n");
        return;
    }
    printf("send %s\n\n", text);
}

int read_img(char *text, char *filename) {
    FILE *imgfd = fopen(filename, "rb");
    FILE *textfd = fopen("test.png", "w");
    int c;
    int n = strlen(text);
    while((c = fgetc(imgfd)) != EOF) {
	text[n] = c;
	text[n + 1] = '\0';
	n++;
	fputc(c, textfd);
	//printf("%02x ", c);
    }
    printf("\n");
    fclose(imgfd);
    fclose(textfd);
    return n;
}

void sendimg(int newsock, char *s) {
    char text[50000];
    memset(text, 0, sizeof(text));
    read_file(text,"img_head");
    text[strlen(text) - 1] = '\0';
    strcat(text, s);
    strcat(text, "\r\n\n");
    int n = read_img(text, s);
    printf("n = %d\n", n);
    int k;
    if((k = send(newsock, text, n, 0)) < 0) {
	fprintf(stderr, "can't send img\n");
	return;
    }
    printf("send %d %ld\n%s\n", k, strlen(text), text);
}

void write_to_text(int newsock, char* buf, char *filename) {
    buf = strtok(buf, "\n");
    while(buf != NULL && strncmp(buf, "text", 4) != 0) {
        buf = strtok(NULL, "\n");
    }
    write_text(buf, filename);
    char response[512];
    read_file(response, "post_head");
    strcat(response, "\r\n");
    int k;
    if((k = send(newsock, response, strlen(response), 0)) < 0) {
        fprintf(stderr, "can't send img\n");
        return;
    }
    printf(" send\n %s\n", response);
}

void get(int newsock, char *buf) {
    char s[512];
    sscanf(buf, "%s", s);
    if(strcmp(s, "/") == 0) {
        sendhtml(newsock);
    }
    else if(strncmp(s, "/Image", 6) == 0){
        sendimg(newsock, &s[1]);
    }
}

void post(int newsock, char *buf) {
    char s[512];
    sscanf(buf, "%s", s);
    if(strcmp(s, "/Text") == 0) {
        write_to_text(newsock, buf, &s[1]);
    }
}

int main() {
    int port_num = 7891;
    struct sockaddr_in sock_addr;
    int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd == -1) {
        fprintf(stderr, "can't create sockfd\n");
        exit(0);
    }
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port_num);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(sock_addr.sin_zero, '\0', sizeof(sock_addr.sin_zero));
    int reuse = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("set error\n");
        exit(0);
    }


    if(bind(sockfd, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in)) == -1) {
        fprintf(stderr, "can't bind socket\n");
        close(sockfd);
        exit(0);
    }
    else {
        printf("binding..\n");
    }
    char hostname[64];
    struct hostent *hent;
    if(gethostname(hostname, sizeof(hostname)) < 0) {
        printf("get name error\n");
    	exit(0);
    }

    if((hent = gethostbyname(hostname)) == NULL) {
        printf("get hostent error\n");
        exit(0);
    }
    printf("host %s\n", hent->h_name);
    for(int i = 0; hent->h_addr_list[i]; i++) {
        printf("%s:%d\n", inet_ntoa(*(struct in_addr*)(hent->h_addr_list[i])), port_num);
    }
    if(listen(sockfd, 10) == -1) {
        fprintf(stderr, "can't listen socket\n");
        close(sockfd);
        exit(0);
    }
    else {
        printf("listen..\n");
    }

    while(1) {
        printf("try accept..\n");
        int newsock = accept(sockfd, NULL, NULL);
        if(newsock < 0) {
            fprintf(stderr, "can't accept\n");
            close(sockfd);
            exit(0);
        }
        else {
            printf("accept\n");
        }
        const void *msg = "send something here\n";
        char buf[4096];
        int n;
        while((n = recv(newsock, (void *)buf, sizeof(buf), 0)) != 0) {
            if(n < 0) {
                fprintf(stderr, "can't recv\n");
                break;
            }
            else if(n > 0) {
                buf[n] = '\0';
                printf("%s\n", buf);
            }
            if(strncmp(buf, get_cmd, 3) == 0) {
                get(newsock, &buf[3]);
                break;
            }
            else if(strncmp(buf, post_cmd, 4) == 0) {
                post(newsock, &buf[4]);
                break;
            }
        }
        shutdown(newsock, SHUT_RDWR);
        close(newsock);
    }
}


