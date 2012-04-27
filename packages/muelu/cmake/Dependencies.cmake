SET(LIB_REQUIRED_DEP_PACKAGES Teuchos)
SET(LIB_OPTIONAL_DEP_PACKAGES Amesos Amesos2 Belos Epetra EpetraExt Kokkos
                              Ifpack Ifpack2 ML Tpetra Zoltan Stratimikos
                              Stokhos)
SET(TEST_REQUIRED_DEP_PACKAGES)
SET(TEST_OPTIONAL_DEP_PACKAGES Epetra EpetraExt Tpetra) # TODO: clean up this line
SET(LIB_REQUIRED_DEP_TPLS BLAS LAPACK)
SET(LIB_OPTIONAL_DEP_TPLS)
SET(TEST_REQUIRED_DEP_TPLS)
SET(TEST_OPTIONAL_DEP_TPLS)

#TODO: why there is no EXAMPLE_*_DEP_* ?
