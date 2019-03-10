#include <stdlib.h>
#include <getopt.h>
#include "aux.h"
#include "umix.h"
#include "mycode4.h"
#include <string.h>

static void (*init_threads)() = MyInitThreads;
static int (*create_thread)(void (*f)(), int) = MyCreateThread;
static int (*yield_thread)(int) = MyYieldThread;
static int (*get_thread)() = MyGetThread;
static void (*sched_thread)() = MySchedThread;
static void (*exit_thread)() = MyExitThread;


#define NUMYIELDS 5
void createThreads(int);
void createThreadsYield(int);
void m_createThreadsYield(int);
void createThreadsSched(int);
void m_createThreadsSched(int);
void threadPrintYield(int);
void m_threadPrintYield(int);
void threadPrintNoYield(int);
void threadPrintSched(int);
void m_threadPrintSched(int);
void threadPrintOnce(int);
void threadExit(int);
int checkZeros(char arr[], int size);


static struct option opts[] = {
  {"expected", no_argument, NULL, 'e'},
  {"test", required_argument, NULL, 't'},
  {0}
};

/* TEST FUNCTIONS START HERE */

// Testing two threads
void test1() {
  int t, me;

  me = get_thread();
  t = create_thread(threadPrintYield, me);

  threadPrintYield(t);
  exit_thread();
}

// Testing two threads, but one yield and one sched
void test2() {
  int t, me;

  me = get_thread();
  t = create_thread(threadPrintYield, me);

  threadPrintSched(t);
  exit_thread();
}

// Testing two threads, with one yielding to itself
void test3() {
  int t, me;
  
  me = get_thread();
  t = create_thread(threadPrintYield, me);
  
  // Yield to the thread created to let it print once
  yield_thread(t);

  threadPrintYield(me);
  exit_thread();
}

// Test 5 threads, printing in the order 4 3 2 1 0
void test4() {
  int me, t1, t2, t3, t4;

  me = get_thread();
  t1 = create_thread(threadPrintYield, me);
  t2 = create_thread(threadPrintYield, t1);
  t3 = create_thread(threadPrintYield, t2);
  t4 = create_thread(threadPrintYield, t3);

  yield_thread(t4);
  threadPrintYield(t4);
  exit_thread();
}

// Test 5 threads, printing in the order 0 1 2 3 4 (FIFO)
void test5() {
  int me, t1, t2, t3, t4;

  me = get_thread();
  t1 = create_thread(threadPrintSched, 0);
  t2 = create_thread(threadPrintSched, 0);
  t3 = create_thread(threadPrintSched, 0);
  t4 = create_thread(threadPrintSched, 0);

  threadPrintSched(me);
  exit_thread();
}

// Test MAXTHREADS, printing in the order 9 8 7 ...
void test6() {
  int i, t;
  t = get_thread();
  for (i = 0; i < MAXTHREADS-1; i++) {
    t = create_thread(threadPrintYield, t);
  }

  yield_thread(t);
  threadPrintYield(t);
  exit_thread();
}

// Test MAXTHREADS, printing in the order 0 1 2 3 ... (FIFO)
void test7() {
  int i, t;
  t = get_thread();
  for (i = 0; i < MAXTHREADS-1; i++) {
    t = create_thread(threadPrintYield, 0);
  }

  threadPrintSched(0);
  exit_thread();
}

// Test thread other than thread 0 creating a new thread
// Thread 0 creates 1, 1 creates 2, 2 creates 3, ... so on
// Thread 0 yields to 1, 1 yields to 2, and last thread yields to 0
void test8() {
  int t, me; 
  me = get_thread();

  t = create_thread(createThreadsYield, me);
  threadPrintYield(t);
  exit_thread();
}

// Same as test 8, but using scheduling instead
// 0 creates 1, prints and schedules (1 starts) -> queue should now contain [0]
// 1 creates 2, prints and schedules (0 starts) -> queue: [2, 1]
// 0 prints and schedules (2 starts) -> queue: [1, 0]
// 2 creates 3, prints and schedules (1 starts) -> queue: [0, 3, 2]
// 1 prints and schedules (0 starts) -> queue: [3, 2, 1]
// 0 prints and schedules (3 starts) -> queue: [2, 1, 0]
// 3 creates 4, prints and schedules (2 starts) -> queue: [1, 0, 4, 3]
// so on .......
void test9() {
  int t, me;
  me = get_thread();

  t = create_thread(createThreadsSched, me);
  threadPrintSched(t);
  exit_thread();
}

// Thread 4 exits after 9 threads get created. Next thread gets created
// must have id 4
void test10() {
  int i;
  for (i = 1; i < MAXTHREADS; i++) {
    if (i == 4) {
      Printf("Created thread %d\n", create_thread(threadExit, 0));
    }
    else {
      Printf("Created thread %d\n", create_thread(threadPrintOnce, 0));
    }
  }
  yield_thread(4);
  int t = create_thread(threadExit, 0);
  Printf("Expected t: %d, actual t: %d\n", 4, t);
  exit_thread();
}

// Thread 0 - 6 come. Thread 5 exits. Thread 7-9 come.
// Then create one more thread which should have id 5
void test11() {
  int i;
  for (i = 1; i < 7; i++) {
    if (i == 5)
      Printf("Created thread %d\n", create_thread(threadExit, 0));
    else
      Printf("Created thread %d\n", create_thread(threadPrintOnce, 0));
  }

  // Let thread 5 exit
  yield_thread(5);

  for (i = 7; i < MAXTHREADS; i++) {
    Printf("Created thread %d\n", create_thread(threadPrintOnce, 0));
  }
  int t = create_thread(threadExit, 0);
  Printf("Expected t: %d, actual t: %d\n", 5, t);
  exit_thread();
}

// Testing thread 0 exits and then gets created again by another thread
// 0 creates 1 then exits -> 1 creates 9 more threads, one of which must be
// thread 0
void test12() {
  threadPrintNoYield(0);
  create_thread(createThreads, 0);
  exit_thread();
}

// Testing a higher number thread creating a lower number thread
// 0 creates 1. 0 yields to 1 and 1 exits. 0 then creates 2 and exits.
// 2 will create 9 more threads, with the last 2 it creates being thread 0
// and 1
void test13() {
  threadPrintNoYield(0);
  create_thread(threadExit, 0);
  yield_thread(1);
  create_thread(createThreads, 0);
  exit_thread();
}

// Testing creating more than MAXTHREADS
void test14() {
  int i;
  threadPrintNoYield(0);
  for (i = 0; i < MAXTHREADS; i++) {
    create_thread(threadPrintNoYield, 0);
  }
  exit_thread();
}


// Test 15, 16, 17, 18 are similar to 4, 5, 8, 9, but use a version of a
// the print function that allocates some stack memory
// This tests to see if stack space was allocated correctly and that the 
// memory of a thread's stack below won't overwrite the memory of a thread's 
// stack above
void test15() {
  int me, t1, t2, t3, t4;

  me = get_thread();
  t1 = create_thread(m_threadPrintYield, me);
  t2 = create_thread(m_threadPrintYield, t1);
  t3 = create_thread(m_threadPrintYield, t2);
  t4 = create_thread(m_threadPrintYield, t3);

  yield_thread(t4);
  m_threadPrintYield(t4);
  exit_thread();
}

void test16() {
  int me, t1, t2, t3, t4;

  me = get_thread();
  t1 = create_thread(m_threadPrintSched, 0);
  t2 = create_thread(m_threadPrintSched, 0);
  t3 = create_thread(m_threadPrintSched, 0);
  t4 = create_thread(m_threadPrintSched, 0);

  m_threadPrintSched(me);
  exit_thread();
}

void test17() {
  int t, me; 
  me = get_thread();

  t = create_thread(m_createThreadsYield, me);
  m_threadPrintYield(t);
  exit_thread();
}

void test18() {
  int t, me;
  me = get_thread();

  t = create_thread(m_createThreadsSched, me);
  m_threadPrintSched(t);
  exit_thread();
}

// When the last id assigned is 2, if thread 2 exits and then we call
// create thread. The new thread should be assigned 3, not 2.
void test19() {
  int me, t1, t2, t3;
  me = get_thread();
  t1 = create_thread(threadPrintSched, me);
  t2 = create_thread(threadExit, t1);
  yield_thread(t2);
  t3 = create_thread(threadPrintSched, me);
  Printf("Expected ID assigned: %d, Actual ID assigned\n", 3, t3);
  threadPrintSched(0);
  exit_thread();
}


// Print functions

// Create 9 threads
void createThreads(int t) {
  int i, t2;
  for (i = 0; i < MAXTHREADS-1; i++) {
    t2 = create_thread(threadPrintNoYield, get_thread());
    Printf("Thread %d created thread %d\n", get_thread(), t2);
  }
  
  threadPrintNoYield(0);
}

// Create a new thread that will yield to this thread
// This thread will yield to the thread that created it
void createThreadsYield(int t) {
  // Change this number to change the number of threads getting created
  // 3 will end up creating 5 threads in total (since 0 creates 1, so 1 will 
  // only need to create 3 more threads)
  static int numThreads = 3;
  int t2 = -1;

  if (numThreads != 0) {
    t2 = create_thread(createThreadsYield, get_thread());
    numThreads--;
  }

  if (t2 != -1) {
    threadPrintYield(t2);
  }
  else {
    // Last thread created yields to thread 0
    threadPrintYield(0);
  }
}

void m_createThreadsYield(int t) {
  // Change this number to change the number of threads getting created
  // 3 will end up creating 5 threads in total (since 0 creates 1, so 1 will 
  // only need to create 3 more threads)
  static int numThreads = 3;
  int t2 = -1;

  if (numThreads != 0) {
    t2 = create_thread(m_createThreadsYield, get_thread());
    numThreads--;
  }

  if (t2 != -1) {
    m_threadPrintYield(t2);
  }
  else {
    // Last thread created yields to thread 0
    m_threadPrintYield(0);
  }
}

void createThreadsSched(int t) {
  static int numThreads = 3;

  if (numThreads != 0) {
    create_thread(createThreadsSched, get_thread());
    numThreads--;
  }

  threadPrintSched(t);
}

void m_createThreadsSched(int t) {
  static int numThreads = 3;

  if (numThreads != 0) {
    create_thread(m_createThreadsSched, get_thread());
    numThreads--;
  }

  m_threadPrintSched(t);
}

void threadPrintYield(int t) {
  int i, yielder;
  for (i = 0; i < NUMYIELDS; i++) {
    Printf("T%d: iteration %d\n", get_thread(), i);
    yielder = yield_thread(t);
    Printf("Thread %d resumed by thread %d\n", get_thread(), yielder);
  }
}

// Thread uses some memory
// If you set STACKSIZE to 8192, you should fail this case, since a thread
// can run into another thread above
void m_threadPrintYield(int t) {
  int i, yielder;
  char mem[8192];
  memset(&mem, 0, sizeof mem);

  for (i = 0; i < NUMYIELDS; i++) {
    Printf("T%d: iteration %d\n", get_thread(), i);
    yielder = yield_thread(t);
    Printf("Thread %d resumed by thread %d\n", get_thread(), yielder);

    if (!checkZeros(mem, 8192))
      Printf("Stack memory has been tampered with by another thread\n");
  }
}

void threadPrintNoYield(int t) {
  int i;
  for (i = 0; i < NUMYIELDS; i++) {
    Printf("T%d: iteration %d\n", get_thread(), i);
  }
}

void threadPrintSched(int t) {
  int i;
  for (i = 0; i < NUMYIELDS; i++) {
    Printf("T%d: iteration %d\n", get_thread(), i);
    sched_thread();
  }
}

void m_threadPrintSched(int t) {
  int i;
  char mem[8192];
  memset(&mem, 0, sizeof mem);

  for (i = 0; i < NUMYIELDS; i++) {
    Printf("T%d: iteration %d\n", get_thread(), i);
    sched_thread();
    
    if (!checkZeros(mem, 8192))
      Printf("Stack memory has been tampered with by another thread\n");
  }
}

void threadPrintOnce(int t) {
  Printf("T%d prints\n", get_thread());
  sched_thread();
  exit_thread();
}

void threadExit(int t) {
  Printf("Thread %d is exiting\n", get_thread());
  exit_thread();
}

int checkZeros(char arr[], int size) {
  int i;
  for (i = 0; i < size; i++) {
    if (arr[i] != 0) {
      return 0;
    }
  }
  return 1;
}


#define NUMTESTS 19
void (*tests[])() = {test1, test2, test3, test4, test5, test6, test7, test8, 
test9, test10, test11, test12, test13, test14, test15, test16, test17, test18,
test19};

void usage(char* argv[]) {
  Printf(
  "Usage: %s -t <test number> [-e]\n"\
    " -e, --expected      Run the test using the reference thread package.\n"\
    " -t, --test          Test case to run.\n\n",
  argv[0]);
}

void Main(int argc, char* argv[]) {
  int flag;
  int run_expected = 0;
  const char* test_num = NULL;

  while ((flag = getopt_long(argc, argv, "et:", opts, NULL)) != -1) {
    switch (flag) {
    case 'e':
      run_expected = 1;
      break;
    case 't':
      test_num = optarg;
      break;
    default:
      usage(argv);
      Exit();
    }
  }

  // Provided extra args
  if (optind < argc) {
    Printf("Provided extra arguments\n");
    usage(argv);
    Exit();
  }

  if (test_num == NULL) {
    Printf("Test argument is not provided\n");
    usage(argv);
    Exit();
  }

  char* endptr;
  int index = strtol(test_num, &endptr, 0);
  if (*endptr != NULL) {
    Printf("Test Argument must be a number.\n");
    usage(argv);
    Exit();
  }

  if (index < 1 || index > NUMTESTS) {
    Printf("Test case number must be between 1 and %d\n", NUMTESTS);
    Exit();
  }

  // Running expected
  if (run_expected) {
    init_threads = InitThreads;
    create_thread = CreateThread;
    yield_thread = YieldThread;
    get_thread = GetThread;
    sched_thread = SchedThread;
    exit_thread = ExitThread;
  }

  init_threads();

  Printf("Running test %d\n\n", index);
  tests[index-1]();
  Exit();
}
