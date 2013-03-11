#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test_byte_array
#include <boost/test/unit_test.hpp>

#include "byte_array.h"

// byte_array();
BOOST_AUTO_TEST_CASE(default_allocation)
{
    byte_array b;
    BOOST_CHECK(b.size() == 0);
    BOOST_CHECK(b.is_empty());
    BOOST_CHECK_THROW(b.at(0), std::out_of_range);
}

// byte_array(const byte_array&);
BOOST_AUTO_TEST_CASE(copy_ctor)
{
    byte_array first("test");
    byte_array second(first);
    BOOST_CHECK(first == second);
    BOOST_CHECK(first[0] == second[0]);
    BOOST_CHECK(second[0] == 't');
}

// byte_array(const std::vector<char>&);
BOOST_AUTO_TEST_CASE(ctor_from_vector)
{
    static const char* init = "test";
    std::vector<char> vect(init, init+4);
    byte_array b(vect);
    BOOST_CHECK(!b.is_empty());
    BOOST_CHECK(b[0] == 't');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 's');
    BOOST_CHECK(b[3] == 't');
    BOOST_CHECK(b.size() == 4);
}

// byte_array(const char* str);
BOOST_AUTO_TEST_CASE(ctor_from_cstring)
{
    byte_array b("hello");
    BOOST_CHECK(!b.is_empty());
    BOOST_CHECK(b[0] == 'h');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 'l');
    BOOST_CHECK(b[3] == 'l');
    BOOST_CHECK(b[4] == 'o');
    BOOST_CHECK(b[5] == 0);
    BOOST_CHECK(b.size() == 6);
}

// byte_array(const char* data, size_t size);
BOOST_AUTO_TEST_CASE(ctor_from_buffer)
{
    byte_array b("hello", 5);
    BOOST_CHECK(!b.is_empty());
    BOOST_CHECK(b[0] == 'h');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 'l');
    BOOST_CHECK(b[3] == 'l');
    BOOST_CHECK(b[4] == 'o');
    BOOST_CHECK(b.size() == 5);
}

// byte_array(byte_array&& other);
BOOST_AUTO_TEST_CASE(move_ctor)
{
    byte_array b(std::move(byte_array("hello")));
    BOOST_CHECK(!b.is_empty());
    BOOST_CHECK(b[0] == 'h');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 'l');
    BOOST_CHECK(b[3] == 'l');
    BOOST_CHECK(b[4] == 'o');
    BOOST_CHECK(b[5] == 0);
    BOOST_CHECK(b.size() == 6);
}

// byte_array& operator = (const byte_array& other);
BOOST_AUTO_TEST_CASE(assign)
{
    byte_array other("hello");
    byte_array b;
    b = other;
    BOOST_CHECK(!b.is_empty());
    BOOST_CHECK(b.at(0) == 'h');
    BOOST_CHECK(b.at(1) == 'e');
    BOOST_CHECK(b.at(2) == 'l');
    BOOST_CHECK(b.at(3) == 'l');
    BOOST_CHECK(b.at(4) == 'o');
    BOOST_CHECK(b.at(5) == 0);
    BOOST_CHECK(b.size() == 6);
}

// byte_array& operator = (const byte_array& other);
BOOST_AUTO_TEST_CASE(self_assign)
{
    byte_array b("hello");
    b = b;
    BOOST_CHECK(!b.is_empty());
    BOOST_CHECK(b[0] == 'h');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 'l');
    BOOST_CHECK(b[3] == 'l');
    BOOST_CHECK(b[4] == 'o');
    BOOST_CHECK(b[5] == 0);
    BOOST_CHECK(b.size() == 6);
}

// byte_array& operator = (byte_array&& other);
BOOST_AUTO_TEST_CASE(move)
{
    byte_array b;
    b = std::move(byte_array("hello"));
    BOOST_CHECK(!b.is_empty());
    BOOST_CHECK(b[0] == 'h');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 'l');
    BOOST_CHECK(b[3] == 'l');
    BOOST_CHECK(b[4] == 'o');
    BOOST_CHECK(b[5] == 0);
    BOOST_CHECK(b.size() == 6);
}

// byte_array& operator = (byte_array&& other);
BOOST_AUTO_TEST_CASE(self_move)
{
    byte_array b("hello");
    b = std::move(b);
    BOOST_CHECK(!b.is_empty());
    BOOST_CHECK(b[0] == 'h');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 'l');
    BOOST_CHECK(b[3] == 'l');
    BOOST_CHECK(b[4] == 'o');
    BOOST_CHECK(b[5] == 0);
    BOOST_CHECK(b.size() == 6);
}

// char* data();
// const char* data() const;
// const char* const_data() const;
// const size_t size() const;
// inline const size_t length() const;
BOOST_AUTO_TEST_CASE(data_and_size)
{
    byte_array b("hello");
    // const
    BOOST_CHECK(b.data()[0] == 'h');
    BOOST_CHECK(b.data()[1] == 'e');
    BOOST_CHECK(b.const_data()[2] == 'l');
    BOOST_CHECK(b.const_data()[3] == 'l');
    BOOST_CHECK(b.data()[4] == 'o');
    BOOST_CHECK(b.data()[5] == 0);
    BOOST_CHECK(b.size() == 6);
    BOOST_CHECK(b.length() == 6);
    BOOST_CHECK(b.size() == b.length());
    // non-const
    b.data()[0] = 'm';
    b.data()[2] = b.data()[3] = 'z';
    byte_array other("mezzo");
    BOOST_CHECK(b == other);
}

// char at(int i) const;
// char operator[](int i) const;
// char& operator[](int i);
BOOST_AUTO_TEST_CASE(accessing_bytes)
{
    byte_array b("hello");
    // const
    BOOST_CHECK(b[0] == 'h');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 'l');
    BOOST_CHECK(b[3] == 'l');

    BOOST_CHECK(b[0] == b.at(0));
    BOOST_CHECK(b[1] == b.at(1));
    BOOST_CHECK(b[2] == b.at(2));
    BOOST_CHECK(b[3] == b.at(3));

    BOOST_CHECK(b.at(0) == 'h');
    BOOST_CHECK(b.at(1) == 'e');
    BOOST_CHECK(b.at(2) == 'l');
    BOOST_CHECK(b.at(3) == 'l');
    BOOST_CHECK(b.at(4) == 'o');
    BOOST_CHECK(b.at(5) == 0);
    BOOST_CHECK_THROW(b.at(6), std::out_of_range);
    // non-const
    b[0] = 'j';
    byte_array other("jello");
    BOOST_CHECK(b == other);
}

// byte_array& fill(char ch, int size = -1);
BOOST_AUTO_TEST_CASE(fill)
{
    byte_array b("hello");
    b.fill('z');
    BOOST_CHECK(b.at(0) == 'z');
    BOOST_CHECK(b.at(1) == 'z');
    BOOST_CHECK(b.at(2) == 'z');
    BOOST_CHECK(b.at(3) == 'z');
    BOOST_CHECK(b.at(4) == 'z');
    BOOST_CHECK(b.at(5) == 'z');
}

// byte_array& fill(char ch, int size = -1);
BOOST_AUTO_TEST_CASE(fill_and_resize)
{
    byte_array b("hello");
    b.fill('z', 3);
    BOOST_CHECK(b.at(0) == 'z');
    BOOST_CHECK(b.at(1) == 'z');
    BOOST_CHECK(b.at(2) == 'z');
    BOOST_CHECK(b.size() == 3);
    BOOST_CHECK_THROW(b.at(3), std::out_of_range);
}

// static byte_array wrap(const char* data, size_t size);
BOOST_AUTO_TEST_CASE(wrap)
{
    byte_array b = byte_array::wrap("hello", 5);
    BOOST_CHECK(b[0] == 'h');
    BOOST_CHECK(b[1] == 'e');
    BOOST_CHECK(b[2] == 'l');
    BOOST_CHECK(b[3] == 'l');
    BOOST_CHECK(b[4] == 'o');
    BOOST_CHECK(b.size() == 5);
}

// bool operator ==(const byte_array& a, const byte_array& b);
BOOST_AUTO_TEST_CASE(equals_comparison)
{
    byte_array hello("hello");
    byte_array other("hello");
    BOOST_CHECK(hello == other);
}
