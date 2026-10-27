#pragma once

template<typename Key, typename Value>
class QwThreadSafeMap
{
private:
	std::map<Key, Value> fMap;
	mutable std::shared_mutex fRW_mutex;
public:
	void InsertOrAssign(Key const& key, Value const& value);
	std::optional<Value> Get(Key const& key) const;
	bool Find(Key const& key) const;
	void Remove(Key const& key);
	void Clear();
	template<typename Callable>
	void ForEach(Callable&& callable);
	template<typename Callable>
	void ForEach(Callable&& callable) const;
public:
	QwThreadSafeMap() = default;
	~QwThreadSafeMap() = default;
	QwThreadSafeMap(QwThreadSafeMap const& other);
	QwThreadSafeMap(QwThreadSafeMap&& other) noexcept;
	QwThreadSafeMap& operator=(QwThreadSafeMap const& other);
	QwThreadSafeMap& operator=(QwThreadSafeMap&& other);
};


template<typename Key, typename Value>
void QwThreadSafeMap<Key, Value>::InsertOrAssign(Key const& key, Value const& value)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	fMap[key] = value;
}
template<typename Key, typename Value>
std::optional<Value> QwThreadSafeMap<Key, Value>::Get(Key const& key) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	auto it = fMap.find(key);
	return (it == fMap.end()) ? std::nullopt : std::optional(it->second);
}
template<typename Key, typename Value>
bool QwThreadSafeMap<Key, Value>::Find(Key const& key) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	auto it = fMap.find(key);
	return (it == fMap.end()) ? false : true;
}
template<typename Key, typename Value>
void QwThreadSafeMap<Key, Value>::Remove(Key const& key)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	fMap.erase(key);
}
template<typename Key, typename Value>
void QwThreadSafeMap<Key, Value>::Clear()
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	if constexpr (std::is_pointer_v<Value>) {
		for (auto const& [key, ptr] : fMap) {
			delete ptr;
		}
	}
	fMap.clear();
}

template<typename Key, typename Value> template<typename Callable>
void QwThreadSafeMap<Key, Value>::ForEach(Callable&& callable)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	for (auto& [key, value] : fMap) {
		callable(key, value);
	}
}

template<typename Key, typename Value> template<typename Callable>
void QwThreadSafeMap<Key, Value>::ForEach(Callable&& callable) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	for (auto const& [key, value] : fMap) {
		callable(key, value);
	}
}

template<typename Key, typename Value>
QwThreadSafeMap<Key, Value>::QwThreadSafeMap(QwThreadSafeMap<Key, Value> const& other)
{
	std::shared_lock<std::shared_mutex> lk(other.fRW_mutex);
	fMap = other.fMap;
}

template<typename Key, typename Value>
QwThreadSafeMap<Key, Value>::QwThreadSafeMap(QwThreadSafeMap&& other) noexcept
{
	std::unique_lock<std::shared_mutex> lk(other.fRW_mutex);
	fMap = std::move(other.fMap);
}

template<typename Key, typename Value>
QwThreadSafeMap<Key, Value>& QwThreadSafeMap<Key, Value>::operator=(QwThreadSafeMap const& other)
{
	if(this == &other) return *this;
	std::scoped_lock lk(fRW_mutex, other.fRW_mutex);
	fMap = other.fMap;
	return *this;
}
template<typename Key, typename Value>
QwThreadSafeMap<Key, Value>& QwThreadSafeMap<Key, Value>::operator=(QwThreadSafeMap&& other)
{
	if(this == &other) return *this;
	std::scoped_lock lk(fRW_mutex, other.fRW_mutex);
	fMap = std::move(other.fMap);
	return *this;
}

