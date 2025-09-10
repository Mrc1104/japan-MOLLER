#ifndef __NEW_QWFACTORY__
#define __NEW_QWFACTORY__
#include <memory>
#include <string>
#include <utility>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cxxabi.h>

#include "QwLog.h"
// Forward declarations
class VQwSubsystem;
class VQwDataElement;
class VQwDataHandler;


namespace EXPERIMENTAL
{

// Exceptions
struct QwException_TypeUnknown {
  QwException_TypeUnknown() { }
};

std::string demangle(const char* name)
{
	int status = -4; // Random init to supress compile warning
	std::unique_ptr<char, decltype(&std::free)> res {
		abi::__cxa_demangle(name, NULL, NULL, &status), free };
	return (status == 0) ? res.get() : name;
}

/* Following the Blog: https://www.nirfriedman.com/2018/04/29/unforgettable-factory/ */
// Base is following the CRTP pattern to inject functionality
// Base = Animal  <-> base_t
// Args = int
// T = Dog        <-> type_t
template<class Base, class... Args>
class Factory {
public:
	friend Base;
	template<class ...T>
	static std::unique_ptr<Base> Create(const std::string &type, T&&... args) {
		if(InspectFactoryForKey(type)== false) {
			QwError   << "Type " << type << " is not registered!" << QwLog::endl;
			QwMessage << "Available types:" << QwLog::endl;
			ListRegisteredTypes();
			QwWarning << "To Register a type with the factory\n";
			QwWarning << "Declare your class as\n";
			QwWarning << "\tclass " << type << " : public Factory::Registrar<" << type << ">";
			QwWarning << QwLog::endl;
			throw( QwException_TypeUnknown() );
		}
		// We already bounds checked
		return data()[type](std::forward<T>(args)...);	
	}
	static bool InspectFactoryForKey(const std::string &type) {
		return data().count(type);
	}
    /// List available type factories
	static void ListRegisteredTypes() {
		std::for_each(data().begin(),data().end(), [](const auto &entry)
		{
			QwMessage << entry.first << QwLog::endl;
		});
	}

	/// Dynamic cast of object into type
	static bool Cast(std::unique_ptr<Base> base, const std::string& type) {
		// I need to think on what to do here
		// Casts()[type] can throw, check first
		if(InspectFactoryForKey(type)== false) {
			QwError   << "Type " << type << " is not registered!" << QwLog::endl;
			QwMessage << "Available types:" << QwLog::endl;
			ListRegisteredTypes();
			QwWarning << "To Register a type with the factory\n";
			QwWarning << "Declare your class as\n";
			QwWarning << "\tclass " << type << " : public Factory::Registrar<" << type << ">";
			QwWarning << QwLog::endl;
			throw( QwException_TypeUnknown() );
		}
		// We already bounds checked
		return Casts()[type](base);
	}

    /// Test whether object inherits from type
    static bool InheritsFrom(std::unique_ptr<Base> base, const std::string& type) {
      return (Cast(base,type) != 0);
    }
public:
	// Registration
	template<class T>
	struct Registrar : Base {
		friend T;
		static bool registerT() {
			const auto name = demangle(typeid(T).name());
			// Fill the map of Function Pointers
			Factory::data()[name] = [](Args ...args)->std::unique_ptr<Base> {
				return std::make_unique<T>(std::forward<Args>(args)...);
			};
			// Define how we will cast to the base type
			// T = QwHelicity
			// Base = VQwSusbsystem
			Factory::Casts()[name] = [](std::unique_ptr<Base> base)->bool {
				return (dynamic_cast<T*>(base.get()) != nullptr);
			};
			return true;
		}
		static bool registered;
	private:
		// Force Registrar to be instantiated
		Registrar() : Base(Key{}) { (void)registered; }
	};
private:
	// Passkey Idiom -- Prevents inheritance from the base (CRTP) class
	// The Base (CRTP) class will need to take a Key{} object in its constructor
	class Key {
		Key(){};
		template<class T> friend struct Registrar;
	};

private:
	Factory() = default;

	// Function Pointer to our GetFactory Method
	using FuncType = std::unique_ptr<Base>(*)(Args...);
	// Map from string to concrete types function pointer
	static auto &data() {
		static std::unordered_map<std::string, FuncType> s;
		return s;
	}

	using CastType = bool(*)(std::unique_ptr<Base>);
	static auto &Casts() {
		static std::unordered_map<std::string, CastType> c;
		return c;
	}
};

// Fun Part
template<class Base, class... Args>
template<class T>
bool Factory<Base,Args...>::Registrar<T>::registered =
	Factory<Base,Args...>::Registrar<T>::registerT();

/// Polymorphic copy constructor virtual base class
template <class Base>
class VQwCloneable {

  public:

    /// Virtual destructor
    virtual ~VQwCloneable() { }

    /// Get demangled name of this class
    std::string GetClassName() const {
	  return demangle(typeid(*this).name());
    }

    /// Abstract clone method when no derived method is defined
    virtual std::unique_ptr<Base> Clone() const {
	  const auto name = GetClassName();
      QwError << "Clone() is not implemented for class " << name << "!" << QwLog::endl;
      QwMessage << "Modify the class definition of " << name << " to:" << QwLog::endl;
      QwMessage << "  class " << name << ": "
                << "public MQwSomethingCloneable<" << name << ">" << QwLog::endl;
      return std::unique_ptr<Base>{};
    }

}; // class VQwCloneable
/// Polymorphic copy construction by curiously recurring template pattern (mix-in)
/// We have lost covariancy: clone will have the base type, not the derived type...
// Base = VQwSubsystem
// Type = QwHelicity
template <class Base, class Type>
class MQwCloneable: virtual public VQwCloneable<Base> {

  public:

    /// Virtual destructor
    virtual ~MQwCloneable() { };

    /// Concrete clone method
    virtual std::unique_ptr<Base> Clone() const {
      return std::make_unique<Base>(static_cast<const Type&>(*this));
    }
}; // class MQwCloneable

template <class subsystem_t>
class MQwDataHandlerCloneable: public MQwCloneable<VQwDataHandler,subsystem_t> { };
/// Mix-in factory functionality for subsystems
typedef class VQwCloneable<VQwSubsystem> VQwSubsystemCloneable;
template <class subsystem_t>
class MQwSubsystemCloneable: public MQwCloneable<VQwSubsystem,subsystem_t> { };
/// Mix-in factory functionality for data elements
typedef class VQwCloneable<VQwDataElement> VQwDataElementCloneable;
template <class dataelement_t>
class MQwDataElementCloneable: public MQwCloneable<VQwDataElement,dataelement_t> { };

// Factory type with functionality for data Handlers

// CRTP pattern, Animal is injecting functionality
// This is creating a Factor of type Animal
// that has an int member variable

// I need to support
// * InheritFrom
// * Clone
struct Animal : Factory<Animal, int> {

  // Define Interface Here
  Animal(Key) {}
  virtual void makeNoise() = 0;
  virtual void loop() = 0;

  // We want to be able to
  // * Cast ()
  // * InheritFrom()
  // * Clone()

};

// Base = Animal
// 
// T = Dog
// class Dog : public Animal::Registrar<Dog> {

// Constructor that we call
// QwFactory<QwSubsystem,QwHelicity>("QwHelicity");

/*
// Factory type with functionality for data Handlers
// CRTP pattern, QwHandlerFactory is injecting functionality
// We will want to add our functionality for our factory here
class VQwDataHandlerFactory : public Factory<VQwDataHandlerFactory, VQwDataHandler>
{
	// I want to define that MQwCloneable stuff here
	// We define our Interface here
	VQwDataHandlerFactory(Key){}
	virtual void makeNoise() {}

};
*/

};



//template<>
// const VQwDataHandlerFactory* MQwCloneable<VQwDataHandler,QwPitaFeedback>::fFactory
//	= new QwFactory<VQwDataHandler,QwPitaFeedback>("QwPitaFeedback");

#endif
