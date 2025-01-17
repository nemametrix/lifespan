#ifndef NS_THREAD
#define NS_THREAD

#include "ns_ex.h"
#include "ns_managed_pointer.h"

#include <map>
#include <vector>
/*******************************************
ns_thread: a multi-platform thread library
This library provides a thin wrapper for POSIX
and Windows threads to allow generic threading
functionality on Windows, Linux, and OSX machines.
(C) Nicholas Stroustrup 2006
*******************************************/

#ifdef _WIN32 
	#include <winsock2.h>
	#include <windows.h>

	struct ns_thread_handle_closer{
		void operator()(const HANDLE & h);
	};

	struct ns_thread_handle{
		DWORD id;

		HANDLE handle(){return handle_;}

		ns_thread_handle():have_handle(false){};
		void set_handle(const HANDLE & h){have_handle = true;handle_manager.take(h);handle_ = h;}
		void set_unmanaged_handle(const HANDLE & h){handle_ = h;}
		ns_thread_handle(const ns_thread_handle & h);
		void close(){if (have_handle){handle_manager.release(handle_);have_handle = false;}}
		~ns_thread_handle(){close();}

		private:
		ns_managed_handle<HANDLE,ns_thread_handle_closer> handle_manager;
		HANDLE handle_;
		bool have_handle;
	};

	typedef DWORD ns_thread_return_type;

#else
//	#include <pthread.h>
	//#include <time.h>
	typedef pthread_t ns_thread_handle;	

	//typedef int ns_thread_handle;
	//typedef int ns_mutex_handle;
	typedef void * ns_thread_return_type;
#endif


void ns_simple_execute(const std::string & command,const std::vector<std::string> & args);



struct ns_timeout_process_handle{
	
	void terminate();
	#ifdef _WIN32 
		HANDLE process_handle;
	#else
	int pid,
		stdin_pipe,
		stdio_pipe,
		stderr_pipe;
	#endif
};

struct ns_process_termination_daemon{
	ns_process_termination_daemon():process_termination_lock("ns_process_termination_lock"),
									process_terminated(false),timeout_cancellation_requested(false),ptr_count(0){}
	void cancel_timeout();
	bool request_permission_to_terminate(bool caller_is_timeout_runner);
	bool process_has_been_terminated(){return process_terminated;}

	long ptr_count;
private:
	bool timeout_cancellation_requested;
	ns_lock process_termination_lock;
	bool process_terminated;
};

struct ns_process_termination_daemon_ptr{
	ns_process_termination_daemon_ptr():daemon(0){}
	ns_process_termination_daemon_ptr(ns_process_termination_daemon * d){acquire(d);}
	ns_process_termination_daemon_ptr(const ns_process_termination_daemon_ptr & d){acquire(d.daemon);}
	ns_process_termination_daemon & operator()();
	void acquire(ns_process_termination_daemon * d);
	void release();
	~ns_process_termination_daemon_ptr();
	private:
	ns_process_termination_daemon * daemon;
};

struct ns_timeout_runner{
	ns_timeout_runner(unsigned long timeout_,ns_timeout_process_handle & handle_,ns_process_termination_daemon_ptr & daemon_);
	void start_timeout();
	bool started;
	ns_lock start_lock;
private:
	void run();
	unsigned long timeout;
	ns_timeout_process_handle handle;
	static ns_thread_return_type run_timeout(void * d);
	ns_process_termination_daemon_ptr termination_daemon;
};
class ns_process_termination_manager{
public:
	ns_process_termination_manager();

	void start_timeout(unsigned long seconds,ns_timeout_process_handle & handle);
	void cancel_timeout();
	bool request_permission_to_terminate();
	bool process_has_been_terminated();
	~ns_process_termination_manager();
private:
	bool started_timeout;
	ns_process_termination_daemon_ptr termination_daemon;
};

struct ns_external_execute_options{
	ns_external_execute_options():take_stdout_handle(true),
		take_stderr_handle(true),
		take_stdin_handle(false),
		discard_stdout(false),
		discard_stderr(false),
		discard_stdin(false),
		binary(false){
			#ifndef _WIN32
			only_fork = false;
			#endif
		}

	bool take_stdout_handle,
		take_stderr_handle,
		take_stdin_handle,
		discard_stdout,
		discard_stderr,
		discard_stdin,
		binary;
	#ifndef _WIN32
	bool only_fork;
	#endif
};

class ns_external_execute{
public:	
	#ifndef _WIN32
	ns_external_execute():child_pid(-1){c_stdout[0] = c_stdout[1] = c_stderr[0] = c_stderr[1] = c_stdin[0] = c_stdin[1] =  -1;}
	#else
		ns_external_execute():closed(true){}
	#endif
	void wait_for_termination();
	bool run(const std::string & command, const std::string & parameters, const ns_external_execute_options & opt);
	//read/write to/from stdin/out
	unsigned int read_stdout(char * a,const unsigned int length);
	unsigned int write_stdin(char * a, const unsigned int length);
	//read/write to/from stderr
	unsigned int read_stderr(char * a,const unsigned int length);

	void terminate_process();
	void start_timeout(unsigned long seconds);

	//close stdin/stdout
	void finished_writing_to_stdin();
	void finished_reading_from_stdout();
	//close sterr
	void finished_reading_from_stderr();

	//close everything.
	void release_io();
	void throw_exception(ns_ex & ex){throw ex;}

	bool timed_out();
	
	~ns_external_execute();
private:
	ns_timeout_process_handle get_handle();
	#ifdef _WIN32 
	HANDLE hChildStdinRd, hChildStdinWr,  
		   hChildStdoutRd, hChildStdoutWr,
		   hChildStderrRd, hChildStderrWr;
	bool closed;
	PROCESS_INFORMATION proc_info;
	#else
	int child_pid;
	int c_stdout[2],
	    c_stderr[2],
	    c_stdin[2];
	#endif
	ns_external_execute_options opts;
	ns_process_termination_manager timeout_manager;
};

typedef enum{NS_THREAD_LOW,NS_THREAD_NORMAL,NS_THREAD_HIGH} ns_thread_priority;

typedef ns_thread_return_type (*ns_thread_function_pointer)(void *);

class ns_thread{

public:
	ns_thread(){};
	ns_thread(ns_thread_function_pointer fptr, void * args){this->run(fptr,args);}

	void run(ns_thread_function_pointer fptr, void * args);
	void set_priority(const ns_thread_priority p);
	void close();
	void block_on_finish(const unsigned int wait_time = 0);
	//only important on unix for efficiency reasons.
	//releases pthread internal structures if block_on_finish() will never be called
	void detach();
	//only works on Windows
	bool still_running();

	
	static unsigned long ns_get_process_id();

	static void sleep(const unsigned int seconds);
	static void sleep_microseconds(const unsigned int microseconds);

	//this generates a handle that is only valid as "self-referential"... it can't be
	//used by any other threads
	static ns_thread get_current_thread();

private:
	ns_thread_handle handle;
};

class ns_process_priority{
public:
	
	typedef enum{ns_above_normal,ns_below_normal,ns_high,ns_idle,ns_normal,ns_background,ns_realtime} ns_priority;
	ns_process_priority():current_priority(ns_normal){}
	bool set_priority(const ns_priority priority);

private:
	ns_priority current_priority;
};

#endif
