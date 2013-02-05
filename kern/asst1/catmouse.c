/*
 * catmouse.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 * 26-11-2007: KMS : Modified to use cat_eat and mouse_eat
 * 21-04-2009: KMS : modified to use cat_sleep and mouse_sleep
 * 21-04-2009: KMS : added sem_destroy of CatMouseWait
 * 05-01-2012: TBB : added comments to try to clarify use/non use of volatile
 *
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>
#include <queue.h>

/*
 * 
 * cat,mouse,bowl simulation functions defined in bowls.c
 *
 * For Assignment 1, you should use these functions to
 *  make your cats and mice eat from the bowls.
 * 
 * You may *not* modify these functions in any way.
 * They are implemented in a separate file (bowls.c) to remind
 * you that you should not change them.
 *
 * For information about the behaviour and return values
 *  of these functions, see bowls.c
 *
 */

/* this must be called before any calls to cat_eat or mouse_eat */
extern int initialize_bowls(unsigned int bowlcount);

extern void cleanup_bowls( void );
extern void cat_eat(unsigned int bowlnumber);
extern void mouse_eat(unsigned int bowlnumber);
extern void cat_sleep(void);
extern void mouse_sleep(void);

/*
 *
 * Problem parameters
 *
 * Values for these parameters are set by the main driver
 *  function, catmouse(), based on the problem parameters
 *  that are passed in from the kernel menu command or
 *  kernel command line.
 *
 * Once they have been set, you probably shouldn't be
 *  changing them.
 *
 * These are only ever modified by one thread, at creation time,
 * so they do not need to be volatile.
 */
int NumBowls;  // number of food bowls
int NumCats;   // number of cats
int NumMice;   // number of mice
int NumLoops;  // number of times each cat and mouse should eat

/*
 * Keep a list of empty bowls
 */
static volatile struct queue *FreeBowlsList;

/* how many cats are currently eating?
 */
static int volatile eating_cats_count;

/* how many mice are currently eating?
 */
static int volatile eating_mice_count;

/* how many cats are waiting to eat?
 */
static int volatile waiting_cats_count;

/* how many mice are waiting to eat?
 */
static int volatile waiting_mice_count;

/*
 * Once the main driver function (catmouse()) has created the cat and mouse
 * simulation threads, it uses this semaphore to block until all of the
 * cat and mouse simulations are finished.
 */
struct semaphore *CatMouseWait;

struct semaphore *MaxBowlCount;

/* lock used to synchronize empty bowls
 *
 */
static struct lock *FreeBlockBowls;

static struct lock *WaitConditions;

static struct cv *cvNoCatsEating;
static struct cv *cvNoMiceEating;

/*
 * 
 * Function Definitions
 * 
 */


/*
 * cat_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds cat identifier from 0 to NumCats-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Each cat simulation thread runs this function.
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 */

static
void
cat_simulation(void * unusedpointer, 
               unsigned long catnumber)
{
  int i;
  unsigned int bowl;

  /* avoid unused variable warnings. */
  (void) unusedpointer;
  (void) catnumber;


  /* your simulated cat must iterate NumLoops times,
   *  sleeping (by calling cat_sleep() and eating
   *  (by calling cat_eat()) on each iteration */
  for(i=0;i<NumLoops;i++) {

    /* do not synchronize calls to cat_sleep().
       Any number of cats (and mice) should be able
       sleep at the same time. */
    cat_sleep();

    /* for now, this cat chooses a random bowl from
     * which to eat, and it is not synchronized with
     * other cats and mice.
     *
     * you will probably want to control which bowl this
     * cat eats from, and you will need to provide 
     * synchronization so that the cat does not violate
     * the rules when it eats */
    
    /* Critical Section: controlling when the cat gets to eat */
    lock_acquire(WaitConditions);
    
        waiting_cats_count++;

        // mice are currently eating
        if (eating_mice_count != 0)
        {
            while (eating_mice_count != 0)
               cv_wait(cvNoMiceEating, WaitConditions);
        }
        // cats are eating right now and mice are waiting
        else if (eating_cats_count != 0 && waiting_mice_count != 0)
        {
            while (eating_cats_count != 0 && waiting_mice_count != 0)
                cv_wait(cvNoMiceEating, WaitConditions);
        }

        eating_cats_count++;
    
    lock_release(WaitConditions);
    
    
    /* Critical Section: allowing only one animal to eat at one bowl at a time */
    P(MaxBowlCount);
        
    lock_acquire(FreeBlockBowls);
        if (!q_empty(FreeBowlsList)) 
        {
            bowl = q_remhead(FreeBowlsList);
        }
    lock_release(FreeBlockBowls);
    
    assert(bowl >= 1);
    assert(bowl <= NumBowls);
    
    // allowing cat to eat
    cat_eat(bowl);
        
    int *bowl_ptr = bowl;

    /* Critical Section: freeing the bowl */
    lock_acquire(FreeBlockBowls);
        // cat finished eating, we can add bowl number back to queue
        q_addtail(FreeBowlsList, bowl_ptr);
    lock_release(FreeBlockBowls);
    
    /* Critical section: update variables to indicate cat has already eaten */
    lock_acquire(WaitConditions);
    
    // decrease the count of cats currently eating
    eating_cats_count--;
    waiting_cats_count--;
     
    if (eating_cats_count == 0)
        cv_broadcast(cvNoCatsEating, WaitConditions);

    lock_release(WaitConditions);
    
    V(MaxBowlCount);
  }

  /* indicate that this cat simulation is finished */
  V(CatMouseWait); 
}

/*
 * mouse_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds mouse identifier from 0 to NumMice-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      each mouse simulation thread runs this function
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 *
 */

static
void
mouse_simulation(void * unusedpointer,
          unsigned long mousenumber)
{
  int i;
  unsigned int bowl;

  /* Avoid unused variable warnings. */
  (void) unusedpointer;
  (void) mousenumber;


  /* your simulated mouse must iterate NumLoops times,
   *  sleeping (by calling mouse_sleep()) and eating
   *  (by calling mouse_eat()) on each iteration */
  for(i=0;i<NumLoops;i++) {

    /* do not synchronize calls to mouse_sleep().
       Any number of mice (and cats) should be able
       sleep at the same time. */
    mouse_sleep();

    /* for now, this mouse chooses a random bowl from
     * which to eat, and it is not synchronized with
     * other cats and mice.
     *
     * you will probably want to control which bowl this
     * mouse eats from, and you will need to provide 
     * synchronization so that the mouse does not violate
     * the rules when it eats */

    lock_acquire(WaitConditions);
    
    waiting_mice_count++;
    
    // cats are eating
    if (eating_cats_count != 0)
    {
        // wait until cats finish eating
        while (eating_cats_count != 0)
           cv_wait(cvNoCatsEating, WaitConditions);
    }
    // mice are eating right now and there are cats waiting
    else if (eating_mice_count != 0 && waiting_cats_count != 0)
    {
        // wait until next batch of cats are done eating
        while (eating_mice_count != 0 && waiting_cats_count != 0)
            cv_wait(cvNoCatsEating, WaitConditions);
    }
    
    eating_mice_count++;
    
    lock_release(WaitConditions);
        
    P(MaxBowlCount);
    
    lock_acquire(FreeBlockBowls);
    
    if (!q_empty(FreeBowlsList)) 
    {
        bowl = q_remhead(FreeBowlsList);
    }
    
    lock_release(FreeBlockBowls);
    
    assert(bowl >= 1);
    assert(bowl <= NumBowls);
    
    /* allowing mouse to eat */
    mouse_eat(bowl);

    /* mouse finished eating, it gets back to its hole */
    int *bowl_ptr = bowl;
    
    /* Critical section: freeing the bowl */
    lock_acquire(FreeBlockBowls);
        q_addtail(FreeBowlsList, bowl_ptr);
    lock_release(FreeBlockBowls);
    
    /* Critical section: update variables to indicate mouse has already eaten */
    lock_acquire(WaitConditions);

        eating_mice_count--; // decrease the count of mice currently eating
        waiting_mice_count--;

        if (eating_mice_count == 0)
            cv_broadcast(cvNoMiceEating, WaitConditions); 

    lock_release(WaitConditions);
    
    V(MaxBowlCount);

  }

  /* indicate that this mouse is finished */
  V(CatMouseWait); 
}


/*
 * catmouse()
 *
 * Arguments:
 *      int nargs: should be 5
 *      char ** args: args[1] = number of food bowls
 *                    args[2] = number of cats
 *                    args[3] = number of mice
 *                    args[4] = number of loops
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up cat_simulation() and
 *      mouse_simulation() threads.
 *      You may need to modify this function, e.g., to
 *      initialize synchronization primitives used
 *      by the cat and mouse threads.
 *      
 *      However, you should should ensure that this function
 *      continues to create the appropriate numbers of
 *      cat and mouse threads, to initialize the simulation,
 *      and to wait for all cats and mice to finish.
 */

int
catmouse(int nargs,
         char ** args)
{
  int index, error;
  int i;
  int iBowl;

  /* check and process command line arguments */
  if (nargs != 5) {
    kprintf("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS\n");
    return 1;  // return failure indication
  }

  /* check the problem parameters, and set the global variables */
  NumBowls = atoi(args[1]);
  if (NumBowls <= 0) {
    kprintf("catmouse: invalid number of bowls: %d\n",NumBowls);
    return 1;
  }
  NumCats = atoi(args[2]);
  if (NumCats < 0) {
    kprintf("catmouse: invalid number of cats: %d\n",NumCats);
    return 1;
  }
  NumMice = atoi(args[3]);
  if (NumMice < 0) {
    kprintf("catmouse: invalid number of mice: %d\n",NumMice);
    return 1;
  }
  NumLoops = atoi(args[4]);
  if (NumLoops <= 0) {
    kprintf("catmouse: invalid number of loops: %d\n",NumLoops);
    return 1;
  }
  kprintf("Using %d bowls, %d cats, and %d mice. Looping %d times.\n",
          NumBowls,NumCats,NumMice,NumLoops);

  /* create the semaphore that is used to make the main thread
     wait for all of the cats and mice to finish */
  CatMouseWait = sem_create("CatMouseWait",0);
  if (CatMouseWait == NULL) {
    panic("catmouse: could not create semaphore\n");
  }

  /* create the semaphore used to restrict eating animals to the 
     number of bowls */
  MaxBowlCount = sem_create("BowlCount",NumBowls);
  if (MaxBowlCount == NULL) {
    panic("MaxBowlCount: could not create semaphore\n");
  }
  
  /* create condition variables */
  cvNoCatsEating = cv_create("catsEating");
  cvNoMiceEating = cv_create("miceEating");
  
  /* create the lock that is used to manage empty bowls 
     and preventing more than one animal to eat from the same bowl */
  FreeBlockBowls = lock_create("freeBowls");
  if (FreeBlockBowls == NULL) {
    panic("synchtest: FreeBlockBowls lock_create failed\n");
  }
    
  WaitConditions = lock_create("EatingAnimalCount");
  if (WaitConditions == NULL) {
      panic("synchtest: Eating Animal Count lock_create failed\n");
  }
  /* 
   * initialize the bowls
   */
  if (initialize_bowls(NumBowls)) {
    panic("catmouse: error initializing bowls.\n");
  }

  /* Initialize list of free bowls */
  FreeBowlsList = q_create(NumBowls);
  for (iBowl = 1; iBowl <= NumBowls; iBowl++)
  {
      int *bowl_ptr = iBowl;
      q_addtail(FreeBowlsList, bowl_ptr);
  }
  
  /* Initialize variables */
  eating_cats_count = 0;
  eating_mice_count = 0;
  
  waiting_cats_count = 0;
  waiting_mice_count = 0;
  
  /*
   * Start NumCats cat_simulation() threads.
   */
  for (index = 0; index < NumCats; index++) {
    error = thread_fork("cat_simulation thread",NULL,index,cat_simulation,NULL);
    if (error) {
      panic("cat_simulation: thread_fork failed: %s\n", strerror(error));
    }
  }

  /*
   * Start NumMice mouse_simulation() threads.
   */
  for (index = 0; index < NumMice; index++) {
    error = thread_fork("mouse_simulation thread",NULL,index,mouse_simulation,NULL);
    if (error) {
      panic("mouse_simulation: thread_fork failed: %s\n",strerror(error));
    }
  }

  /* wait for all of the cats and mice to finish before
     terminating */  
  for(i=0;i<(NumCats+NumMice);i++) {
    P(CatMouseWait);
  }

  /* clean up the semaphore that we created */
  sem_destroy(CatMouseWait);
  
  /* test semaphore */
  sem_destroy(MaxBowlCount);
  
  /* clean up cvs*/
  cv_destroy(cvNoCatsEating);
  cv_destroy(cvNoMiceEating);
  
  /* clean up locks */
  lock_destroy(FreeBlockBowls);
  lock_destroy(WaitConditions);
  
  /* clean up resources used for tracking bowl use */
  cleanup_bowls();

  /* clean up queue of available bowls */
  while (!q_empty(FreeBowlsList))
      q_remhead(FreeBowlsList);
  
  q_destroy(FreeBowlsList);
  
  return 0;
}

/*
 * End of catmouse.c
 */
