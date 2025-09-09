#ifndef __NEW_QWFACTORY__
#define __NEW_QWFACTORY__
#include <memory>
#include <string>
#include <utility>
#include <map>
#include <unordered_map>
#include <cxxabi.h>

namespace EXPERIMENTAL
{

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
	template<class ...T>
	static std::unique_ptr<Base> Create(const std::string &s, T&&... args) {
		return data().at(s)(std::forward<T>(args)...);
	}
	friend Base;
public:
	// Registration
	template<class T>
	struct Registrar : Base {
		friend T;
		static bool registerT() {
			const auto name = demangle(typeid(T).name());
			Factory::data()[name] = [](Args ...args)->std::unique_ptr<Base> {
				return std::make_unique<T>(std::forward<Args>(args)...);
			};
			return true;
		}
		static bool registered;
	private:
		// Force Registrar to be instiated
		Registrar() : Base(Key{}) { (void)registered;}
	};
private:
	// Passkey Idion -- Prevents inheritance from the base (CRTP) class
	// The Base (CRTP) class will need to take a Key{} object in its constructor
	class Key {
		Key(){};
		template<class T> friend struct Registrar;
	};


private:
	using FuncType = std::unique_ptr<Base>(*)(Args...);
	Factory() = default;
	static auto &data() {
		static std::unordered_map<std::string, FuncType> s;
		return s;
	}

};

// Fun Part
template<class Base, class... Args>
template<class T>
bool Factory<Base,Args...>::Registrar<T>::registered =
	Factory<Base,Args...>::Registrar<T>::registerT();


/* Usage */
struct Animal : Factory<Animal, int> {
  Animal(Key) {}
  virtual void makeNoise() = 0;
};


};
#endif
