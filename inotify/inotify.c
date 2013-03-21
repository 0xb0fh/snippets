#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/inotify.h>
#include <signal.h>
#include <errno.h>

#define debug_log(fmt, ...) fprintf(stderr, "%s:%s:%d "fmt,__FILE__,__func__,__LINE__, ## __VA_ARGS__)
#define fatal(fmt, ...) do { debug_log("FATAL: "fmt, ## __VA_ARGS__); exit(1); } while (0)

#define EVENT_SIZE  (sizeof (struct inotify_event))
#define IEB_SIZE     0x1000

#define LOOP_CONTINUE 1
#define LOOP_TERMINATE 0


unsigned short main_loop;


char *ievent_masks[] = 
  {
    "IN_ACCESS",
    "IN_MODIFY",
    "IN_ATTRIB",
    "IN_CLOSE_WRITE",
    "IN_CLOSE_NOWRITE",  
    "IN_OPEN",
    "IN_MOVED_FROM",     
    "IN_MOVED_TO",
    "IN_CREATE",
    "IN_DELETE",         
    "IN_DELETE_SELF",
    "IN_MOVE_SELF",
  };


int get_in_events(int fd)
{
  char inot_evt_buf[IEB_SIZE];
  int len, next_evt = 0;
  struct inotify_event *event;
  
  
  len = read (fd, inot_evt_buf, IEB_SIZE);
  if (len < 0) 
    return len;

  while (next_evt < len) {
    event = (struct inotify_event *) &inot_evt_buf[next_evt];
    
    debug_log("wd=%d mask=%s cookie=%d len=%d\n",
	      event->wd, ievent_masks[ffs(event->mask) - 1],
	      event->cookie, event->len);
    
    if (event->len)
      printf ("name=%s\n", event->name);
    
    next_evt += EVENT_SIZE + event->len;
  }
}


void check_in_events(int fd)
{
  fd_set rfds;
  sigset_t mask;
  sigset_t orig_mask;
 
  sigemptyset (&mask);
  sigaddset (&mask, SIGTERM);
 
  if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) 
    debug_log("sigprocmask()");
  
  
  while(main_loop){
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    
    if(pselect (FD_SETSIZE, &rfds, NULL, NULL, NULL, &orig_mask) == -1){
      if(errno == EINTR) 
	continue;
      
      fatal("select() failed\n");
    }
    
    get_in_events(fd);
  }
}

int add_to_watch(int fd, const char *fname, unsigned long mask)
{
  int wd;
  
  wd = inotify_add_watch(fd, fname, mask);
  if (wd < 0)
    fatal("Unable to add to watch::wd=%d\n", wd);
    
  debug_log("Added to watch: %s::wd=%d\n", fname, wd);

  return wd;
}


void signal_hl(int signum)
{
  debug_log("Signal: %d received.\n\nTerminating...\n", signum);
  main_loop = LOOP_TERMINATE;
}


void usage(char *me)
{
  printf("Usage: %s <dir>\n", me);
  exit(1);
}


int main (int argc, char **argv)
{
  int fd, wd;
  
  main_loop = LOOP_CONTINUE;
  
  if (argc != 2)
    usage(argv[0]);
  
  /* 
     "This is because non-job-control shells often ignore certain signals when starting children, 
     and it is important for the children to respect this."
  */
  if (signal (SIGINT, signal_hl) == SIG_IGN){
    signal (SIGINT, SIG_IGN);
    debug_log("Signal SIGINT previously ignored, ignoring...\n");
  }

  fd = inotify_init();
  
  if (fd < 0)
    fatal("Error while initializing::FD=%d\n",fd);
  

  wd = add_to_watch(fd, argv[1], IN_ALL_EVENTS/*IN_MODIFY | IN_CREATE | IN_DELETE*/);
  check_in_events(fd);
  close(fd);

  return 0;
}

