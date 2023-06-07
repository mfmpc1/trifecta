/*
 * ServerSocket.cpp
 *
 */

#include <Networking/ServerSocket.h>
#include <Networking/sockets.h>
#include "Exceptions/Exceptions.h"

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
using namespace std;

void* accept_thread(void* server_socket)
{
  ((ServerSocket*)server_socket)->accept_clients();
  return 0;
}

ServerSocket::ServerSocket(int Portnum) : portnum(Portnum), thread(0)
{
  struct sockaddr_in serv; /* socket info about our server */

  memset(&serv, 0, sizeof(serv));    /* zero the struct before filling the fields */
  serv.sin_family = AF_INET;         /* set the type of connection to TCP/IP */
  serv.sin_addr.s_addr = INADDR_ANY; /* set our address to any interface */
  serv.sin_port = htons(Portnum);    /* set the server port number */

  main_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (main_socket<0) { error("set_up_socket:socket"); }

  int one=1;
  int fl=setsockopt(main_socket,SOL_SOCKET,SO_REUSEADDR,(char*)&one,sizeof(int));
  if (fl<0) { error("set_up_socket:setsockopt"); }

  /* disable Nagle's algorithm */
  fl= setsockopt(main_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&one,sizeof(int));
  if (fl<0) { error("set_up_socket:setsockopt");  }

  octet my_name[512];
  memset(my_name,0,512*sizeof(octet));
  gethostname((char*)my_name,512);

  /* bind serv information to mysocket
   *   - Just assume it will eventually wake up
   */
  fl=1;
  while (fl!=0)
    { fl=::bind(main_socket, (struct sockaddr *)&serv, sizeof(struct sockaddr));
      if (fl != 0)
        { cerr << "Binding to socket on " << my_name << ":" << Portnum << " failed, trying again in a second ..." << endl;
          sleep(1);
        }
      else
        {
#ifdef DEBUG_NETWORKING
          cerr << "ServerSocket is bound on port " << Portnum << endl;
#endif
      }
    }
  if (fl<0) { error("set_up_socket:bind");  }


  /* start listening, allowing a queue of up to 1000 pending connection */
  fl=listen(main_socket, 1000);
  if (fl<0) { error("set_up_socket:listen");  }

  // Note: must not call virtual init() method in constructor: http://www.aristeia.com/EC3E/3E_item9.pdf
}

void ServerSocket::init()
{
  pthread_create(&thread, 0, accept_thread, this);
}

class ServerJob
{
  ServerSocket& server;
  int socket;
  struct sockaddr dest;

public:
  pthread_t thread;

  ServerJob(ServerSocket& server, int socket, struct sockaddr dest) :
      server(server), socket(socket), dest(dest), thread(0)
  {
  }

  static void* run(void* job)
  {
    auto& server_job = *(ServerJob*)(job);
    server_job.server.wait_for_client_id(server_job.socket, server_job.dest);
    return 0;
  }
};

ServerSocket::~ServerSocket()
{
  for (auto& job : jobs)
    {
      pthread_cancel(job->thread);
      pthread_join(job->thread, 0);
      delete job;
    }

  pthread_cancel(thread);
  pthread_join(thread, 0);
  if (close(main_socket)) { error("close(main_socket"); };
}

void ServerSocket::wait_for_client_id(int socket, struct sockaddr dest)
{
  (void) dest;
  int client_id;
  try
    {
      receive(socket, (unsigned char*) &client_id, sizeof(client_id));
      process_connection(socket, client_id);
    }
  catch (closed_connection&)
    {
#ifdef DEBUG_NETWORKING
      auto& conn = *(sockaddr_in*) &dest;
      fprintf(stderr, "client on %s:%d left without identification\n",
          inet_ntoa(conn.sin_addr), ntohs(conn.sin_port));
#endif
    }
}

void ServerSocket::accept_clients()
{
  while (true)
    {
      struct sockaddr dest;
      memset(&dest, 0, sizeof(dest));    /* zero the struct before filling the fields */
      int socksize = sizeof(dest);
      int consocket = accept(main_socket, (struct sockaddr *)&dest, (socklen_t*) &socksize);
      if (consocket<0) { error("set_up_socket:accept"); }

      int client_id;
      if (receive_all_or_nothing(consocket, (unsigned char*)&client_id, sizeof(client_id)))
        process_connection(consocket, client_id);
      else
        {
#ifdef DEBUG_NETWORKING
          auto& conn = *(sockaddr_in*) &dest;
          fprintf(stderr, "deferring client on %s:%d to thread\n",
              inet_ntoa(conn.sin_addr), ntohs(conn.sin_port));
#endif
          // defer to thread
          jobs.push_back(new ServerJob(*this, consocket, dest));
          pthread_create(&jobs.back()->thread, 0, ServerJob::run, jobs.back());
        }

#ifdef __APPLE__
      int flags = fcntl(consocket, F_GETFL, 0);
      int fl = fcntl(consocket, F_SETFL, O_NONBLOCK |  flags);
      if (fl < 0)
          error("set non-blocking");
#endif
    }
}

void ServerSocket::process_connection(int consocket, int client_id)
{
  data_signal.lock();
#ifdef DEBUG_NETWORKING
  cerr << "client " << hex << client_id << " is on socket " << dec << consocket
      << endl;
#endif
  process_client(client_id);
  clients[client_id] = consocket;
  data_signal.broadcast();
  data_signal.unlock();
}

int ServerSocket::get_connection_socket(int id)
{
  data_signal.lock();
  if (used.find(id) != used.end())
    {
      stringstream ss;
      ss << "Connection id " << hex << id << " already used";
      throw IO_Error(ss.str());
    }

  while (clients.find(id) == clients.end())
  {
      if (data_signal.wait(60) == ETIMEDOUT)
          throw runtime_error("No client after one minute");
  }

  int client_socket = clients[id];
  used.insert(id);
  data_signal.unlock();
  return client_socket;
}

void* anonymous_accept_thread(void* server_socket)
{
  ((AnonymousServerSocket*)server_socket)->accept_clients();
  return 0;
}

void AnonymousServerSocket::init()
{
  pthread_create(&thread, 0, anonymous_accept_thread, this);
}

void AnonymousServerSocket::process_client(int client_id)
{
  if (clients.find(client_id) != clients.end())
    close_client_socket(clients[client_id]);
  client_connection_queue.push(client_id);
}

int AnonymousServerSocket::get_connection_socket(int& client_id)
{
  data_signal.lock();

  //while (clients.find(next_client_id) == clients.end())
  while (client_connection_queue.empty())
      data_signal.wait();

  client_id = client_connection_queue.front();
  client_connection_queue.pop();
  int client_socket = clients[client_id];
  data_signal.unlock();
  return client_socket;
}