#ifndef MUELU_LEVEL_HPP
#define MUELU_LEVEL_HPP

#include <iostream>
#include <sstream>

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_Exceptions.hpp"
#include "MueLu_Needs.hpp"
#include "MueLu_FactoryBase.hpp"
#include "MueLu_NoFactory.hpp"
#include "MueLu_FactoryManagerBase.hpp"

namespace MueLu {

  /*!
    @class Level
    @brief Class that holds all level-specific information.

    All data is stored in an associative list. See the Needs class for more information.

    The Level class uses the functionality of the Needs class with the extended hashtables and
    adds the handling of default factories.
    All data that is stored in the <tt>Level</tt> class need a variable name (e.g. "A", "P", ...) and
    a pointer to the generating factory. Only with both the variable name and the generating factory
    the data can be accessed.

    If no pointer to the generating factory is provided (or it is NULL) then the Level class
    uses the information from a factory manager, which stores default factories for different
    variable names.
  */
  class Level : public BaseClass {

  public:

    //@{

    //! @name Constructors / Destructors
    Level();

    //! Constructor
    Level(RCP<FactoryManagerBase> & factoryManager);

    //@}

    //@{
    //! @name Build methods
    //! Builds a new Level object.
    RCP<Level> Build();

    //@}

    //! Destructor
    virtual ~Level();

    //@{
    //! @name Level handling

    //! @brief Return level number.
    int GetLevelID() const;

    //! @brief Set level number.
    void SetLevelID(int levelID);

    //! Previous level
    RCP<Level> & GetPreviousLevel();

    //! Set previous level object
    //! @\param[in] const RCP<Level>& previousLevel
    void SetPreviousLevel(const RCP<Level> & previousLevel);

    //! @name Set factory manager
    //@{
    //! Set default factories (used internally by Hierarchy::SetLevel()).
    // Users should not use this method.
    void SetFactoryManager(const RCP<const FactoryManagerBase> & factoryManager);

    //@}

    //@{
    //! @name Get functions

    //! Store need label and its associated data. This does not increment the storage counter.
    //! - If factory is not specified, mark data as user-defined
    //! - If factory == NULL, use defaultFactory (if available).
    template <class T>
    void Set(const std::string & ename, const T &entry, const FactoryBase* factory = NoFactory::get()) {
      const FactoryBase* fac = GetFactory(ename, factory);
      
      if (fac == NoFactory::get()) {
        // user defined data
        // keep data
        Keep(ename, NoFactory::get());
      }
      
      needs_.Set<T>(ename, entry, fac);
      
    } // Set

    //@}

    //! @name Get functions
    //! @brief Get functions for accessing stored data

    //@{
    /*! @brief Get data without decrementing associated storage counter (i.e., read-only access).
     *   Usage: Level->Get< RCP<Operator> >("A", factory)
     *   if factory == NULL => use default factory
     *
     *  @param[in] const std::string& ename
     *  @param[in] const FactoryBase* factory
     *  @return data (templated)
     * */
    template <class T>
    T & Get(const std::string& ename, const FactoryBase* factory = NoFactory::get()) {
      const FactoryBase* fac = GetFactory(ename, factory);

      if (!IsAvailable(ename, fac)) {
        TEUCHOS_TEST_FOR_EXCEPTION(needs_.NumRequests(ename, fac) < 1 && !needs_.IsKept(ename, fac), Exceptions::RuntimeError, 
                                   "MueLu::Level::Get(): " << ename << " has not been requested (counter = " << needs_.NumRequests(ename, fac) << ", IsKept() = " << needs_.IsKept(ename, fac) << "). " << std::endl << "Generating factory:" << *fac);
        
        fac->CallBuild(*this);
        Release(*fac);
      }
      
      TEUCHOS_TEST_FOR_EXCEPTION(!IsAvailable(ename, fac), Exceptions::RuntimeError, "MueLu::Level::Get(): factory did not produce expected output. " << ename << " has not been generated by " << *fac);
      
      return needs_.Get<T>(ename, fac);
    }

    /*! @brief Get data without decrementing associated storage counter (i.e., read-only access).*/
    template <class T>
    void Get(const std::string& ename, T& Value, const FactoryBase* factory = NoFactory::get()){
      Value = Get<T>(ename, factory);
    }

    //TODO: add a const version of Get()?

    //@}

    //! @name Permanent storage
    //@{

    //! keep variable 'ename' generated by 'factory'
    void Keep(const std::string& ename, const FactoryBase* factory = NoFactory::get());

    //! returns true, if 'ename' generated by 'factory' is marked to be kept
    bool IsKept(const std::string& ename, const FactoryBase* factory = NoFactory::get()) const;

    /*! @brief remove the permanently stored variable 'ename' generated by 'factory' */
    void Delete(const std::string& ename, const FactoryBase* factory = NoFactory::get());

    //@}

    //! @name Request/Release functions
    //! @brief Request and Release for incrementing/decrementing the reference count pointer for a specific variable.
    //@{

    //! Increment the storage counter for all the inputs of a factory
    void Request(const FactoryBase& factory);

    //! Decrement the storage counter for all the inputs of a factory
    void Release(const FactoryBase& factory);

    //! Callback from FactoryBase::CallDeclareInput() and FactoryBase::DeclareInput()
    void DeclareInput(const std::string& ename, const FactoryBase* factory);

    //! Callback from FactoryBase::CallDeclareInput() and FactoryBase::DeclareInput() to declare factory dependencies
    void DeclareDependencies(const FactoryBase* factory, bool bRequestOnly = false, bool bReleaseOnly = false);

    //! Indicate that an object is needed. This increments the storage counter.
    void Request(const std::string& ename, const FactoryBase* factory = NoFactory::get());

    //! Decrement the storage counter.
    void Release(const std::string& ename, const FactoryBase* factory = NoFactory::get());

    //@}

    //! @name Utility functions
    //@{

    //! Test whether a need's value has been saved.
    bool IsAvailable(const std::string & ename, const FactoryBase* factory = NoFactory::get());

    //! Test whether a need has been requested.  Note: this tells nothing about whether the need's value exists.
    bool IsRequested(const std::string & ename, const FactoryBase* factory = NoFactory::get());

    //@}

    //! @name I/O Functions
    //@{

    //! Printing method
    // TODO: print only shows requested variables. check if we also list kept factories with ref counter=0?
    void print(Teuchos::FancyOStream &out, const VerbLevel verbLevel = Default) const;

    //@}

  private:
    
    //! Copy constructor.
    Level(const Level& source);
    //
    // explicit Level(const Level& source);

    //! If input factory == NULL, returns the default factory. Else, return input factory.
    const FactoryBase* GetFactory(const std::string& varname, const FactoryBase* factory) const;

    enum   RequestMode { REQUEST, RELEASE, UNDEF }; //EI TODO
    static RequestMode requestMode_;                //EI TODO

    //
    //
    //
  
    int levelID_; // id number associated with level
    RCP<const FactoryManagerBase> factoryManager_;
    RCP<Level> previousLevel_;  // linked list of Level

    Needs needs_;

  }; //class Level

} //namespace MueLu

//TODO: Caps should not matter

#define MUELU_LEVEL_SHORT
#endif // MUELU_LEVEL_HPP
