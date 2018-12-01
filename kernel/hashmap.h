#ifndef HASHMAP_H
#define HASHMAP_H
#define MAP_INITIAL_SIZE 101
#include "atomic.h"

static bool IsPrime(uint32_t number)
{
    if (number == 2 || number == 3)
        return true;
    if (number % 2 == 0 || number % 3 == 0)
        return false;
    uint32_t divisor = 6;
    while (divisor * divisor - 2 * divisor + 1 <= number)
    {
        if (number % (divisor - 1) == 0)
            return false;
        if (number % (divisor + 1) == 0)
            return false;
        divisor += 6;
    }
    return true;
}

static uint32_t NextPrime(uint32_t a)
{
    while (!IsPrime(++a));
    return a;
}


template <typename K>
struct HashCode {
    uint32_t operator()(const K key) const {
        return reinterpret_cast<int32_t>(key);
    }
};


template <class K, class V, class H=HashCode<K> >
class Hashmap
{
	private:
		class Hashnode
		{
			private:
				K key;
				V val;
				Hashnode* next;
			public:
				Hashnode(K key, V val) : key(key), val(val), next(nullptr) {}
				~Hashnode(){}
			friend class Hashmap<K,V>;
		};

	private:
		//hash table
		Hashnode** table;
		//num Elements
		uint32_t size;
		//current container bucket_count
		uint32_t bucket_count;
		//hash functor
		H hash;
		
		InterruptSafeLock mLock;

		const float load_limit = 0.75f;

		void resize();

	public:
		Hashmap();
		~Hashmap();
		V& at(K key);
		bool find(K key);
		bool insert(const K key, const V val);
		bool erase(const K key);
		void clear();
		bool empty() const;
		uint32_t num_buckets() const;
		uint32_t getSize() const;
		V& operator[](const K key);
};


/* Hashmap */
template <class K, class V, class H>
Hashmap<K,V,H>::Hashmap()
{
	this->table = new Hashnode*[MAP_INITIAL_SIZE]();
	this->size = 0;
	this->bucket_count = MAP_INITIAL_SIZE;
}

template <class K, class V, class H>
Hashmap<K,V,H>::~Hashmap()
{
	for (uint32_t i = 0; i < this->bucket_count; i++) {
	    Hashnode* curNode = this->table[i];
	    while (curNode != nullptr) {
	        Hashnode* previous = curNode;
	        curNode = curNode->next;
	        delete previous;
	    }
	    this->table[i] = nullptr;
	}
	delete[] this->table;
}

template <class K, class V, class H>
bool Hashmap<K,V,H>::find(K key)
{
	InterruptSafeLocker locker(mLock);

	uint32_t hashCode = this->hash(key);
	Hashnode* curNode = this->table[hashCode % this->bucket_count];
	while(curNode != nullptr)
	{
		if(curNode->key == key)
		{
			return true;
		}
		curNode = curNode->next;
	}
	return false;
}

template <class K, class V, class H>
V& Hashmap<K,V,H>::at(K key)
{
	InterruptSafeLocker locker(mLock);

	uint32_t hashCode = this->hash(key);
	Hashnode* curNode = this->table[hashCode % this->bucket_count];
	while(curNode != nullptr)
	{
		if(curNode->key == key)
		{
			return curNode->val;
		}
		curNode = curNode->next;
	}
	// should really throw something here
	return curNode->val;
}

template <class K, class V, class H>
bool Hashmap<K,V,H>::insert(const K key, const V val)
{
	InterruptSafeLocker locker(mLock);

	if(this->size / (float) this->bucket_count > load_limit)
	{
		this->resize();
	}
	uint32_t hashCode = this->hash(key);
	Hashnode* curNode = this->table[hashCode % this->bucket_count];
	Hashnode* prevNode = nullptr;

	while(curNode != nullptr)
	{
		if(curNode->key == key)
		{
			return false;
		}
		prevNode = curNode;
		curNode = curNode->next;
	}
	Hashnode* tmpNode = new Hashnode(key, val);
	//first element in bucket
	if (prevNode == nullptr)
	{
		this->table[hashCode % this->bucket_count] = tmpNode;
	}
	//append to end
	else
	{
		prevNode->next = tmpNode;
	}
	this->size++;
	return true;
}

template <class K, class V, class H>
bool Hashmap<K,V,H>::erase(const K key)
{
	InterruptSafeLocker locker(mLock);
	
	uint32_t hashCode = this->hash(key);
	Hashnode* curNode = this->table[hashCode % this->bucket_count];
	Hashnode* prevNode = nullptr;

	while(curNode != nullptr)
	{
		if(curNode->key == key)
		{
			//first bucket
			if(prevNode == nullptr)
			{
				this->table[hashCode % this->bucket_count] = nullptr;
			}
			else
			{
				prevNode->next = curNode->next;
			}
			delete curNode;
			this->size--;

			return true;
		}
		prevNode = curNode;
		curNode = curNode->next;
	}

	return false;
}

template <class K, class V, class H>
void Hashmap<K,V,H>::clear()
{
	InterruptSafeLocker locker(mLock);

	for (uint32_t i = 0; i < this->bucket_count; i++) {
	    Hashnode* curNode = this->table[i];
	    while (curNode != nullptr) {
	        Hashnode* previous = curNode;
	        curNode = curNode->next;
	        delete previous;
	    }
	    this->table[i] = nullptr;
	}
	this->size = 0;
}

template <class K, class V, class H>
bool Hashmap<K,V,H>::empty() const
{
	return this->size == 0;
}

template <class K, class V, class H>
uint32_t Hashmap<K,V,H>::num_buckets() const
{
	return this->bucket_count;
}

template <class K, class V, class H>
uint32_t Hashmap<K,V,H>::getSize() const
{
	return this->size;
}

template <class K, class V, class H>
V& Hashmap<K,V,H>::operator[](const K key)
{
	InterruptSafeLocker locker(mLock);

	if(this->size / (float) this->bucket_count > load_limit)
	{
		this->resize();
	}
	uint32_t hashCode = this->hash(key);
	Hashnode* curNode = this->table[hashCode % this->bucket_count];
	Hashnode* prevNode = nullptr;

	while(curNode != nullptr)
	{
		if(curNode->key == key)
		{
			return curNode->val;
		}
		prevNode = curNode;
		curNode = curNode->next;
	}
	Hashnode* tmpNode = new Hashnode(key, V());
	//first element in bucket
	if (prevNode == nullptr)
	{
		this->table[hashCode % this->bucket_count] = tmpNode;
	}
	//append to end
	else
	{
		prevNode->next = tmpNode;
	}
	this->size++;
	return tmpNode->val;
}

template <class K, class V, class H>
void Hashmap<K,V,H>::resize()
{
	InterruptSafeLocker locker(mLock);

	uint32_t new_bucket_count = NextPrime(this->bucket_count * 2 + 1);
	Hashnode** oldTable = this->table;
	uint32_t old_bucket_count = this->bucket_count;
	this->table = new Hashnode*[new_bucket_count]();
	this->bucket_count = new_bucket_count;
	this->size = 0;

	for (uint32_t i = 0; i < old_bucket_count; i++) {
	    Hashnode* curNode = oldTable[i];
	    while (curNode != nullptr) {
	    	this->insert(curNode->key, curNode->val);
	        Hashnode* previous = curNode;
	        curNode = curNode->next;
	        delete previous;
	    }
	    oldTable[i] = nullptr;
	}

	delete[] oldTable;
}

#endif