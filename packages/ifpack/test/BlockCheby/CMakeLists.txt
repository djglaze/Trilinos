

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  BlockCheby
  SOURCES cxx_main.cpp
  ARGS "-v"
  NUM_MPI_PROCS 1-4
  COMM serial mpi
  )

