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
template<class Base, class... Args>
class Factory {
public:
	friend Base;
	template<class ...T>
	static std::unique_ptr<Base> Create(const std::string &type, T&&... args) {
		std::unique_ptr<Base> ptr{};
		if(InspectFactoryForKey(type)== false) {
			QwError   << "Type " << type << " is not registered!" << QwLog::endl;
			QwMessage << "Available types:" << QwLog::endl;
			ListRegisteredTypes();
			QwWarning << "To Register a type with the factory\n";
			QwWarning << "Declare your class as\n";
			QwWarning << "\tclass " << type << " : public Factory::Registrar<" << type << ">";
			QwWarning << QwLog::endl;
			throw( QwException_TypeUnknown() ); // This is horrible error handling
		} else {
			ptr = data().at(type)(std::forward<T>(args)...);	
		}
		return ptr;
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
			Factory::Casts()[name] = [](std::unique_ptr<Base> base)->bool {
				return (dynamic_cast<T*>(base.get()) != nullptr);
			};
			return true;
		}
		static bool registered;
	private:
		// Force Registrar to be instantiated
		Registrar() : Base(Key{}) { (void)registered;}
	public:
		// std::unique_ptr<T>
	};
private:
	// Passkey Idion -- Prevents inheritance from the base (CRTP) class
	// The Base (CRTP) class will need to take a Key{} object in its constructor
	class Key {
		Key(){};
		template<class T> friend struct Registrar;
	};

private:
	// Function Pointer to our GetFactory Method
	using FuncType = std::unique_ptr<Base>(*)(Args...);
	Factory() = default;
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

public:
    /// Dynamic cast of object into type
    static bool Cast(std::unique_ptr<Base> base, const std::string& type) {
	  // I need to think on what to do here
      return Casts().at(type)(base);
    }

    /// Test whether object inherits from type
    static bool InheritsFrom(std::unique_ptr<Base> base, const std::string& type) {
      return (Cast(base,type) != 0);
    }
};

// Fun Part
template<class Base, class... Args>
template<class T>
bool Factory<Base,Args...>::Registrar<T>::registered =
	Factory<Base,Args...>::Registrar<T>::registerT();

/*
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
template <class Base, class Type>
class MQwCloneable: virtual public VQwCloneable<Base> {

  public:

    /// Virtual destructor
    virtual ~MQwCloneable() { };

    /// Concrete clone method
    virtual auto Clone() const {
      return std::make_unique<Base>(static_cast<const type_t&>(*this));
    }

    /// Object dynamic cast
    static auto Cast(std::unique_ptr<type>) {
      return dynamic_cast<Type>(Base);
    }
	static auto Cast(std::unique_ptr<Base> base, std::string &type) {
		data().at(type)->Cast(base);
	}

    /// Test whether object inherits from type
    static bool InheritsFrom(std::unique_ptr<Base>, const std::string& type) {
      return (Cast(base,type) != 0);
    }

}; // class MQwCloneable
*/
// Factory type with functionality for data Handlers
// CRTP pattern, Animal is injecting functionality
// This is creating a Factor of type Animal
// that has an int member variable
struct Animal : Factory<Animal, int> {

  // Define Interface Here
  Animal(Key) {}
  virtual void makeNoise() = 0;

  // We want to be able to
  // * cast ()
  // * InheritFrom()


};
/*
// Factory type with functionality for data Handlers
// CRTP pattern, QwHandlerFactory is injecting functionality
// We will want to add our functionality for our factory here
class VQwDataHandlerFactory : public Factory<VQwDataHandlerFactory, VQwDataHandler>
{
	// We define our Interface here
	QwhandlerFactory(Key){}
	virtual void makeNoise() = 0;

}
*/

};



//template<>
// const VQwDataHandlerFactory* MQwCloneable<VQwDataHandler,QwPitaFeedback>::fFactory
//	= new QwFactory<VQwDataHandler,QwPitaFeedback>("QwPitaFeedback");

#endif
