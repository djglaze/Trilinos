// @HEADER
// ***********************************************************************
// 
//                    Teuchos: Common Tools Package
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#include "Teuchos_ConfigDefs.hpp"
#include "Teuchos_TimeMonitor.hpp"
#include "Teuchos_ScalarTraits.hpp"
#include "Teuchos_Version.hpp"

#ifdef HAVE_MPI
#include <mpi.h>
#endif

using namespace Teuchos;
using std::string;

/* Test of Teuchos timing classes */


/* create timers for several functions */
static Time& sqrtTimer() {static RefCountPtr<Time> t = TimeMonitor::getNewTimer("square roots"); return *t;}

static Time& factTimer() {static RefCountPtr<Time> t = TimeMonitor::getNewTimer("factorials"); return *t;}

static Time& exceptTimer() {static RefCountPtr<Time> t = TimeMonitor::getNewTimer("func with exception"); return *t;}

static Time& localTimer() {static RefCountPtr<Time> t = TimeMonitor::getNewTimer("a function that is not called on all procs"); return *t;}

static Time& anotherTimer() {static RefCountPtr<Time> t = TimeMonitor::getNewTimer("another func"); return *t;}

static Time& yetAnotherTimer() {static RefCountPtr<Time> t = TimeMonitor::getNewTimer("yet another func"); return *t;}

static Time& yetOneMoreTimer() {static RefCountPtr<Time> t = TimeMonitor::getNewTimer("yet one more func"); return *t;}


int main(int argc, char* argv[])
{
  bool verbose = 0;
  int procRank = 0;
  int FailedTests = 1; // This will be set to 0, if the exception is caught!

#ifdef HAVE_MPI 
  /* initialize MPI if we are running in parallel */
  MPI_Init(&argc, &argv);
  MPI_Comm_rank( MPI_COMM_WORLD, &procRank );
#endif      
  
  // Check for verbose flag.
  if (argc>1) if (argv[1][0]=='-' && argv[1][1]=='v') verbose = true;

  if (verbose && procRank==0)
    cout << Teuchos::Teuchos_Version() << endl << endl;

  try
    {
      double sqrtFunc();
      double factFunc(int x);
      double exceptFunc();
      double localFunc();
      double anotherFunc();
      double yetAnotherFunc();
      double yetOneMoreFunc();

      
      /* time a simple function */
      for (int i=0; i<100; i++)
        {
          double x = 0.0;
          x = sqrtFunc();
        }

      /* time a reentrant function */
      for (int i=0; i<100; i++)
        {
          factFunc(100);
        }

      /* time a couple of silly functions */
      for (int i=0; i<100; i++)
        {
          anotherFunc();
          yetAnotherFunc();
          yetOneMoreFunc();
        }

      /* Time a function that will be called only on the root proc. This 
       * checks that the TimeMonitor will work properly when different
       * processors have different sets of timers. */
      if (procRank==0)
        {
          for (int i=0; i<100; i++)
            {
              double x = 0.0;
              x = localFunc();
            }
        }

      /* time a function that throws an exception */
       for (int i=0; i<100; i++)
         {
           double x = 0.0;
           x = exceptFunc();
         }

      
    }
  catch(std::exception& e)
    {
      if (verbose && procRank==0)

	cerr << "Caught exception [expected]:  " << e.what() << endl;

      // Return 0 since we caught the exception
      FailedTests = 0;
    }

  /* Summarize timings. This must be done before finalizing MPI  */
  TimeMonitor::format().setRowsBetweenLines(3);
  if (verbose)
    TimeMonitor::summarize();

#ifdef HAVE_MPI
  /* clean up MPI if we are running in parallel*/
  MPI_Finalize();
#endif

  if (FailedTests != 0) {
    cout << "End Result: TEST FAILED" << endl;
    return -1;
  }

  cout << "End Result: TEST PASSED" << endl;
  return FailedTests;
}

/* sum sqrt(x), x=[0, 10000). */
double sqrtFunc()
{
  /* construct a time monitor. This starts the timer. It will stop when leaving scope */
  TimeMonitor timer(sqrtTimer());

  double sum = 0.0;

  for (int i=0; i<10000; i++) 
    {
      TEST_FOR_EXCEPTION(ScalarTraits<double>::squareroot((double) i) > 1000.0, std::runtime_error,
                         "throw an exception");
      sum += ScalarTraits<double>::squareroot((double) i);
    }

  return sum;
}


/* compute log(factorial(x)) */
double factFunc(int x)
{
  /* construct a time monitor. This starts the timer. It will stop when leaving scope */
  TimeMonitor timer(factTimer());

  if (x==0) return 0;
  if (x==1) return 1;
  return log((double) x)  + factFunc(x-1);
}



/* sum sqrt(x), x=[0, 10000). */
double exceptFunc()
{
  /* construct a time monitor. This starts the timer. It will stop when leaving scope */
  TimeMonitor timer(exceptTimer());

  double sum = 0.0;
  for (int i=0; i<10000; i++)
    {
      TEST_FOR_EXCEPTION(ScalarTraits<double>::squareroot((double) i) > 60.0, std::runtime_error,
                         "throw an exception");
      sum += ScalarTraits<double>::squareroot((double) i);
    }
  return sum;
}



/* sum x, x=[0, 10000). */
double localFunc()
{
  /* construct a time monitor. This starts the timer. It will stop when leaving scope */
  TimeMonitor timer(localTimer());

  double sum = 0.0;

  for (int i=0; i<10000; i++) 
    {
      sum += i;
    }

  return sum;
}

/* sum x^2, x=[0, 10000). */
double anotherFunc()
{
  /* construct a time monitor. This starts the timer. It will stop when leaving scope */
  TimeMonitor timer(anotherTimer());

  double sum = 0.0;

  for (int i=0; i<10000; i++) 
    {
      sum += i*i;
    }

  return sum;
}

/* sum x^3, x=[0, 10000). */
double yetAnotherFunc()
{
  /* construct a time monitor. This starts the timer. It will stop when leaving scope */
  TimeMonitor timer(yetAnotherTimer());

  double sum = 0.0;

  for (int i=0; i<10000; i++) 
    {
      sum += i*i*i;
    }

  return sum;
}

/* sum x+1, x=[0, 10000). */
double yetOneMoreFunc()
{
  /* construct a time monitor. This starts the timer. It will stop when leaving scope */
  TimeMonitor timer(yetOneMoreTimer());

  double sum = 0.0;

  for (int i=0; i<10000; i++) 
    {
      sum += i+1;
    }

  return sum;
}





