#pragma once

#include <vector>
#include <utility>

/**
 * Class mimicking Qt's QByteArray behavior using STL containers.
 */
class byte_array
{
	friend bool operator ==(const byte_array& a, const byte_array& b);
	std::vector<char> value; // XXX make implicitly shared cow?
public:
	byte_array();
	byte_array(const byte_array&);
	byte_array(const std::vector<char>&);
	byte_array(const char* str);
	byte_array(const char* data, size_t size);
	~byte_array();
	byte_array& operator = (const byte_array& other);
	byte_array& operator = (byte_array&& other);

	bool is_empty() const { return size() == 0; }

	char* data();
	const char* data() const;
	const char* const_data() const;
	/**
	 * @sa length(), capacity()
	 */
	const size_t size() const;
	/**
	 * @sa size(), capacity()
	 */
	inline const size_t length() const {
		return size();
	}

	char at(int i) const;
	char operator[](int i) const;
	char& operator[](int i);

	/**
	 * Fill entire array to char @a ch.
	 * If the size is specified, resizes the array beforehand.
	 */
	byte_array& fill(char ch, int size = -1);

	/**
	 * Unlike Qt's fromRawData current implementation of
	 * wrap does not actually wrap the data, it creates
	 * its own copy. XXX fix it
	 */
	static byte_array wrap(const char* data, size_t size);
};

bool operator ==(const byte_array& a, const byte_array& b);

std::ostream& operator << (std::ostream& os, const byte_array& a);

/**
struct BigArray{
       BigArray():m_data(new int[10000000]){}
       int  operator[](size_t i)const{return (*m_data)[i];}
       int& operator[](size_t i){
          if(!m_data.unique()){//"detach"
            shared_ptr<int> _tmp(new int[10000000]);
            memcpy(_tmp.get(),m_data.get(),10000000);
            m_data=_tmp;
          }
          return (*m_data)[i];
       }
    private:
       shared_ptr<int> m_data;
    };
*/
