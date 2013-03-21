#include <vector>
#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// from http://www.velocityreviews.com/forums/t451601-vectorstream-analogous-to-std-stringstream.html
class vectorinbuf : public std::streambuf
{
public:
	vectorinbuf(std::vector<char>& vec) {
		this->setg(&vec[0], &vec[0], &vec[0] + vec.size());
	}
};

class ivectorstream : private virtual vectorinbuf, public std::istream
{
public:
	ivectorstream(std::vector<char>& vec)
		: vectorinbuf(vec)
		, std::istream(this)
	{
	}
};

class vectoroutbuf : public std::streambuf
{
public:
	vectoroutbuf(std::vector<char>& vec)
		: m_vector(vec)
	{
	}
	int overflow(int c)
	{
		if (c != std::char_traits<char>::eof())
			m_vector.push_back(c);
		return std::char_traits<char>::not_eof(c);
	}

private:
	std::vector<char> m_vector;
};

class ovectorstream : private virtual vectoroutbuf, public std::ostream
{
public:
	ovectorstream(std::vector<char>& vec)
		: vectoroutbuf(vec)
		, std::ostream(this)
	{
	}
};


int main()
{
    bool t = true;
    uint8_t c = 'z';
    uint16_t s = 0xbabe;
    uint32_t i = 0xabbadead;
    uint64_t l = 0xcafecafecafecafe;

	std::ofstream os("testarchive.bin", std::ios_base::binary| std::ios_base::out);
    {
        boost::archive::binary_oarchive oa(os, boost::archive::no_header);
        oa & t & c & s & i & l;
    }
    std::vector<char> data = { 0x01, 0x7A, 0xbe, 0xba, 0xad, 0xde, 0xba, 0xab, 0xfe, 0xca, 0xfe, 0xca, 0xfe, 0xca, 0xfe, 0xca};
	ivectorstream iv(data);
	{
        boost::archive::binary_iarchive ia(iv, boost::archive::no_header);
        bool t1;
        uint8_t c1;
        uint16_t s1;
        uint32_t i1;
        uint64_t l1;
        ia & t1 & c1 & s1 & i1 & l1;
        assert(t == t1);
        assert(c == c1);
        assert(s == s1);
        assert(i == i1);
        assert(l == l1);
	}
}
