// @HEADER
// ***********************************************************************
//
//                Copyright message goes here.
//
// ***********************************************************************
// @HEADER

/*! \file Zoltan2_IdentifierMap.hpp
    \brief Defines the IdentifierMap class.
*/

#ifndef _ZOLTAN2_IDENTIFIERMAP_HPP_
#define _ZOLTAN2_IDENTIFIERMAP_HPP_

#include <Zoltan2_IdentifierTraits.hpp>
#include <Zoltan2_InputTraits.hpp>
#include <Zoltan2_AlltoAll.hpp>

#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

#include <Teuchos_as.hpp>
#include <Teuchos_Hashtable.hpp>

namespace Zoltan2
{
/*! \brief Identifier translations available from IdentifierMap.
*/

enum TranslationType {
  TRANSLATE_APP_TO_LIB,  /*!< \brief convert user ids to internal ids */
  TRANSLATE_LIB_TO_APP   /*!< \brief convert internal ids to user ids */
};

/*!  \brief An IdentifierMap manages a global space of object identifiers.

    Data types:
    \li \c lno_t    local indices and local counts
    \li \c gno_t    global indices and global counts
    \li \c gid_t    application global Ids

    The template parameter \c User is a user-defined data type
    which, through a traits mechanism, provides the actual data types
    with which the Zoltan2 library will be compiled.
    \c User may be the actual class or structure used by application to
    represent a vector, or it may be the helper class BasicUserTypes.
    See InputTraits for more information.

      \todo test for global IDs that are std::pair<T1, T2>

  \todo we require that user's gid_ts have base zero if we are to
     use them as consecutive gno_ts.  This can be fixed if needed.
     We are not getting the efficiency
     advantage of using the user's gids simply because
     those IDs do not have base 0.  We can add a flag
     about base 0 being required, and then map only
     if base 0 is required.
*/

////////////////////////////////////////////////////////////////////
// Declarations
////////////////////////////////////////////////////////////////////

template<typename User>
    class IdentifierMap{

public:

  /*! \brief Constructor - Must be called by all processes 
   *
   * \param env  the problem and library environment
   * \param comm the problem communicator
   * \param gids  the application global IDs
   * \param gidsMustBeConsecutive  set to true if the algorithm
   *           or third party library requires consective ids
   *           If necessary the IdentifierMap will map the application's
   *           global IDs to consecutive integral IDs beginning at zero.
   */

  typedef typename InputTraits<User>::lno_t lno_t;
  typedef typename InputTraits<User>::gno_t gno_t;
  typedef typename InputTraits<User>::gid_t gid_t;

  explicit IdentifierMap( const RCP<const Environment > &env, 
                          const RCP<const Comm<int> > &comm,
                          const ArrayRCP<const gid_t> &gids, 
                          bool gidsMustBeConsecutive=false);

  /*! \brief Destructor */
  ~IdentifierMap() {};

  /*! \brief Copy Constructor */
  IdentifierMap(const IdentifierMap &id);

  /*! \brief Assignment operator */
  IdentifierMap &operator=(const IdentifierMap &id);

  /*! \brief Return true if we are using the application global IDs 
   *  for our internal global numbers 
   */
  bool gnosAreGids() const;

  /*! \brief Return the global number of identifiers.
   */
  gno_t getGlobalNumberOfIds() const { return globalNumberOfIds_;}

  /*! \brief Return the local number of identifiers.
   */
  gno_t getLocalNumberOfIds() const { return localNumberOfIds_;}

  /*! \brief Return the minimum and maximum values of the internal
   *  global numbers 
   */
  void getGnoRange(gno_t &min, gno_t &max) const;

  /*! \brief Return true if our internal global numbers are consecutive.
   */
  bool gnosAreConsecutive() const;

  /*! \brief Return true if consecutive Gids are required.
   */
  bool consecutiveGnosAreRequired() const;

  /*! \brief Return the minimum Zoltan2 global Id across all processes.
   */
  gno_t getMinimumGlobalId() const;

  /*! \brief Return the maximum Zoltan2 global Id across all processes.
   */
  gno_t getMaximumGlobalId() const;

  /*! \brief Map the application global IDs to internal global numbers or vice versa.

      \param gid an array of caller's global IDs
      \param gno an array of Zoltan2 global numbers
      \param tt should be TRANSLATE_APP_TO_LIB or TRANSLATE_LIB_TO_APP

      Both gid and gno are preallocated by the caller.

      This is a local call.  If gid is a vector of application global IDs, then
      gno will be set to the corresponding internal global numbers.  If the
      gno vector contains internal global numbers, they will be translated
      to application global IDs.  The application global IDs must be from
      those supplied by this process.

      \todo If the gnos need to be translated to gids, check first to
      see if the gnos are in their original order.  If so, we can 
      save memory by returning an arrayview of the gids.
   */
  void gidTranslate(ArrayView<gid_t> gid, 
                    ArrayView<gno_t> gno,
                    TranslationType tt) const;

  /*! \brief Map application indices to internal global numbers or vice versa.

      \param lno an array of indices
      \param gno an array of Zoltan2 global numbers
      \param tt should be TRANSLATE_APP_TO_LIB or TRANSLATE_LIB_TO_APP

      This is a local call. 

      If gno contains a list of Zoltan2 internal
      global numbers, then lno[i] on return will be the index at which 
      the application's global ID associated with gno[i] appeared in the 
      input adapter's "get" method list. In this case, lno should be
      pre-allocated by the caller, and tt should be TRANSLATE_LIB_TO_APP.

      Similarly, if lno contains a list of indices ranging from 0 to N-1
      (where N would be the local number of objects), then gno[i] on
      return will be the internal global number associated with the
      application's global ID that appeared in location lno[i] in the 
      input adapter's "get" method list. In this case, gno should be
      pre-allocated by the caller and tt should be TRANSLATE_APP_TO_LIB.
   */
  void lnoTranslate(ArrayView<lno_t> lno, 
                    ArrayView<gno_t> gno,
                    TranslationType tt) const;

  /*! \brief Map application global IDs to internal global numbers.

      \param in_gid input, an array of the global IDs
      \param out_gno output, an optional array of the corresponding 
          global numbers used by Zoltan2.  If out_gno.size() is zero,
          we assume global numbers are not needed.
      \param out_proc output, an array of the process ranks corresponding with
                         the in_gid and out_gno, out_proc[i] is the process
                         that supplied in_gid[i] to Zoltan2.  

      If in_gid[i] is not one of the global Ids supplied, then
      out_proc[i] will be -1, and out_gno[i] (if requested) is undefined.
      This behavior supports graph subsetting.

      All processes must call this.  The global IDs 
      supplied may belong to another process.  
   */
  void gidGlobalTranslate( ArrayView<const gid_t> in_gid,
                           ArrayView<gno_t> out_gno,
                           ArrayView<int> out_proc) const;

  /*! \brief Map library internal global numbers to application GIDs,
                        and their process owners

      \param in_gno input, an array of the global numbers.
      \param out_gid output, an optional array of the corresponding 
          user global Ids.  If out_gid.size() is zero,
          we assume global Ids are not desired.
      \param out_proc output, an array of the process ranks corresponding with
                         the in_gno and out_gid, out_proc[i] is the process
                         that supplied out_gid[i] to Zoltan2.  

      All processes must call this.  The global numbers
      supplied may belong to another process.  
   */
  void gnoGlobalTranslate( ArrayView<const gno_t> in_gno,
                           ArrayView<gid_t> out_gid,
                           ArrayView<int> out_proc) const;
private:

  // Problem parameters, library configuration.

  const RCP<const Environment> env_;

  // Problem communicator

  const RCP<const Comm<int> > comm_;

  // Application global IDs

  const ArrayRCP<const gid_t> myGids_; 

  // Zoltan2 gno_ts will be consecutive if the application gid_ts
  // were mapped to gno_ts, or if the application gid_ts happen
  // to be consecutive ordinals.  In either case, gnoDist_[p]
  // is the first gno_t on process p.  gnoDist_[numProcs_] is the
  // global total of gno_ts.

  ArrayRCP<gno_t> gnoDist_;

  // If application gid_ts are not consecutive ordinals, the gidHash_
  // table maps a key (a double generated from the gid_t) to the
  // location of the gid_t in the myGids_ list.

  RCP<Teuchos::Hashtable<double, lno_t> >  gidHash_;

  global_size_t globalNumberOfIds_;
  size_t localNumberOfIds_;
  int myRank_;
  int numProcs_;

  // By "Consecutive" we mean globally consecutive increasing
  // with process rank.

  bool userGidsAreTeuchosOrdinal_;
  bool userGidsAreConsecutive_;
  bool userGidsAreZoltan2Gnos_;
  bool zoltan2GnosAreConsecutive_;
  
  bool consecutiveGidsAreRequired_;

  gno_t minGlobalGno_;
  gno_t maxGlobalGno_;

  void setupMap();
};

////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////

template<typename User>
  IdentifierMap<User>::IdentifierMap( const RCP<const Environment> &env,
    const RCP<const Comm<int> > &comm,
    const ArrayRCP<const gid_t> &gids, bool idsMustBeConsecutive) 
         : env_(env), comm_(comm), myGids_(gids), gnoDist_(), gidHash_(),
           globalNumberOfIds_(0), localNumberOfIds_(0),
           myRank_(0), numProcs_(1),
           userGidsAreTeuchosOrdinal_(false), userGidsAreConsecutive_(false), 
           userGidsAreZoltan2Gnos_(false), zoltan2GnosAreConsecutive_(false), 
           consecutiveGidsAreRequired_(idsMustBeConsecutive),
           minGlobalGno_(0), maxGlobalGno_(0)
{
  myRank_ = comm_->getRank();
  numProcs_ = comm_->getSize();
  setupMap();
}

// TODO many of these should be inline

template< typename User>
  bool IdentifierMap<User>::gnosAreGids() const
{
  return userGidsAreZoltan2Gnos_;
}

template <typename User>
  void IdentifierMap<User>::getGnoRange(gno_t &min, gno_t &max) const
{
  min = minGlobalGno_;
  max = maxGlobalGno_;
}

template< typename User>
  bool IdentifierMap<User>::gnosAreConsecutive() const
{
  return zoltan2GnosAreConsecutive_;
}

template< typename User>
  bool IdentifierMap<User>::consecutiveGnosAreRequired() const
{
  return consecutiveGidsAreRequired_;
}

template< typename User>
  typename InputTraits<User>::gno_t IdentifierMap<User>::getMinimumGlobalId() const
{
  return minGlobalGno_;
}

template< typename User>
  typename InputTraits<User>::gno_t IdentifierMap<User>::getMaximumGlobalId() const
{
  return maxGlobalGno_;
}

template< typename User>
  void IdentifierMap<User>::gidTranslate(
    ArrayView<gid_t> gid, 
    ArrayView<gno_t> gno,
    TranslationType tt) const
{
  size_t len=gid.size();

  if (len == 0){
    return;
  }

  env_->localInputAssertion(__FILE__, __LINE__, "invalid TranslationType", 
    (tt==TRANSLATE_APP_TO_LIB) || (tt==TRANSLATE_LIB_TO_APP), 
    BASIC_ASSERTION);

  env_->localInputAssertion(__FILE__, __LINE__, 
    "Destination array is too small",
    ((tt==TRANSLATE_LIB_TO_APP) && (gid.size() >= gno.size())) || 
     ((tt==TRANSLATE_APP_TO_LIB) && (gno.size() >= gid.size())),
    BASIC_ASSERTION);

  if (userGidsAreZoltan2Gnos_){   // our gnos are the app gids
    if (tt == TRANSLATE_LIB_TO_APP)
      for (size_t i=0; i < len; i++)
        gid[i] = Teuchos::as<gid_t>(gno[i]);
    else
      for (size_t i=0; i < len; i++)
        gno[i] = Teuchos::as<gno_t>(gid[i]);
  }
  else{              // we mapped gids to consecutive gnos
    gno_t firstGno = gnoDist_[myRank_];
    gno_t endGno = gnoDist_[myRank_ + 1];

    if (tt == TRANSLATE_LIB_TO_APP){
      for (size_t i=0; i < len; i++){

        env_->localInputAssertion(__FILE__, __LINE__, "invalid global number", 
        (gno[i] >= firstGno) && (gno[i] < endGno), BASIC_ASSERTION);

        gid[i] = myGids_[gno[i] - firstGno];
      }
    }
    else{
      lno_t idx=0;
      if (userGidsAreConsecutive_){
        for (size_t i=0; i < len; i++){
          gno[i] = firstGno + IdentifierTraits<gid_t>::difference(
            myGids_[0], gid[i]);
          env_->localInputAssertion(__FILE__, __LINE__, "invalid global id", 
            (gno[i] >= firstGno) && (gno[i] < endGno), BASIC_ASSERTION);
        }
      }
      else{
        for (size_t i=0; i < len; i++){
          try{
            double key = Zoltan2::IdentifierTraits<gid_t>::key(gid[i]);
            idx = gidHash_->get(key);
          }
          catch (const std::exception &e) {
            env_->localInputAssertion(__FILE__, __LINE__, "invalid global id", 
              false, BASIC_ASSERTION);
          }
          
          gno[i] = firstGno + idx;
        }
      }
    }
  }
  return;
}

template< typename User>
  void IdentifierMap<User>::lnoTranslate(
    ArrayView<lno_t> lno, 
    ArrayView<gno_t> gno, 
    TranslationType tt) const
{
  size_t len=lno.size();

  if (len == 0){
    return;
  }
  env_->localInputAssertion(__FILE__, __LINE__, "invalid TranslationType", 
    (tt==TRANSLATE_LIB_TO_APP) || (tt==TRANSLATE_APP_TO_LIB), 
    BASIC_ASSERTION);

  env_->localInputAssertion(__FILE__, __LINE__, 
    "Destination array is too small",
    ((tt==TRANSLATE_LIB_TO_APP) && (lno.size() >= gno.size())) || 
    ((tt==TRANSLATE_APP_TO_LIB) && (gno.size() >= lno.size())),
    BASIC_ASSERTION);

  gno_t firstGno(0), endGno(0);
  if (gnoDist_.size() > 0){
    firstGno = gnoDist_[myRank_];
    endGno = gnoDist_[myRank_+1];
  }
  
  if (tt == TRANSLATE_LIB_TO_APP){
    if (gnoDist_.size() > 0) {   // gnos are consecutive
      for (size_t i=0; i < len; i++){
        env_->localInputAssertion(__FILE__, __LINE__, "invalid global number", 
          (gno[i] >= firstGno) && (gno[i] < endGno), BASIC_ASSERTION);
        lno[i] = gno[i] - firstGno;
      }
    }
    else {                    // gnos must be the app gids
      if (userGidsAreConsecutive_){
        for (size_t i=0; i < len; i++){ 
          gid_t tmp = Teuchos::as<gid_t>(gno[i]);
          lno[i] = IdentifierTraits<gid_t>::difference(myGids_[0], tmp);
          env_->localInputAssertion(__FILE__, __LINE__, "invalid global number",
            (lno[i] >= 0) && (lno[i] < localNumberOfIds_), BASIC_ASSERTION);
        }
      }
      else{
        for (size_t i=0; i < len; i++){ 
          try{
            gid_t keyArg = Teuchos::as<gid_t>(gno[i]);
            lno[i] = gidHash_->get(IdentifierTraits<gid_t>::key(keyArg));
          }
          catch (const std::exception &e) {
            env_->localInputAssertion(__FILE__, __LINE__, 
              "invalid global number", false, BASIC_ASSERTION);
          }
        }
      }
    }
  }
  else{                           // TRANSLATE_APP_TO_LIB
    for (size_t i=0; i < len; i++){
      lno_t idx = lno[i];

      if (gnoDist_.size() > 0)  // gnos are consecutive
        gno[i] = firstGno + idx;
      else                     // gnos must be the app gids
        gno[i] = Teuchos::as<gno_t>(myGids_[idx]);
    }
  }
}

template< typename User>
  void IdentifierMap<User>::gidGlobalTranslate(
    ArrayView<const gid_t> in_gid,
    ArrayView<gno_t> out_gno,
    ArrayView<int> out_proc) const
{
  typedef typename Teuchos::Hashtable<double, lno_t> id2index_hash_t;

  size_t len=in_gid.size();

  if (len == 0){
    return;
  }

  bool skipGno = (out_gno.size() == 0);

  env_->localInputAssertion(__FILE__, __LINE__, 
    "Destination array is too small", 
    (out_proc.size() >= len) && (skipGno || (out_gno.size() >= len)),
    BASIC_ASSERTION);

  if (userGidsAreZoltan2Gnos_ && (gnoDist_.size() > 0)){

    // Easy case - communication is not needed.
    // Global numbers are the application global IDs and
    // they are increasing consecutively with rank.

    gno_t *gnos = gnoDist_.getRawPtr();
    gno_t *final = gnos + numProcs_ + 1;

    for (size_t i=0; i < len; i++){
      gno_t gno = Teuchos::as<gno_t>(in_gid[i]);
      if (!skipGno)
        out_gno[i] = gno;

      gno_t *ub = std::upper_bound(gnos, final, gno);
      if (ub !=final)
        out_proc[i] = (ub - gnos - 1);
      else
        out_proc[i] = -1;   // globally not one of our gids
    }

    return;
  }

  bool needGnoInfo = !userGidsAreZoltan2Gnos_;

  ///////////////////////////////////////////////////////////////////////
  // First: Hash each of my gid_ts to a process that will answer
  // for it.  Send my gid_ts (and the Gnos if they are different)
  // to their assigned processes.  Build a search structure for
  // the gid_ts that were assigned to me, so I can reply with
  // with the process owning them (and their Gnos if they are different).
  ///////////////////////////////////////////////////////////////////////

  Array<int> hashProc;
  Array<gid_t> gidOutBuf;
  Array<gno_t> gnoOutBuf;
  Array<lno_t> countOutBuf(numProcs_, 0);
  Array<lno_t> offsetBuf(numProcs_ + 1, 0);

  ArrayRCP<gid_t> gidInBuf;
  ArrayRCP<gno_t> gnoInBuf;
  ArrayRCP<lno_t> countInBuf;

  if (localNumberOfIds_ > 0){

    try{ 
      hashProc.resize(localNumberOfIds_, 0);
    }
    catch(...){
      env_->localMemoryAssertion(__FILE__, __LINE__, localNumberOfIds_, false); 
    }

    try{ 
      gidOutBuf.resize(localNumberOfIds_); 
    }
    catch(...){
      env_->localMemoryAssertion(__FILE__, __LINE__, localNumberOfIds_, false); 
    }

    for (size_t i=0; i < localNumberOfIds_; i++){
      hashProc[i] = IdentifierTraits<gid_t>::hashCode(myGids_[i]) % numProcs_;
      countOutBuf[hashProc[i]]++;
    }
  
    for (int p=1; p <= numProcs_; p++){
      offsetBuf[p] = offsetBuf[p-1] + countOutBuf[p-1];
    }
  
    if (needGnoInfo){   
      // The gnos are not the gids, which also implies that
      // gnos are consecutive numbers given by gnoDist_.
      gnoOutBuf.resize(localNumberOfIds_, 0);
    }
  
    for (size_t i=0; i < localNumberOfIds_; i++){
      lno_t offset = offsetBuf[hashProc[i]];
      gidOutBuf[offset] = myGids_[i];
      if (needGnoInfo)
        gnoOutBuf[offset] = gnoDist_[myRank_] + i;
      offsetBuf[hashProc[i]] = offset + 1;
    }
    hashProc.clear();
  }

  // Z2::AlltoAllv comment: Buffers are in process rank contiguous order.

  try{
    ArrayView<const gid_t> gidView = gidOutBuf();
    ArrayView<const lno_t> countView = countOutBuf();
    AlltoAllv<gid_t, lno_t>(*comm_, *env_, gidView, countView, gidInBuf, countInBuf);
  }
  Z2_FORWARD_EXCEPTIONS;

  gidOutBuf.clear();

  if (needGnoInfo){
    countInBuf.release();
    ArrayView<const gno_t> gnoView = gnoOutBuf();
    ArrayView<const lno_t> countView = countOutBuf();
    try{
      AlltoAllv<gno_t, lno_t>(*comm_, *env_, gnoView, countView, gnoInBuf, countInBuf);
    }
    Z2_FORWARD_EXCEPTIONS;
  }

  gnoOutBuf.clear();
  countOutBuf.clear();

  //
  // Save the information that was hashed to me so I can do lookups.
  //

  std::vector<lno_t> firstIndex;
  std::vector<int> sendProc;
  lno_t indexTotal = 0;

  for (int p=0; p < numProcs_; p++){
    lno_t len = countInBuf[p];
    if (len > 0){
      firstIndex.push_back(indexTotal);
      sendProc.push_back(p);
      indexTotal += len;
    }
  }
  firstIndex.push_back(indexTotal);

  id2index_hash_t gidToIndex(indexTotal);

  lno_t total = 0;
  for (int p=0; p < numProcs_; p++){
    for (lno_t i=0; i < countInBuf[p]; i++, total++){
      try{
        double keyVal = IdentifierTraits<gid_t>::key(gidInBuf[total]);
        gidToIndex.put(keyVal, total);
      }
      Z2_THROW_OUTSIDE_ERROR(*env_);
    }
  }

  // Keep gnoInBuf.  We're done with the others.

  gidInBuf.release();
  countInBuf.release();

  ///////////////////////////////////////////////////////////////////////
  // Send a request for information to the "answer process" for each 
  // of the gid_ts in in_gid.  First remove duplicate gid_ts from the list.
  //
  // It is possible that some of the gids do not belong to any process.
  ///////////////////////////////////////////////////////////////////////
  
  Array<double> uniqueGidQueries;
  Array<Array<lno_t> > uniqueGidQueryIndices;
  size_t numberOfUniqueGids = 0;
  Array<lno_t> gidLocation;

  countOutBuf.resize(numProcs_, 0);

  std::map<double, std::vector<lno_t> > gidIndices;
  typename std::map<double, std::vector<lno_t> >::iterator next; 

  if (len > 0){
    for (lno_t i=0; i < len; i++){
      double uniqueKey(IdentifierTraits<gid_t>::key(in_gid[i]));
      next = gidIndices.find(uniqueKey);
      if (next == gidIndices.end()){
        std::vector<lno_t> v(1, i);
        gidIndices[uniqueKey] = v;
      }
      else{
        std::vector<lno_t> &v = next->second;
        v.push_back(i);
      }
    }
  
    numberOfUniqueGids = gidIndices.size();
  
    try{ 
      gidOutBuf.resize(numberOfUniqueGids); 
    }
    catch(...){
      env_->localMemoryAssertion(__FILE__, __LINE__, numberOfUniqueGids, 
        false); 
    }

    try{ 
      hashProc.resize(numberOfUniqueGids, 0);
    }
    catch(...){
      env_->localMemoryAssertion(__FILE__, __LINE__, numberOfUniqueGids, 
        false); 
    }
  
    lno_t idx = 0;
    for (next = gidIndices.begin(); next != gidIndices.end(); ++next, ++idx){
      double key = next->first;
      gid_t gid = IdentifierTraits<gid_t>::keyToGid(key);
      hashProc[idx] = IdentifierTraits<gid_t>::hashCode(gid) % numProcs_;
      countOutBuf[hashProc[idx]]++;
    }
  
    offsetBuf[0] = 0;
  
    for (int p=0; p < numProcs_; p++){
      offsetBuf[p+1] = offsetBuf[p] + countOutBuf[p];
    }
  
    try{ 
      gidLocation.resize(numberOfUniqueGids, 0);
    }
    catch(...){
      env_->localMemoryAssertion(__FILE__, __LINE__, numberOfUniqueGids, 
        false); 
    }
  
    idx = 0;
    for (next = gidIndices.begin(); next != gidIndices.end(); ++next, ++idx){
      double key = next->first;
      gid_t gid = IdentifierTraits<gid_t>::keyToGid(key);
      gidLocation[idx] = offsetBuf[hashProc[idx]];
      gidOutBuf[gidLocation[idx]] = gid;
      offsetBuf[hashProc[idx]] = gidLocation[idx] + 1;
    }

    hashProc.clear();
  }

  try{
    ArrayView<const gid_t> gidView = gidOutBuf();
    ArrayView<const lno_t> countView = countOutBuf();
    AlltoAllv<gid_t,lno_t>(*comm_, *env_, gidView, countView, gidInBuf, countInBuf);
  }
  Z2_FORWARD_EXCEPTIONS;

  gidOutBuf.clear();

  ///////////////////////////////////////////////////////////////////////
  // Create and send answers to the processes that made requests of me.
  ///////////////////////////////////////////////////////////////////////

  total = 0;

  for (int p=0; p < numProcs_; p++){
    countOutBuf[p] = countInBuf[p];
    total += countOutBuf[p];
  }

  Array<int> procOutBuf(total);
  ArrayRCP<int> procInBuf;

  if (needGnoInfo){
    try{ 
      gnoOutBuf.resize(total, 0);
    }
    catch(...){
      env_->localMemoryAssertion(__FILE__, __LINE__, total, false); 
    }
  }

  if (total > 0){
  
    total=0;
    typename std::vector<lno_t>::iterator indexFound;
  
    for (int p=0; p < numProcs_; p++){
      for (lno_t i=0; i < countInBuf[p]; i++, total++){
        double k(IdentifierTraits<gid_t>::key(gidInBuf[total]));
        lno_t index(0);
        bool badGid = false;
        try{
          index = gidToIndex.get(k);
        }
        catch (std::exception &e){
          badGid = true;
        }

        env_->localBugAssertion(__FILE__, __LINE__, "gidToIndex table", 
          badGid || ((index >= 0)&&(index<=indexTotal)), BASIC_ASSERTION);

        
        if (!badGid){
          indexFound = upper_bound(firstIndex.begin(), firstIndex.end(), index);
          procOutBuf[total] = sendProc[indexFound - firstIndex.begin() - 1];
    
          if (needGnoInfo){
            gnoOutBuf[total] = gnoInBuf[index];
          }
        }
        else{
          // globally not one of our gids, can happen in subsetting
          procOutBuf[total] = -1; 
        }
      }
    }
  }

  gidInBuf.release();
  if (needGnoInfo){
    gnoInBuf.release();
  }

  try{
    ArrayView<const int> procView = procOutBuf();
    ArrayView<const lno_t> countView = countOutBuf();
    AlltoAllv<int,lno_t>(*comm_, *env_, procView, countView, procInBuf, countInBuf);
  }
  Z2_FORWARD_EXCEPTIONS;

  procOutBuf.clear();

  if (needGnoInfo){
    try{
      ArrayView<const gno_t> gnoView = gnoOutBuf();
      ArrayView<const lno_t> countView = countOutBuf();
      AlltoAllv<gno_t,lno_t>(*comm_, *env_, gnoView, countView, gnoInBuf, countInBuf);
    }
    Z2_FORWARD_EXCEPTIONS;

    gnoOutBuf.clear();
  }

  countOutBuf.clear();

  ///////////////////////////////////////////////////////////////////////
  // Done.  Process the replies to my queries
  ///////////////////////////////////////////////////////////////////////

  lno_t idx = 0;
  for (next = gidIndices.begin(); next != gidIndices.end(); ++next, ++idx){
    double key = next->first;
    std::vector<lno_t> &v = next->second;
    int gidProc = procInBuf[gidLocation[idx]];

    gno_t gno;
    if (needGnoInfo){
      gno = gnoInBuf[gidLocation[idx]];
    }
    else{
      gid_t gid = IdentifierTraits<gid_t>::keyToGid(key);
      gno = Teuchos::as<gno_t>(gid);
    }

    for (size_t j=0; j < v.size(); j++){
      out_proc[v[j]] = gidProc;
      if (!skipGno)
        out_gno[v[j]] = gno;
    }
  }
}

template< typename User>
  void IdentifierMap<User>::gnoGlobalTranslate(
    ArrayView<const gno_t> in_gno,
    ArrayView<gid_t> out_gid,
    ArrayView<int> out_proc) const
{
  size_t len=in_gno.size();

  if (len == 0){
    return;
  }

  bool skipGid = (out_gid.size() == 0);

  env_->localInputAssertion(__FILE__, __LINE__, 
    "Destination array is too small", 
    (out_proc.size() >= len) && (skipGid || (out_gid.size() >= len)),
    BASIC_ASSERTION);

  if (userGidsAreZoltan2Gnos_){

    // Easy case - use gidGlobalTranslate.

    const gno_t *gnos = in_gno.getRawPtr();
    ArrayView<const gid_t> gids(static_cast<const gid_t *>(gnos), len);
    ArrayView<gno_t> noGnos;

    try{
      gidGlobalTranslate(gids, noGnos, out_proc);
    }
    Z2_FORWARD_EXCEPTIONS;

    if (!skipGid)
      for (lno_t i=0; i < len; i++)
        out_gid[i] = gids[i];

    return;
  }

  // Since user global IDs were mapped to internal global numbers,
  // and since internal global numbers are consecutive, we know the
  // process owning each global number.

  gno_t *gnos = gnoDist_.getRawPtr();
  gno_t *final = gnos + numProcs_ + 1;
  bool remote = false;
  int rank = comm_->getRank();

  for (size_t i=0; i < len; i++){

    env_->localInputAssertion(__FILE__, __LINE__, "invalid global number", 
      in_gno[i] < globalNumberOfIds_, BASIC_ASSERTION);

    gno_t *ub = std::upper_bound(gnos, final, in_gno[i]);

    env_->localBugAssertion(__FILE__, __LINE__, "finding gno", ub != final, 
      BASIC_ASSERTION);
   
    out_proc[i] = (ub - gnos - 1);

    if (!remote && out_proc[i] != rank)
      remote = true;
  }

  if (skipGid)
    return;

  if (!remote){

    // Make a local call to get the gids

    const gno_t *gnos = in_gno.getRawPtr();
    ArrayView<gno_t> gnoList(const_cast<gno_t *>(gnos), len);

    try{
      gidTranslate(out_gid, gnoList, TRANSLATE_LIB_TO_APP);
    }
    Z2_FORWARD_EXCEPTIONS;

    return;
  }

  // Get the global ID from the owner.

  ArrayRCP<lno_t> indexList;
  ArrayRCP<gno_t> gnoOutBuf;

  if (len){
    lno_t *tmpLno = new lno_t [len];
    env_->localMemoryAssertion(__FILE__, __LINE__, len, tmpLno);
    indexList = arcp(tmpLno, 0, len, true);
  
    gno_t *tmpGno = new gno_t [len];
    env_->localMemoryAssertion(__FILE__, __LINE__, len, tmpGno);
    gnoOutBuf = arcp(tmpGno, 0, len, true);
  }

  lno_t *tmpCount = new lno_t [numProcs_];
  env_->localMemoryAssertion(__FILE__, __LINE__, numProcs_, tmpCount);
  ArrayRCP<lno_t> countOutBuf(tmpCount, 0, numProcs_, true);

  lno_t *tmpOff = new lno_t [numProcs_+1];
  env_->localMemoryAssertion(__FILE__, __LINE__, numProcs_+1, tmpOff);
  ArrayRCP<lno_t> offsetBuf(tmpOff, 0, numProcs_+1, true);

  for (int i=0; i < len; i++)
    countOutBuf[out_proc[i]]++;

  for (int i=0; i < numProcs_; i++)
    offsetBuf[i+1] = offsetBuf[i] + countOutBuf[i];

  for (int i=0; i < len; i++){
    int p = out_proc[i];
    int off = offsetBuf[p];
    indexList[off] = i;
    gnoOutBuf[off] = in_gno[i];
    offsetBuf[p]++;
  }

  offsetBuf.clear();

  ArrayRCP<gno_t> gnoInBuf;
  ArrayRCP<lno_t> countInBuf;

  try{
    AlltoAllv<gno_t, lno_t>(*comm_, *env_, 
      gnoOutBuf.view(0, len), countOutBuf.view(0, numProcs_),
      gnoInBuf, countInBuf);
  }
  Z2_FORWARD_EXCEPTIONS;

  gnoOutBuf.clear();
  countOutBuf.clear();

  lno_t numRequests = 0;
  for (int i=0; i < numProcs_; i++)
    numRequests += countInBuf[i];

  ArrayRCP<gid_t> gidQueryBuf;

  if (numRequests){
    gid_t *tmpGid = new gid_t [numRequests];
    env_->localMemoryAssertion(__FILE__, __LINE__, numRequests, tmpGid);
    gidQueryBuf = arcp(tmpGid, 0, numRequests);
  }

  try{
    gidTranslate(gidQueryBuf.view(0, numRequests),
                 gnoInBuf.view(0, numRequests),
                 TRANSLATE_LIB_TO_APP);
  }
  Z2_FORWARD_EXCEPTIONS;

  ArrayRCP<gid_t> gidInBuf;
  ArrayRCP<lno_t> newCountInBuf;

  try{
    AlltoAllv<gid_t, lno_t>(*comm_, *env_, 
      gidQueryBuf.view(0, numRequests), countInBuf.view(0, numProcs_),
      gidInBuf, newCountInBuf);
  }
  Z2_FORWARD_EXCEPTIONS;

  gidQueryBuf.clear();

  // copy in to right place in gid list

  for (int i=0; i < len; i++){
    out_gid[indexList[i]] = gidInBuf[i];
  }
}

template< typename User> 
  void IdentifierMap<User>::setupMap(void)
{
  env_->globalInputAssertion(__FILE__, __LINE__, 
       "application global ID type is not supported yet",
       IdentifierTraits<gid_t>::is_valid_id_type() == true, BASIC_ASSERTION,
       comm_);

  numProcs_ = comm_->getSize(); 
  myRank_ = comm_->getRank(); 

  if (IdentifierTraits<gid_t>::isGlobalOrdinal())
    userGidsAreTeuchosOrdinal_ = true;

  localNumberOfIds_ = myGids_.size();
  const gid_t *gidPtr = myGids_.get();
  ArrayRCP<gid_t> tmpDist;
  gid_t mingid_t=0, maxgid_t=0;

  userGidsAreConsecutive_ = globallyConsecutiveOrdinals<gid_t>(
    *comm_, *env_, gidPtr, localNumberOfIds_,      // input
    tmpDist, globalNumberOfIds_);                  // output

  bool baseZeroConsecutiveIds = false;

  if (userGidsAreTeuchosOrdinal_){
    if (!userGidsAreConsecutive_){
      mingid_t = tmpDist[0];
      maxgid_t = tmpDist[1];
    }
    else{
      // A gno_t is large enough to hold gid_ts, but may be a different type.
      if (sizeof(gid_t) == sizeof(gno_t)) {
        gnoDist_ = arcp_reinterpret_cast<gno_t>(tmpDist);
      }
      else{
        gnoDist_.resize(numProcs_ + 1);
        for (lno_t i=0; i <= numProcs_; i++){
          gnoDist_[i] = static_cast<gno_t>(tmpDist[i]);
        }
      }

      if (gnoDist_[0] == 0)
        baseZeroConsecutiveIds = true;
    }
  }

  // If user global IDs are not consecutive ordinals, create a hash table
  // mapping the global IDs to their location in myGids_.

  typedef typename Teuchos::Hashtable<double, lno_t> id2index_hash_t;

  if (!userGidsAreConsecutive_){
    id2index_hash_t *p = NULL;
    if (localNumberOfIds_){
      try{
        p = new id2index_hash_t(localNumberOfIds_);
      }
      catch (const std::exception &e){
        env_->localMemoryAssertion(__FILE__, __LINE__, localNumberOfIds_, 
          false);
      }
    }

    for (size_t i=0; i < localNumberOfIds_; i++){
      try{
        p->put(IdentifierTraits<gid_t>::key(gidPtr[i]), i);
      }
      Z2_THROW_OUTSIDE_ERROR(*env_);
    }
    gidHash_ = RCP<id2index_hash_t>(p);
  }

  // Can we use the user's global IDs as our internal global numbers?
  // If so, we can save memory.

  if (consecutiveGidsAreRequired_ && userGidsAreConsecutive_ &&
       !baseZeroConsecutiveIds){

  } 

  userGidsAreZoltan2Gnos_ = false;

  if (userGidsAreTeuchosOrdinal_ &&
      ((consecutiveGidsAreRequired_ && baseZeroConsecutiveIds) ||
      !consecutiveGidsAreRequired_))
  {
    // We can use the application's global IDs
    userGidsAreZoltan2Gnos_ = true;
  }

  if (userGidsAreZoltan2Gnos_){

    zoltan2GnosAreConsecutive_ = userGidsAreConsecutive_;

    if (userGidsAreConsecutive_){
      minGlobalGno_ = gnoDist_[0];
      maxGlobalGno_ = gnoDist_[numProcs_]-1;
    }
    else{
      minGlobalGno_ = static_cast<gno_t>(mingid_t);
      maxGlobalGno_ = static_cast<gno_t>(maxgid_t);
    }
  } else{
    // We map application gids to consecutive global numbers starting with 0.

    zoltan2GnosAreConsecutive_ = true;

    try{
      gnoDist_.resize(numProcs_ + 1, 0);
    }
    Z2_THROW_OUTSIDE_ERROR(*env_);

    gno_t myNum = static_cast<gno_t>(localNumberOfIds_);

    try{
      gno_t *p = gnoDist_.getRawPtr();
      Teuchos::gatherAll<int, gno_t>(*comm_, 1, &myNum, numProcs_, p+1);
    }
    Z2_THROW_OUTSIDE_ERROR(*env_);

    for (int i=2; i <= numProcs_; i++){
      gnoDist_[i] += gnoDist_[i-1];
    }

    minGlobalGno_ = gnoDist_[0];
    maxGlobalGno_ = gnoDist_[numProcs_]-1;
  }
}

}   // end namespace Zoltan2

#endif /* _ZOLTAN2_IDENTIFIERMAP_HPP_ */
