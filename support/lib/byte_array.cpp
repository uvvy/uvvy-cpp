//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "byte_array.h"
#include <iostream>
#include <iomanip>

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

char* byte_array::data()
{
	return &value[0];
}

const char* byte_array::data() const
{
	return &value[0];
}

const char* byte_array::const_data() const
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

bool operator == (const byte_array& a, const byte_array& b)
{
	return a.value == b.value;
}

////////////////////
using namespace std;
////////////////////

struct hex_output
{
	uint8_t ch;
	int width;
	bool fill;

	hex_output(uint8_t c, int w, bool f) : ch(c), width(w), fill(f) {}
};

inline std::ostream& operator<<(std::ostream& o, const hex_output& hs)
{
	return (o << setw(hs.width) << setfill(hs.fill ? '0' : ' ') << std::hex << (int)hs.ch);
}

inline hex_output hex(uint8_t c, int w = 2, bool f = true)
{
	return hex_output(c,w,f);
}

ostream& operator << (ostream& os, const byte_array& a)
{
	for (size_t s = 0; s < a.size(); ++s)
	{
		os << noshowbase << hex(a.at(s)) << ' ';
	}
	return os;
}
