#pragma once

#include <vector>
#include <utility>

/**
 * Class mimicking Qt's QByteArray behavior using STL containers.
 */
class byte_array
{
	std::vector<uint8_t> value; ///< @todo: make implicitly shared cow?
public:
	byte_array();
	byte_array(const byte_array&);
	byte_array(const std::vector<uint8_t>&);
	byte_array(const char*, size_t);
	~byte_array();
	byte_array(byte_array&& other)
		: value(std::move(other.value))
	{}
	byte_array& operator = (const byte_array& other);
	byte_array& operator = (byte_array&& other);

	const char* data() const;
	/**
	 * @sa length(), capacity()
	 */
	const size_t size() const;
	/**
	 * @sa size(), capacity()
	 */
	const size_t length() const;

	char at(int i) const;
	char operator[](int i) const;
	char& operator[](int i);

	/**
	 * Fill entire array to char @a ch.
	 * If the size is specified, resizes the array beforehand.
	 */
	byte_array& fill(char ch, int size = -1);
};

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
