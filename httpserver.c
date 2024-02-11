#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>

#include "libhttp.h"
#include "wq.h"
#include "libword.h"

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
wq_t work_queue;
int num_threads;
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;

pthread_mutex_t mutex_queue;
pthread_cond_t cond_queue;

struct input {
   int fd;
   int target_fd;
} input;

void list_directory(char* path,char* server_files_directory,char* final_string){
    char* directory_path = malloc(sizeof(char) * 50);
    strcpy(directory_path, "./");
    strcat(directory_path, server_files_directory);
    strcat(directory_path, "/");
    strcat(directory_path, path);
    struct dirent *de;  // Pointer for directory entry 
  
    // opendir() returns a pointer of DIR type.  
    DIR *dr = opendir(directory_path); 
  
    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
    { 
        printf("Could not open current directory\n" ); 
        return; 
    } 
  
    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html 
    // for readdir() 
    while ((de = readdir(dr)) != NULL){
       char* path_copy = malloc(sizeof(char) * 400);
       strcpy(path_copy, path);
       strcat(path_copy, "/");
       strcat(path_copy, de->d_name);

       strcat(final_string, "<a href=\"/");
       strcat(final_string, path_copy);
       strcat(final_string, "\">");
       strcat(final_string, de->d_name);

       strcat(final_string, "</a><br>");
       
       free(path_copy);   
    } 
  
    closedir(dr);     
}
int is_regular_file(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int file_exists(char *filename){ //0 -> false 1 -> exist
    FILE *fp = fopen(filename, "r");
    int is_exist = 0;
    if (fp != NULL)
    {
        is_exist = 1;
        fclose(fp); // close the file
    }
    return is_exist;
}

/*
 * Serves the contents the file stored at `path` to the client socket `fd`.
 * It is the caller's reponsibility to ensure that the file stored at `path` exists.
 * You can change these functions to anything you want.
 * 
 * ATTENTION: Be careful to optimize your code. Judge is
 *            sesnsitive to time-out errors.
 */
void serve_file(int fd, char *path) {
   FILE *fptr;

   // Open a file in read mode
   fptr = fopen(path, "r");

   // Store the content of the file
   char myString[500];
   char final_string[50000];
   strcpy(final_string, "");

   while(fgets(myString, 500, fptr)) {
      strcat(final_string, myString);
   }

   // Close the file
   fclose(fptr);

  
   int string_len_int = counter(final_string);  
   char* string_len_str = int_to_str(string_len_int);
   
   http_start_response(fd, 200);
   http_send_header(fd, "Content-Type", http_get_mime_type(path));
   http_send_header(fd, "Content-Length", string_len_str); // Change this too
   http_end_headers(fd);

  /* TODO: PART 1 Bullet 2 */
   http_send_string(fd,final_string);
   
   
   close(fd);
}

void serve_directory(int fd, char *path) {
  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", http_get_mime_type(".html"));
  http_end_headers(fd);

  /* TODO: PART 1 Bullet 3,4 */

}


/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 * 
 *   Closes the client socket (fd) when finished.
 */
void handle_files_request() {
  while(1){
     pthread_mutex_lock(&mutex_queue);
     while(work_queue.size == 0){
        pthread_cond_wait(&cond_queue, &mutex_queue);
     }
     int fd = wq_pop(&work_queue);
     pthread_mutex_unlock(&mutex_queue);

  struct http_request *request = http_request_parse(fd);
  if (request == NULL || request->path[0] != '/') {
    http_start_response(fd, 400);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);
    continue;
  }
  if (strstr(request->path, "..") != NULL) {
    http_start_response(fd, 403);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);
    continue;
  }

  /* Remove beginning `./` */
  char *path = malloc(2 + strlen(request->path) + 1);
  //path[0] = '.';
  //path[1] = '/';
  memcpy(path, request->path, strlen(request->path) + 1);

  /* 
   * TODO: First is to serve files. If the file given by `path` exists,
   * call serve_file() on it. Else, serve a 404 Not Found error below.
   *
   * TODO: Second is to serve both files and directories. You will need to
   * determine when to call serve_file() or serve_directory() depending
   * on `path`.
   *  
   * Feel FREE to delete/modify anything on this function.
   */
   char cwd_path[128];
   getcwd(cwd_path, 128);
   memmove(path, path+1, strlen(path));
   char true_path[128];
   strcpy (true_path, cwd_path);
   strcat (true_path, "/");
   strcat (true_path, server_files_directory);
   strcat (true_path, path);
   char index_path[200];
   strcpy(index_path, true_path);
   strcat(index_path, "/index.html");
   
   if (is_regular_file(true_path)) {
    // file exists
      serve_file(fd, true_path);
      free(path);
      continue;
   } else if(access(index_path, F_OK) == 0){
        serve_file(fd, index_path);
        free(path);
        continue;
        
     } else if(access(true_path, F_OK) == 0){
    // file is a directory (exist)
       char* final_string = malloc(sizeof(char) * 4000);
       strcpy(final_string, "");
       slash_remover(path);
       list_directory(path,server_files_directory,final_string);
       strcat(path, "/..");
       strcat(final_string, "<a href=\"/");
       strcat(final_string, path);
       strcat(final_string, "\">");
       strcat(final_string, "Parent directory</a>");
       
       int string_len_int = counter(final_string);  
       char* string_len_str = int_to_str(string_len_int);

       http_start_response(fd, 200);
       http_send_header(fd, "Content-Type", "text/html");
       http_send_header(fd, "Content-Length", string_len_str);
       http_end_headers(fd);
       http_send_string(fd,final_string);

      // list_directory();
       
       close(fd);
       free(final_string);
       free(path);
       continue;
   }
     else{
       http_start_response(fd, 404);
       http_send_header(fd, "Content-Type", "text/html");
       http_end_headers(fd);
       close(fd);
       free(path);
       continue;
     }
/*
  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", "text/html");
  http_end_headers(fd);
  http_send_string(fd,
      "<center>"
      "<h1>Welcome to httpserver!</h1>"
      "<hr>"
      "<p>Nothing's here yet.</p>"
      "</center>");

  close(fd);
*/
    //return;

  }//while(1)
}

/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void listen_and_answer(struct input* input1){
   int fd = input1 -> fd;
   int target_fd = input1 -> target_fd;
   while(1){
      ssize_t valread;
      char buffer[1024] = {0};
      int res = 0;

      valread = read(fd, buffer,1024 - 1);
      if((int)valread <= 0){
         close(fd);
         close(target_fd);
         return;
      }
         char* pointer = strstr(buffer, "Content-Length: ");
         if(pointer != NULL){
            int count = 16;
            char result[10];
            strcpy(result, "");
            while(*(pointer + count) != '\n'){
            result[count - 16] = *(pointer + count);
            count++;
            }
            res = atoi(result);
         }
         send(target_fd, buffer,strlen(buffer), 0);
         while((int)valread < res){
            valread += read(fd, buffer,1024 - 1);
            send(target_fd, buffer,strlen(buffer), 0);
         }
   }
}
void handle_proxy_request() {
  while(1){
     pthread_mutex_lock(&mutex_queue);
     while(work_queue.size == 0){
        pthread_cond_wait(&cond_queue, &mutex_queue);
     }
     int fd = wq_pop(&work_queue);
     pthread_mutex_unlock(&mutex_queue);
   // printf("4\n");
  /*
  * The code below does a DNS lookup of server_proxy_hostname and 
  * opens a connection to it. Please do not modify.
  */

  struct sockaddr_in target_address;
  memset(&target_address, 0, sizeof(target_address));
  target_address.sin_family = AF_INET;
  target_address.sin_port = htons(server_proxy_port);

  struct hostent *target_dns_entry = gethostbyname2(server_proxy_hostname, AF_INET);

  int target_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (target_fd == -1) {
    fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
    close(fd);
    exit(errno);
  }

  if (target_dns_entry == NULL) {
    fprintf(stderr, "Cannot find host: %s\n", server_proxy_hostname);
    close(target_fd);
    close(fd);
    exit(ENXIO);
  }

  char *dns_address = target_dns_entry->h_addr_list[0];

  memcpy(&target_address.sin_addr, dns_address, sizeof(target_address.sin_addr));
  int connection_status = connect(target_fd, (struct sockaddr*) &target_address,
      sizeof(target_address));

  if (connection_status < 0) {
    /* Dummy request parsing, just to be compliant. */
    http_request_parse(fd);

    http_start_response(fd, 502);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    http_send_string(fd, "<center><h1>502 Bad Gateway</h1><hr></center>");
    close(target_fd);
    close(fd);
    return;

  }
  /* 
  * TODO: Your solution for task 3 belongs here! 
  */
//this thread will listen on (client - > proxy) requests

    struct timeval timeout;      
    timeout.tv_sec = 0.5;
    timeout.tv_usec = 0;
    
    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0)
        perror("setsockopt failed\n");

    if (setsockopt (fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0)
        perror("setsockopt failed\n");

    if (setsockopt (target_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0)
        perror("setsockopt failed\n");

    if (setsockopt (target_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0)
        perror("setsockopt failed\n");


    struct input input1;
    input1.fd = target_fd;
    input1.target_fd = fd;

    pthread_t thread;
    pthread_create(&thread, NULL, &listen_and_answer, &input1);

    struct input input2;
    input2.fd = fd;
    input2.target_fd = target_fd;
    listen_and_answer(&input2);

   close(fd);
   close(target_fd);
     
   }//while(1)
}

void init_thread_pool(int num_threads, void (*request_handler)) {
  /*
   * TODO: Part of your solution for Task 2 goes here!
   */
   pthread_t thread[num_threads];
   for(int i = 0; i < num_threads; i++){
      if(pthread_create(&thread[i], NULL, request_handler, NULL) != 0)
          perror("Failed to create the thread\n");
   }
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *) &server_address,
        sizeof(server_address)) == -1) {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    perror("Failed to listen on socket");
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);
  
  init_thread_pool(num_threads, request_handler);

  while (1) {
    client_socket_number = accept(*socket_number,
        (struct sockaddr *) &client_address,
        (socklen_t *) &client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    // TODO: Change me?
    //
    pthread_mutex_lock(&mutex_queue);
    wq_push(&work_queue,client_socket_number);
    pthread_mutex_unlock(&mutex_queue);
    pthread_cond_signal(&cond_queue);
    //
    //request_handler(client_socket_number);
    //close(client_socket_number);

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);
  }

  shutdown(*socket_number, SHUT_RDWR);
  close(*socket_number);
}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000 [--num-threads 5]\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000 [--num-threads 5]\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_callback_handler);
  signal(SIGPIPE, SIG_IGN);

  /* Default settings */
  server_port = 8000;
  void (*request_handler) = NULL;


  //
  wq_init(&work_queue);
  pthread_mutex_init(&mutex_queue, NULL);
  pthread_cond_init(&cond_queue, NULL);
  //


  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--num-threads", argv[i]) == 0) {
      char *num_threads_str = argv[++i];
      if (!num_threads_str || (num_threads = atoi(num_threads_str)) < 1) {
        fprintf(stderr, "Expected positive integer after --num-threads\n");
        exit_with_usage();
      }
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
