#pragma once

#include <boost/serialization/detail/stack_constructor.hpp>

BOOST_CLASS_IMPLEMENTATION(optional<uint32_t>, boost::serialization::object_serializable)

// function specializations must be defined in the appropriate
// namespace - boost::serialization
namespace boost {
namespace serialization {

template<class Archive, class T>
void save(
    Archive & ar, 
    const boost::optional< T > & t, 
    const unsigned int /*version*/
){
    const bool tflag = t.is_initialized();
    ar << boost::serialization::make_nvp("initialized", tflag);
    if (tflag){
        ar << boost::serialization::make_nvp("value", *t);
    }
}

template<class Archive, class T>
void load(
    Archive & ar, 
    boost::optional< T > & t, 
    const unsigned int /*version*/
){
    bool tflag;
    ar >> boost::serialization::make_nvp("initialized", tflag);
    if (tflag){
        detail::stack_construct<Archive, T> aux(ar, 0);
        ar >> boost::serialization::make_nvp("value", aux.reference());
        t.reset(aux.reference());
    }
    else {
        t.reset();
    }
}

template<class Archive, class T>
void serialize(
    Archive & ar, 
    boost::optional< T > & t, 
    const unsigned int version
){
    boost::serialization::split_free(ar, t, version);
}

} // namespace serialization
} // namespace boost
