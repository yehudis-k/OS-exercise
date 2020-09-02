#include <dirent.h>
#include <stdio.h>
#include <fnmatch.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef struct queue {
    char filename[100];
    struct queue* next;
}queue;

struct sigaction sa;

queue* head;
queue* tail;
pthread_mutex_t queue_mutex;
pthread_mutex_t dir_mutex;
pthread_cond_t cond;
pthread_t* thread_arr;
int num_threads, active_threads;
int num_found_files = 0;

void my_handler(int sig)
{
    printf("Search stopped, found %d files\n", num_found_files);
    while (head != NULL)
    {
        queue* tmp = head;
        head = head->next;
        free(tmp);
    }
    for (int i = 0; i < num_threads; i++)
    {
        pthread_cancel(thread_arr[i]);
    }
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&dir_mutex);
    pthread_cond_destroy(&cond);
    free(thread_arr);
    exit(0);
}

int name_corresponds(const char* path, const char* substring)
{
    char sub[strlen(substring) + 2];
    strcpy(sub, "*");
    strcat(sub, substring);
    strcat(sub, "*");
    return fnmatch(sub,strrchr(path,'/') + 1 ,0) == 0;
}

void treat_file(const char* path, const char* substring)
{
    if (name_corresponds(path, substring))
    {
        printf("%s\n", path);
        num_found_files++;
    }
}


void* browse(void* a)
{
    int e;
    char* substring = (char*)a;
    while (1)
    {
        pthread_mutex_lock(&queue_mutex);
        active_threads--;
        while (head == NULL)
        {
            if(active_threads == 0)
            {
                pthread_mutex_unlock(&queue_mutex);
                pthread_cond_signal(&cond);
                pthread_testcancel();
                pthread_exit(NULL);
            }
            pthread_cond_wait(&cond, &queue_mutex);
        }
        active_threads++;
        char path[strlen(head->filename)];
        strcpy(path, head->filename);
        queue* tmp = head;
        head = head->next;
        if(tmp == tail)
            tail = NULL;
        free(tmp);
        pthread_mutex_unlock(&queue_mutex);
        DIR *dir = opendir(path);
        struct dirent *entry;
        struct stat dir_stat;
        pthread_testcancel();
        if (!dir) {
            fprintf(stderr, "Error! open dir failed\n");
            active_threads--;
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&dir_mutex);
        while((entry = readdir(dir)) != NULL) {
            pthread_mutex_unlock(&dir_mutex);
            char buff[strlen(path)+strlen(entry->d_name)+2];
            sprintf(buff, "%s/%s", path, entry->d_name);
            e = stat(buff, &dir_stat);
            pthread_testcancel();
            if (e == -1)
            {
                fprintf(stderr, "Error! stat failed\n");
                active_threads--;
                pthread_exit(NULL);
            }
            if(strcmp(entry->d_name, "..") != 0){
                if(((dir_stat.st_mode & __S_IFMT) == __S_IFDIR) && strcmp(entry->d_name, ".") != 0)
                {
                    pthread_mutex_lock(&queue_mutex);
                    pthread_testcancel();
                    pthread_cond_signal(&cond);
                    if (tail == NULL)
                    {
                        head = malloc(sizeof(queue));
                        if (head == NULL)
                        {
                            fprintf(stderr, "Error! malloc failed\n");
                            active_threads--;
                            pthread_exit(NULL);
                        }
                        strcpy(head->filename, buff);
                        head->next = NULL;
                        tail = head;
                    }
                    else
                    {
                        tail->next=malloc(sizeof(queue));
                        if (tail->next == NULL)
                        {
                            fprintf(stderr, "Error! malloc failed\n");
                            active_threads--;
                            pthread_exit(NULL);
                        }
                        tail=tail->next;
                        strcpy(tail->filename, buff);
                        tail->next=NULL;
                    }
                    pthread_cond_signal(&cond);
                    pthread_mutex_unlock(&queue_mutex);
                }
                else
                    treat_file(buff, substring);
            }
            pthread_mutex_lock(&dir_mutex);
        }
        pthread_mutex_unlock(&dir_mutex);
        closedir(dir);
    }
}

int main(int argc, char **argv)
{
    if (argc < 4)
        exit(1);

    sa.sa_handler = my_handler;
    sigaction(SIGINT, &sa, NULL);

    int e;
    e = pthread_mutex_init(&queue_mutex, NULL);
    if (e !=0)
    {
        fprintf(stderr, "Error! creating mutex failed\n");
        exit(1);
    }
    e = pthread_mutex_init(&dir_mutex, NULL);
    if (e !=0)
    {
        fprintf(stderr, "Error! creating mutex failed\n");
        exit(1);
    }
    e = pthread_cond_init(&cond, NULL);
    if (e !=0)
    {
        fprintf(stderr, "Error! creating cond failed\n");
        exit(1);
    }

    head = malloc(sizeof(queue));
    if (head == NULL)
    {
        fprintf(stderr, "Error! malloc failed\n");
        exit(1);
    }
    strcpy(head->filename, argv[1]);
    head->next = NULL;
    tail = head;

    num_threads = atoi(argv[3]);
    active_threads = num_threads;
    thread_arr = malloc(sizeof(pthread_t) * num_threads);
    if (thread_arr == NULL)
    {
        fprintf(stderr, "Error! malloc failed\n");
        exit(1);
    }
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&thread_arr[i], NULL, browse, argv[2]);
    }
    for (int i=0; i < num_threads; i++)
    {
        pthread_join(thread_arr[i], NULL);
    }
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&dir_mutex);
    pthread_cond_destroy(&cond);
    free(thread_arr);
    printf("Done searching, found %d files\n", num_found_files);
    return 0;
}