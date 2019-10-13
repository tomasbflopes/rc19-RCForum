/* Group 39 - Joao Fonseca 89476, Tiago Pires 89544, Tomas Lopes 89552 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>

#define max(A, B) ((A) >= (B) ? (A) : (B))

int topic_amount = 0;

int isAlphanumeric(char *str)
{
    int i;
    for (i = 0; str[i] != '\0'; i++)
        if (!isalpha(str[i]) && !isdigit(str[i]))
            return 0;
    return 1;
}

void readUntil(int fd, char **bufferptr, int amount, char token)
{
    int i, n;
    for (i = 0; i < amount; i++)
    {
        do
        { // Reads until '\n'
            if ((n = read(fd, *bufferptr, 1)) <= 0)
                exit(1);
            *bufferptr += n;
        } while (*(*bufferptr - n) != token);
    }
}

void read_full(int fd, char **ptr, long n)
{
    int nr;
    while (n > 0)
    {
        if ((nr = read(fd, *ptr, n)) <= 0)
            exit(1);
        n -= nr;
        *ptr += nr;
    }
}

void write_full(int fd, char *string, long n)
{
    int nw;
    char *ptr = &string[0];
    while (n > 0)
    {
        if ((nw = write(fd, ptr, n)) <= 0)
            exit(1);
        n -= nw;
        ptr += nw;
    }
}

void setPort(int argc, char **argv, char *port)
{
    if (argc == 3)
    {
        if (!strcmp(argv[1], "-p"))
            strcpy(port, argv[2]);
        else
        {
            printf("Invalid command line arguments\n");
            exit(1);
        }
    }
    else if (argc != 1)
    {
        printf("Invalid command line arguments\n");
        exit(1);
    }
}

// Checks if a topic exists and gives its path (if pathname isn't NULL)
int findTopic(char *topic, char *pathname)
{
    DIR *dir;
    struct dirent *entry;
    char delim[2] = "_", topic_name[32], topic_folder[32];

    if ((dir = opendir("server/topics")) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(topic_folder, entry->d_name);
        strtok(entry->d_name, delim);
        strcpy(topic_name, strtok(NULL, delim));
        if (!strcmp(topic, topic_name))
        {
            if (pathname != NULL)
                sprintf(pathname, "server/topics/%s", topic_folder);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

// Finds a question in a certain topic and saves the ID of the submitter if id != NULL
int findQuestion(char *question, char *pathname, int *id)
{
    DIR *dir;
    struct dirent *entry;
    char delim[2] = "_", question_name[32], question_folder[32], quserid[8];

    if ((dir = opendir(pathname)) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(question_folder, entry->d_name);
        strtok(entry->d_name, delim);
        strcpy(question_name, strtok(NULL, delim));
        strcpy(quserid, strtok(NULL, delim)); // Gets user ID
        if (!strcmp(question_name, question))
        {
            strcat(pathname, "/");
            strcat(pathname, question_folder);
            if (id != NULL)
                *id = atoi(quserid);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

int findQImg(char *pathname, char *ext)
{
    DIR *dir;
    struct dirent *entry;
    char img[32], delim[2] = ".";

    if ((dir = opendir(pathname)) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(img, entry->d_name);
        strtok(img, delim);
        if (!strcmp(img, "qimg"))
        {
            strcpy(ext, strtok(NULL, delim));
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

int findAnswer(char *pathname, char *answer, int *id, int i)
{
    DIR *dir;
    struct dirent *entry;
    char i_txt[4], delim[3] = "_.", aux[4], aid[16];

    if (i < 10)
        sprintf(i_txt, "0%d", i);
    else
        sprintf(i_txt, "%d", i);

    if ((dir = opendir(pathname)) == NULL)
        return -1;
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(answer, entry->d_name);
        strcpy(aux, strtok(entry->d_name, delim));
        strcpy(aid, strtok(NULL, delim)); // Gets user ID
        if (!strcmp(i_txt, aux))
        {
            strtok(answer, ".");
            *id = atoi(aid);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

int findAImg(char *pathname, char *answer, char *aiext, int i)
{
    DIR *dir;
    struct dirent *entry;
    char delim[2] = ".", i_txt[4], aux[16];

    if (i < 10)
        sprintf(i_txt, "0%d", i);
    else
        sprintf(i_txt, "%d", i);

    if ((dir = opendir(pathname)) == NULL)
        return -1;
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(aux, entry->d_name);
        strtok(aux, delim);
        strcpy(aiext, strtok(NULL, delim));

        if (!strcmp(answer, aux) && strcmp(aiext, "txt"))
        {
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

// Returns number of questions in a topic or -1 if it finds a duplicate
int getQuestionCount(char *pathname, char *question)
{
    DIR *dir;
    struct dirent *entry;
    int question_count = 0;
    char aux[32], delim[2] = "_";

    if ((dir = opendir(pathname)) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        question_count++;
        strtok(entry->d_name, delim);
        strcpy(aux, strtok(NULL, delim));
        if (!strcmp(aux, question))
        {
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);
    return question_count;
}

// Returns number of answers in a question given a question path
int getAnswerCount(char *pathname)
{
    FILE *fp;
    int answer_count;
    char answer_path[128];

    strcpy(answer_path, pathname);
    strcat(answer_path, "/anscount.txt");

    if ((fp = fopen(answer_path, "r")) == NULL)
        exit(1);

    fscanf(fp, "%d\n", &answer_count);
    fclose(fp);

    return answer_count;
}

long getSizeOfFile(char *pathname, char *filename)
{
    FILE *fp;
    long size;
    char path_aux[512];

    strcpy(path_aux, pathname);
    strcat(path_aux, "/");
    strcat(path_aux, filename);
    if ((fp = fopen(path_aux, "r")) == NULL)
        return -1;
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fclose(fp);

    return size;
}

// ------ COMMANDS ------ //

int register_user(char *buffer, int *id, char *response, struct sockaddr_in *cliaddr)
{
    sscanf(buffer, "REG %d", id);
    printf("User %d (IP %s) trying to register... ", *id, inet_ntoa(cliaddr->sin_addr));
    if (*id > 9999 && *id < 100000)
    {
        strcpy(response, "RGR OK\n");
        printf("success\n");
    }
    else
    {
        strcpy(response, "RGR NOK\n");
        printf("denied\n");
    }
    return 0;
}

int topic_list(char *response)
{
    DIR *dir;
    struct dirent *entry;
    char topic_buffer[128], topic_name[16], topic_id[16];
    char delim[2] = "_";

    printf("User is listing the available topics\n");
    sprintf(response, "LTR %d", topic_amount);

    // Lists every directory
    if ((dir = opendir("server/topics")) == NULL)
        return -1;
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strtok(entry->d_name, delim);
        strcpy(topic_name, strtok(NULL, delim));
        strcpy(topic_id, strtok(NULL, delim));
        sprintf(topic_buffer, " %s:%s", topic_name, topic_id);
        strcat(response, topic_buffer);
    }
    strcat(response, "\n");

    closedir(dir);
    return 0;
}

int topic_propose(char *buffer, int *id, char *response)
{
    char topic_name[128], pathname[128];

    sscanf(buffer, "PTP %d %s", id, topic_name);

    printf("User %d is proposing topic %s... ", *id, topic_name);

    if (strlen(topic_name) > 10 || !isAlphanumeric(topic_name))
    {
        printf("invalid\n");
        strcpy(response, "PTR NOK\n");
    }
    else
    {
        if (findTopic(topic_name, NULL)) // Checks if topic already exists
        {
            printf("duplicate\n");
            strcpy(response, "PTR DUP\n");
        }
        else if (topic_amount >= 99) // Checks if the maximum number of topics has been reached
        {
            printf("full\n");
            strcpy(response, "PTR FUL\n");
        }
        else
        {
            printf("success\n");
            topic_amount++;
            if (topic_amount < 10)
                sprintf(pathname, "server/topics/0%d_%s_%d", topic_amount, topic_name, *id);
            else
                sprintf(pathname, "server/topics/%d_%s_%d", topic_amount, topic_name, *id);
            printf("%s\n", pathname);
            if (mkdir(pathname, 0666) == -1) // Enables R/W for all users
                return -1;
            strcpy(response, "PTR OK\n");
        }
    }
    return 0;
}

int question_list(char *buffer, char *response)
{
    DIR *dir;
    struct dirent *entry;
    char topic_buffer[128], topic_name[16], pathname[128], question_name[16], question_id[16], question_path[512];
    char delim[2] = "_";
    int question_amount = 0, answer_amount = 0;

    sscanf(buffer, "LQU %s", topic_name);

    printf("User is listing the available questions for the topic %s\n", topic_name);

    findTopic(topic_name, pathname);

    // Counts number of questions in a topic
    if ((dir = opendir(pathname)) == NULL)
        return -1;
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
        question_amount++;
    closedir(dir);
    sprintf(response, "LQR %d", question_amount);

    // Lists all the questions in a topic
    if ((dir = opendir(pathname)) == NULL)
        return -1;
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(question_path, pathname);
        strcat(question_path, "/");
        strcat(question_path, entry->d_name);
        answer_amount = getAnswerCount(question_path);

        strtok(entry->d_name, delim);
        strcpy(question_name, strtok(NULL, delim));
        strcpy(question_id, strtok(NULL, delim));
        sprintf(topic_buffer, " %s:%s:%d", question_name, question_id, answer_amount);
        strcat(response, topic_buffer);
    }
    closedir(dir);
    strcat(response, "\n");
    return 0;
}

int question_get(int connfd, char *buffer)
{
    char topic[16], question[16], pathname[128], path_aux[128], img[16], iext[8], aux[64];
    char *response, *responseptr, *bufferptr;
    int answer_count, fd, i, j, quserid = -1;
    long ressize = 2048, qsize, isize, realloc_aux;
    FILE *fp;

    if ((response = (char *)malloc(2048 * sizeof(char))) == NULL)
        return -1;

    responseptr = &response[0];
    bufferptr = &buffer[4];

    readUntil(connfd, &bufferptr, 1, '\n');

    if (sscanf(buffer, "GQU %s %s\n", topic, question) != 2)
    {
        write_full(connfd, "QGR ERR\n", strlen("QGR ERR\n"));
        printf("User is trying to get question... failure\n");
        free(response);
        free(buffer);
        return 0;
    }

    printf("User is trying to get question %s of topic %s... ", question, topic);

    // Opens topic folder
    strcpy(pathname, "server/topics");
    if (!findTopic(topic, pathname))
    {
        write_full(connfd, "QGR EOF\n", strlen("QGR EOF\n"));
        free(response);
        free(buffer);
        printf("not found\n");
        return 0;
    }

    // Opens question folder
    if (!findQuestion(question, pathname, &quserid))
    {
        write_full(connfd, "QGR EOF\n", strlen("QGR EOF\n"));
        free(response);
        free(buffer);
        printf("not found\n");
        return 0;
    }

    // Checking number of answers
    answer_count = getAnswerCount(pathname);

    // Determining size of question file
    qsize = getSizeOfFile(pathname, "qinfo.txt");

    ressize += qsize;
    realloc_aux = responseptr - response;
    if ((response = (char *)realloc(response, ressize * sizeof(char))) == NULL)
        return -1;
    responseptr = response + realloc_aux;

    sprintf(response, "QGR %d %ld ", quserid, qsize);
    responseptr = response + strlen(response);

    // Opens question text file
    strcpy(path_aux, pathname);
    strcat(path_aux, "/qinfo.txt");
    if ((fd = open(path_aux, O_RDONLY)) < 0)
        return -1;
    read_full(fd, &responseptr, qsize);
    if (close(fd) < 0)
        return -1;

    memcpy(responseptr, " ", 1);
    responseptr++;

    // Opens image file if one exists
    if (findQImg(pathname, iext))
    {
        memcpy(responseptr, "1 ", 2);
        responseptr += 2;
        memcpy(responseptr, iext, 3);
        responseptr += 3;
        memcpy(responseptr, " ", 1);
        responseptr += 1;

        sprintf(img, "qimg.%s", iext);
        isize = getSizeOfFile(pathname, img);

        ressize += isize;
        realloc_aux = responseptr - response;
        if ((response = (char *)realloc(response, ressize * sizeof(char))) == NULL)
            return -1;
        responseptr = response + realloc_aux;

        sprintf(aux, "%ld", isize);
        memcpy(responseptr, aux, strlen(aux));
        responseptr += strlen(aux);
        memcpy(responseptr, " ", 1);
        responseptr += 1;

        // Opens image file
        strcpy(path_aux, pathname);
        strcat(path_aux, "/");
        strcat(path_aux, img);
        if ((fd = open(path_aux, O_RDONLY)) < 0)
            return -1;
        read_full(fd, &responseptr, isize);
        if (close(fd) < 0)
            return -1;

        memcpy(responseptr, " ", 1);
        responseptr += 1;
    }
    else
    {
        memcpy(responseptr, "0 ", 2);
        responseptr += 2;
    }
    sprintf(aux, "%d", answer_count);
    if (answer_count >= 10)
    {
        memcpy(responseptr, "10", 2);
        responseptr += 2;
    }
    else
    {
        memcpy(responseptr, aux, 1);
        responseptr += 1;
    }

    for (i = answer_count, j = 0; i > 0 && j < 10; i--, j++)
    {

        char aiext[8], answer[32];
        long asize, aisize;
        int auserid;

        // Opens question folder
        findAnswer(pathname, answer, &auserid, i);

        // Determining size of answer text file
        strcpy(path_aux, pathname);
        strcat(path_aux, "/");
        strcat(path_aux, answer);
        strcat(path_aux, ".txt");

        if ((fp = fopen(path_aux, "r")) == NULL)
            return -1;
        fseek(fp, 0L, SEEK_END);
        asize = ftell(fp);
        fclose(fp);

        ressize += asize;
        realloc_aux = responseptr - response;
        if ((response = (char *)realloc(response, ressize * sizeof(char))) == NULL)
            return -1;
        responseptr = response + realloc_aux;

        if (i < 10)
            sprintf(aux, " 0%d %d %ld ", i, auserid, asize);
        else
            sprintf(aux, " %d %d %ld ", i, auserid, asize);
        memcpy(responseptr, aux, strlen(aux));
        responseptr += strlen(aux);

        // Opens answer text file
        if ((fd = open(path_aux, O_RDONLY)) < 0)
            return -1;

        read_full(fd, &responseptr, asize);

        if (close(fd) < 0)
            return -1;

        memcpy(responseptr, " ", 1);
        responseptr++;

        // Opens image file if one exists
        if (findAImg(pathname, answer, aiext, i))
        {
            char image_name[32];

            memcpy(responseptr, "1 ", 2);
            responseptr += 2;
            memcpy(responseptr, aiext, 3);
            responseptr += 3;
            memcpy(responseptr, " ", 1);
            responseptr += 1;

            strcpy(image_name, answer);
            strcat(image_name, ".");
            strcat(image_name, aiext);

            aisize = getSizeOfFile(pathname, image_name);

            ressize += aisize;
            realloc_aux = responseptr - response;
            if ((response = (char *)realloc(response, ressize * sizeof(char))) == NULL)
                return -1;
            responseptr = response + realloc_aux;

            sprintf(aux, "%ld", aisize);
            memcpy(responseptr, aux, strlen(aux));
            responseptr += strlen(aux);
            memcpy(responseptr, " ", 1);
            responseptr += 1;

            // Opens image file
            strcpy(path_aux, pathname);
            strcat(path_aux, "/");
            strcat(path_aux, image_name);
            if ((fd = open(path_aux, O_RDONLY)) < 0)
                return -1;
            read_full(fd, &responseptr, aisize);
            if (close(fd) < 0)
                return -1;
        }
        else
        {
            memcpy(responseptr, "0", 1);
            responseptr += 1;
        }
    }
    memcpy(responseptr, "\n", 1);
    responseptr += 1;
    write_full(connfd, response, (long)(responseptr - response + 1));
    printf("success\n");
    free(response);
    free(buffer);
    return 0;
}

int question_submit(int connfd, char *buffer)
{
    int question_count, quserid, n, fd;
    long qsize, realloc_aux, buffer_size = 2048;
    char *ptr, *bufferptr;
    char topic[16], question[16], pathname[128], path_aux[128], question_folder[32];

    bufferptr = &buffer[4];

    // Reads until 4th space (between qsize and qdata)
    readUntil(connfd, &bufferptr, 4, ' ');

    // Scans command information
    sscanf(buffer, "QUS %d %s %s %ld", &quserid, topic, question, &qsize);

    realloc_aux = bufferptr - buffer;
    if ((buffer = (char *)realloc(buffer, (buffer_size + qsize) * sizeof(char))) == NULL)
        return -1;
    bufferptr = buffer + realloc_aux;

    printf("User %d is trying to submit question %s in topic %s... ", quserid, question, topic);

    if (strlen(topic) > 10 || !isAlphanumeric(topic))
    {
        printf("invalid\n");
        write_full(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
        free(buffer);
        return 0;
    }

    // Opens topic folder
    findTopic(topic, pathname);

    // Counts number of questions in topic and checks for duplicate
    question_count = getQuestionCount(pathname, question);

    if (question_count >= 99)
    {
        printf("full\n");
        write_full(connfd, "QUR FUL\n", strlen("QUR FUL\n"));
        free(buffer);
        return 0;
    }
    else if (question_count == -1) // Found a duplicate
    {
        printf("duplicate\n");
        write_full(connfd, "QUR DUP\n", strlen("QUR DUP\n"));
        free(buffer);
        return 0;
    }

    // Creates question folder
    if (question_count < 9)
        sprintf(question_folder, "0%d_%s_%d", question_count + 1, question, quserid);
    else
        sprintf(question_folder, "%d_%s_%d", question_count + 1, question, quserid);
    strcat(pathname, "/");
    strcat(pathname, question_folder);

    if (mkdir(pathname, 0666) == -1) // Enables R/W for all users
        return -1;

    // Creates answer count file
    strcpy(path_aux, pathname);
    strcat(path_aux, "/anscount.txt");
    if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
        return -1;
    if (write(fd, "0", 1) != 1)
        return -1;
    if (write(fd, "\n", 1) != 1)
        return -1;
    if (close(fd) < 0)
        return -1;

    // Creates question info file
    strcpy(path_aux, pathname);
    strcat(path_aux, "/qinfo.txt");
    if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
        return -1;
    ptr = bufferptr;
    read_full(connfd, &bufferptr, qsize);
    write_full(fd, ptr, qsize);

    if (close(fd) < 0)
        return -1;

    // Reads space between qdata and qIMG
    if ((n = read(connfd, bufferptr, 1)) <= 0)
        return -1;
    bufferptr += n;

    // Reads qIMG
    if ((n = read(connfd, bufferptr, 1)) <= 0)
        return -1;
    bufferptr += n;

    if (*(bufferptr - n) == '1') // Has image to read
    {
        char *ptr_aux = bufferptr;
        char iext[4];
        long isize;

        readUntil(connfd, &bufferptr, 3, ' ');

        sscanf(ptr_aux, " %s %ld ", iext, &isize);

        realloc_aux = bufferptr - buffer;
        if ((buffer = (char *)realloc(buffer, (buffer_size + qsize + isize) * sizeof(char))) == NULL)
            return -1;
        bufferptr = buffer + realloc_aux;

        strcpy(path_aux, pathname);
        strcat(path_aux, "/qimg.");
        strcat(path_aux, iext);

        if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
            return -1;

        ptr = bufferptr;
        read_full(connfd, &bufferptr, isize);
        write_full(fd, ptr, isize);

        if (close(fd) < 0)
            return -1;

        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        if (*bufferptr == '\n')
        {
            printf("success\n");
            write_full(connfd, "QUR OK\n", strlen("QUR OK\n"));
            free(buffer);
            return 0;
        }
        else
        {
            printf("failure\n");
            write_full(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
            free(buffer);
            return 0;
        }
    }
    else if (*(bufferptr - n) == '0') // Has no image to read
    {
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        if (*bufferptr == '\n')
        {
            printf("success\n");
            write_full(connfd, "QUR OK\n", strlen("QUR OK\n"));
            free(buffer);
            return 0;
        }
        else
        {
            printf("failure\n");
            write_full(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
            free(buffer);
            return 0;
        }
    }
    else
    {
        printf("failure\n");
        write_full(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
        free(buffer);
        return 0;
    }
}

int answer_submit(int connfd, char *buffer)
{
    int answer_count = 0;
    int auserid, n, fd;
    long asize, realloc_aux, buffer_size = 2048;
    char *ptr, *bufferptr;
    char topic[16], question[16], pathname[128], path_aux[128], ansf[4], answer_name[32];

    bufferptr = &buffer[4];

    readUntil(connfd, &bufferptr, 4, ' ');

    // Scans command information
    sscanf(buffer, "ANS %d %s %s %ld", &auserid, topic, question, &asize);

    realloc_aux = bufferptr - buffer;
    if ((buffer = (char *)realloc(buffer, (buffer_size + asize) * sizeof(char))) == NULL)
        return -1;
    bufferptr = buffer + realloc_aux;

    printf("User %d is trying to submit answer for question %s in topic %s... ", auserid, question, topic);

    // Opens topic folder
    findTopic(topic, pathname);

    // Opens question folder
    findQuestion(question, pathname, NULL);

    // Checking number of answers
    answer_count = getAnswerCount(pathname);

    if (answer_count >= 99)
    {
        write_full(connfd, "ANR FUL\n", strlen("ANR FUL\n"));
        free(buffer);
        return 0;
    }
    else
    {
        strcpy(path_aux, pathname);
        strcat(path_aux, "/anscount.txt");
        if ((fd = open(path_aux, O_WRONLY | O_TRUNC)) < 0)
            return -1;
        answer_count++;
        sprintf(ansf, "%d\n", answer_count);

        write_full(fd, ansf, strlen(ansf));

        if (close(fd) < 0)
            return -1;
    }

    // Creates answer text file
    strcpy(path_aux, pathname);
    strcat(path_aux, "/");
    if (answer_count > 9)
        sprintf(answer_name, "%d_%d.txt", answer_count, auserid);
    else
        sprintf(answer_name, "0%d_%d.txt", answer_count, auserid);
    strcat(path_aux, answer_name);

    if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
        return -1;

    ptr = bufferptr;
    read_full(connfd, &bufferptr, asize);
    write_full(fd, ptr, asize);

    if (close(fd) < 0)
        return -1;

    // Reads space between adata and aIMG
    if ((n = read(connfd, bufferptr, 1)) <= 0)
        return -1;
    bufferptr += n;

    // Reads aIMG
    if ((n = read(connfd, bufferptr, 1)) <= 0)
        return -1;
    bufferptr += n;

    if (*(bufferptr - n) == '1') // Has image to read
    {
        char iext[4];
        long isize;

        ptr = bufferptr;
        readUntil(connfd, &bufferptr, 3, ' ');

        sscanf(ptr, " %s %ld ", iext, &isize);

        realloc_aux = bufferptr - buffer;
        if ((buffer = (char *)realloc(buffer, (buffer_size + asize + isize) * sizeof(char))) == NULL)
            return -1;
        bufferptr = buffer + realloc_aux;

        strcpy(path_aux, pathname);
        strcat(path_aux, "/");
        if (answer_count > 9)
            sprintf(answer_name, "%d_%d.%s", answer_count, auserid, iext);
        else
            sprintf(answer_name, "0%d_%d.%s", answer_count, auserid, iext);
        strcat(path_aux, answer_name);

        if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
            return -1;

        ptr = bufferptr;
        read_full(connfd, &bufferptr, isize);
        write_full(fd, ptr, isize);

        if (close(fd) < 0)
            return -1;

        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        if (*bufferptr == '\n')
        {
            printf("success\n");
            write_full(connfd, "ANR OK\n", strlen("ANR OK\n"));
            free(buffer);
            return 0;
        }
        else
        {
            printf("failure\n");
            write_full(connfd, "ANR NOK\n", strlen("ANR NOK\n"));
            free(buffer);
            return 0;
        }
    }
    else if (*(bufferptr - n) == '0') // Has no image to read
    {
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        if (*bufferptr == '\n')
        {
            printf("success\n");
            write_full(connfd, "ANR OK\n", strlen("ANR OK\n"));
            free(buffer);
            return 0;
        }
        else
        {
            printf("failure\n");
            write_full(connfd, "ANR NOK\n", strlen("ANR NOK\n"));
            free(buffer);
            return 0;
        }
    }
    else
    {
        printf("failure\n");
        write_full(connfd, "ANR NOK\n", strlen("ANR NOK\n"));
        free(buffer);
        return 0;
    }
}

int receiveudp(int udpfd, char *buffer, struct sockaddr_in *cliaddr, socklen_t *len)
{
    char request[128], response[2048];
    int id, n;

    if ((n = recvfrom(udpfd, buffer, 2048, 0, (struct sockaddr *)cliaddr, len)) == -1)
        return -1;
    buffer[n] = '\0'; // Appends a '\0' to the message so it can be used in strcmp
    sscanf(buffer, "%s", request);

    //Register user
    if (!strcmp(request, "REG"))
    {
        if (register_user(buffer, &id, response, cliaddr) == -1)
            exit(1);
    }
    //List topics
    else if (!strcmp(request, "LTP"))
    {
        if (topic_list(response) == -1)
            exit(1);
    }
    //Propose topics
    else if (!strcmp(request, "PTP"))
    {
        if (topic_propose(buffer, &id, response) == -1)
            exit(1);
    }
    //List questions
    else if (!strcmp(request, "LQU"))
    {
        if (question_list(buffer, response) == -1)
            exit(1);
    }
    //Error
    else
        strcpy(response, "ERR\n");

    return sendto(udpfd, response, strlen(response), 0, (struct sockaddr *)cliaddr, *len);
}

int receivetcp(int connfd, struct sockaddr_in *cliaddr, socklen_t *len)
{
    char request[128];
    char *bufferptr, *buffer;

    if ((buffer = (char *)malloc(2048 * sizeof(char))) == NULL)
        exit(1);
    bufferptr = &buffer[0];

    readUntil(connfd, &bufferptr, 1, ' ');
    sscanf(buffer, "%s ", request);

    //Gets question
    if (!strcmp(request, "GQU"))
        return question_get(connfd, buffer);
    //Submits question
    else if (!strcmp(request, "QUS"))
        return question_submit(connfd, buffer);
    //Submits answer
    else if (!strcmp(request, "ANS"))
        return answer_submit(connfd, buffer);
    //Error
    else
    {
        printf("Unexpected protocol message received\n");
        write_full(connfd, "ERR\n", strlen("ERR\n"));
        free(buffer);
        return 0;
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd, udpfd, nready, maxfdp1, errcode;
    char buffer[2048], port[16];
    fd_set rset;
    struct sockaddr_in cliaddr;
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;
    socklen_t len;
    struct sigaction act, act2;
    pid_t childpid;
    DIR *dir;
    struct dirent *entry;

    memset(buffer, 0, sizeof buffer);

    memset(&hintstcp, 0, sizeof hintstcp);
    hintstcp.ai_family = AF_INET;
    hintstcp.ai_socktype = SOCK_STREAM;
    hintstcp.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    memset(&hintsudp, 0, sizeof hintsudp);
    hintsudp.ai_family = AF_INET;
    hintsudp.ai_socktype = SOCK_DGRAM;
    hintsudp.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1)
        exit(1);
    memset(&act2, 0, sizeof act);
    act2.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &act2, NULL) == -1)
        exit(1);

    strcpy(port, "58039");
    setPort(argc, argv, port);

    if ((dir = opendir("server/topics")) == NULL)
        exit(1);
    while ((entry = readdir(dir)) != NULL)
        topic_amount++;
    topic_amount -= 2; // Disregards directories . and ..
    closedir(dir);

    if ((errcode = getaddrinfo(NULL, port, &hintstcp, &restcp)) != 0)
        exit(1);
    if ((errcode = getaddrinfo(NULL, port, &hintsudp, &resudp)) != 0)
        exit(1);

    /* tcp socket */
    if ((listenfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1)
        exit(1);
    if (bind(listenfd, restcp->ai_addr, restcp->ai_addrlen) == -1)
        exit(1);
    if (listen(listenfd, 5) == -1)
        exit(1);

    /* udp socket */
    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1)
        exit(1);
    if ((errcode = bind(udpfd, resudp->ai_addr, resudp->ai_addrlen)) == -1)
        exit(1);

    FD_ZERO(&rset);
    maxfdp1 = max(listenfd, udpfd) + 1;

    while (1)
    {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        if ((nready = select(maxfdp1 + 1, &rset, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL)) <= 0)
            exit(1);

        //Receives from TCP socket
        if (FD_ISSET(listenfd, &rset))
        {
            len = sizeof(cliaddr);
            do
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
            while (connfd == -1 && errno == EINTR);
            if (connfd == -1)
                exit(1);

            if ((childpid = fork()) == 0)
            {
                close(listenfd);
                if (receivetcp(connfd, &cliaddr, &len) == -1)
                    exit(1);
                else
                    exit(0);
            }
            else if (childpid == -1)
                exit(1);
            do
                errcode = close(connfd);
            while (errcode == -1 && errno == EINTR);
            if (errcode == -1)
                exit(1);
        }
        //Receives from UDP socket
        if (FD_ISSET(udpfd, &rset))
        {
            len = sizeof(cliaddr);
            memset(buffer, 0, sizeof buffer);
            if (receiveudp(udpfd, buffer, &cliaddr, &len) == -1)
                exit(1);
        }
    }
    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    close(listenfd);
    close(udpfd);
    exit(0);
}
