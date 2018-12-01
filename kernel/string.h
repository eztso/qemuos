#ifndef STRING_H
#define STRING_H

typedef unsigned long uint32_t;
class string
{
	private:
		char* c_array;
		uint32_t s_size;
	public:
		string(const char* str);
		string(string&& s);
		string(string& s);
		string();
		~string();
		string substr(uint32_t start, uint32_t len);
		char& operator[](const uint32_t idx)const;
		string& operator+=(const string& str);
		string& operator= (const string& str);
		bool operator== ( const string& rhs);
		char* c_str();
		uint32_t size();
		uint32_t length();

};
string::string(string& s)
{
	this->c_array = new char[s.s_size + 1];
	for(uint32_t i = 0; i < s.s_size; i++)
	{
		this->c_array[i] = s.c_array[i];
	}
	this->c_array[s.s_size] = '\0';
	this->s_size =s.s_size;
}
string::string(string&& s)
{
	this->c_array = s.c_array;
	s.c_array = nullptr;
	this->s_size = s.s_size;
	s.s_size = 0;
}
string::string()
{
	this->s_size = 0;
	this->c_array = new char[1];
}
string::string(const char* str)
{	
	uint32_t len = 0;
	for(;str[len]!='\0'; len++){}
	this->c_array = new char[len + 1];
	for(uint32_t idx = 0; idx < len; idx++)
	{
		this->c_array[idx] = str[idx];
	}
	this->c_array[len] = '\0';
	this->s_size = len;
	
}
string::~string()
{
	delete[] c_array;
}
char* string::c_str()
{
	return c_array;
}

uint32_t string::size()
{
	return this-> s_size;
}

uint32_t string::length()
{
	return this-> s_size;
}

string string::substr(uint32_t start, uint32_t len)
{
	char* tmp = new char[len + 1];
	for(uint32_t i = 0; i < len; i++)
	{
		tmp[i] = this->c_array[i + start];
	}
	tmp[len] = '\0';
	return string(tmp);
}

char& string::operator[](const uint32_t idx)const
{
	return this->c_array[idx];
}

bool string::operator== ( const string& rhs)
{
	if(s_size != rhs.s_size) return false;
	for(uint32_t i = 0; i < s_size; i++)
	{
		if(rhs[i]!=c_array[i]) return false;
	}
	return true;
}

string& string::operator+=(const string& str)
{
	uint32_t len = 0;
	for(;str[len]!='\0'; len++){}	
	char* tmp = this->c_array;
	this->c_array = new char[this->s_size + len + 1];
	for(uint32_t idx = 0; idx < this->s_size; idx++)
	{
		this->c_array[idx] = tmp[idx];
	}
	for(uint32_t idx = this->s_size; idx < this->s_size + len; idx ++)
	{
		this->c_array[idx] = str[idx - this->s_size];

	}
	this->s_size += len;
	this->c_array[this->s_size] = '\0';
	return *this;
}

string& string::operator= (const string& str)
{

	uint32_t len = 0;
	for(;str[len]!='\0'; len++){}

	char* tmp = new char[len + 1];

	for(uint32_t idx = 0; idx < len; idx++)
	{
		tmp[idx] = str[idx];
	}
	tmp[len] = '\0';
	this->s_size = len;
	delete[] this->c_array;
	this->c_array = tmp;
	return *this;
}


#endif