#include "byte_array.h"

byte_array::byte_array()
	: value()
{}

byte_array::byte_array(const byte_array& other)
	: value(other.value)
{}

byte_array::byte_array(const std::vector<char>& v)
	: value(v)
{}

byte_array::byte_array(const char* str)
	: value(str, str+strlen(str)+1)
{}

byte_array::byte_array(const char* data, size_t size)
	: value(data, data+size)
{}

byte_array::~byte_array()
{}

byte_array& byte_array::operator = (const byte_array& other)
{
	if (&other != this)
	{
		value = other.value;
	}
	return *this;
}

byte_array& byte_array::operator = (byte_array&& other)
{
	if (&other != this)
	{
		value = std::move(other.value);
	}
	return *this;
}

const char* byte_array::data() const
{
	return &value[0];
}

const size_t byte_array::size() const
{
	return value.size();
}

char byte_array::at(int i) const
{
	return value.at(i);
}

char byte_array::operator[](int i) const
{
	return value[i];
}

char& byte_array::operator[](int i)
{
	return value[i];
}

byte_array& byte_array::fill(char ch, int size)
{
	if (size != -1)
		value.resize(size);
	std::fill(value.begin(), value.end(), ch);
	return *this;
}

byte_array byte_array::wrap(const char* data, size_t size)
{
	return byte_array(data, size);
}

bool operator ==(const byte_array& a, const byte_array& b)
{
	return a.value == b.value;
}
