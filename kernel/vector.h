#ifndef VECTOR_H
#define VECTOR_H

#define INITIAL_VECTOR_CAPACITY 10


template<class T>
class vector
{
	private:
		T* v_array;
		uint32_t v_capacity;
		uint32_t v_size;

		void resize(uint32_t capacity);
	public:
		vector();
		vector(uint32_t initial_size);
		vector(uint32_t initial_size, T initial_val);
		vector(const vector<T>& cp);
		vector(vector<T>&& cp);
		~vector();
		void reserve(uint32_t capacity);
		void erase(uint32_t idx);
		bool empty() const;
		void clear();
		void insert(T val, uint32_t idx);
		void push_back(T val);
		bool pop_back();
		uint32_t size() const;
		uint32_t capacity() const;
		uint32_t find(T val);
		T& at(uint32_t idx) const;
		T& front() const;
		T& back() const;
		T* data() const;
		T& operator[](const int idx) const;
};


template<class T>
vector<T>::vector()
{
	this->v_array = new T[INITIAL_VECTOR_CAPACITY]();
	this->v_size = 0;
	this->v_capacity = INITIAL_VECTOR_CAPACITY;
}

template<class T>
vector<T>::vector(uint32_t initial_size)
{
	this->v_capacity = initial_size * 2 + 1;
	this->v_size = initial_size;
	this->v_array = new T[this->v_capacity]();
}

template<class T>
vector<T>::vector(uint32_t initial_size, T initial_val)
{
	this->v_capacity = initial_size * 2 + 1;
	this->v_size = initial_size;
	this->v_array = new T[this->v_capacity]();
	for(uint32_t i = 0; i < initial_size; i++)
	{
		this->v_array[i] = initial_val;
	}
}

template<class T>
vector<T>::vector(const vector<T>& cp)
{
	this->v_array = new T[cp.v_capacity]();
	this->v_size = cp.v_size;
	this->v_capacity = cp.v_capacity;
	for(uint32_t i = 0; i < this->v_size; i++)
	{
		this->v_array[i] = cp[i];
	}
}

template<class T>
vector<T>::vector(vector<T>&& cp)
{
	this->v_array = cp.v_array;
	this->v_size = cp.v_size;
	this->v_capacity = cp.v_capacity;
	cp.v_array = nullptr;
	cp.v_size = 0;
	cp.v_capacity = 0;
}

template<class T>
vector<T>::~vector()
{
	delete[] this->v_array;
}

template<class T>
void vector<T>::resize(uint32_t capacity)
{
	// Debug::printf("*** in vr\n" );
	T* old_arr = this->v_array;
	this->v_array = new T[capacity]();
	this->v_capacity = capacity;
	for(uint32_t i = 0; i < this->v_size; i++)
	{
		this->v_array[i] = old_arr[i];
	}
	delete[] old_arr;
}

template<class T>
void vector<T>::reserve(uint32_t capacity)
{
	this->resize(capacity);
}

template<class T>
void vector<T>::erase(uint32_t idx)
{
	for(uint32_t i = idx; i < this->v_size - 1; i++)
	{
		this->v_array[i] = this->v_array[i + 1];
	}
	this->v_size--;
}

template<class T>
bool vector<T>::empty() const
{
	return this->size == 0;
}

template<class T>
void vector<T>::clear()
{
	this->v_size = 0;
}

template<class T>
void vector<T>::insert(T val, uint32_t idx)
{
	if(this->v_size == this->v_capacity)
	{
		this->resize(this->v_capacity * 2 + 1);
	}

	this->v_size++;

	T tmp;
	for(uint32_t i = idx; i < this->v_size; i ++)
	{
		tmp = this->v_array[i];
		this->v_array[i] = val;
		val = tmp;
	}
}

template<class T>
void vector<T>::push_back(T val)
{
	if(this->v_size == this->v_capacity)
	{
		this->resize(this->v_capacity * 2 + 1);
	}
	this->v_array[(this->v_size)++] = val;
}

template<class T>
bool vector<T>::pop_back()
{
	if (this->v_size == 0)
		return false;
	this->v_size--;
}

template<class T>
uint32_t vector<T>::size() const
{
	return this->v_size;
}

template<class T>
uint32_t vector<T>::capacity() const
{
	return this->v_capacity;
}

template<class T>
T& vector<T>::at(uint32_t idx) const
{
	return this->v_array[idx];
}

template<class T>
T& vector<T>::front() const
{
	return this->v_array[0];
}

template<class T>
T& vector<T>::back() const
{
	return this->v_array[this->v_size - 1];
}

template<class T>
T* vector<T>::data() const
{
	return this->v_array;
}

template<class T>
T& vector<T>::operator[](const int idx) const
{
	return this->v_array[idx];
}

template<class T>
uint32_t vector<T>::find(const T val)
{
	for(int i = 0; i < this->v_size; i++)
	{
		if(this->v_array[i] == val)
		{
			return i;
		}
	}
	return -1;
}


#endif
