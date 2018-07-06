/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file halholder.cpp
 *
 * @brief
 *   Implementation of Hal Holder C++ API.
 *
 *****************************************************************************/
#include "cam_api/impexinfo.h"

#include <sstream>
#include <fstream>

/******************************************************************************
 * local macro definitions
 *****************************************************************************/


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


template<class T>
class TagValue
    : public Tag
{
public:
    typedef T           value_type;
    typedef T&          reference_type;
    typedef const T&    const_reference_type;

public:
    static Tag::Type    type();
    static Tag*         create( const std::string& id , const_reference_type value )
    { return new TagValue<T>( id, value ); }

public:
    TagValue( const std::string& id, const_reference_type value )
        : Tag ( TagValue<T>::type(), id ), m_value ( value )
    { }

public:
    virtual std::string  toString() const
    { std::ostringstream oss; oss << m_value; return oss.str(); }
    virtual void         fromString( const std::string& str )
    { std::stringstream iss; iss << str; iss >> m_value; }

            value_type   get() const { return m_value; }
            void         get( reference_type value ) const { value = m_value; }
            void         set( const_reference_type value ) { m_value = value; }

private:
    value_type    m_value;
};


// explicit template instantiation
template bool Tag::getValue<bool>( bool& ) const;
template bool Tag::getValue<int>( int& ) const;
template bool Tag::getValue<uint32_t>( uint32_t& ) const;
template bool Tag::getValue<float>( float& ) const;
template bool Tag::getValue<std::string>( std::string& ) const;

template bool Tag::setValue<bool>( const bool& );
template bool Tag::setValue<int>( const int& );
template bool Tag::setValue<uint32_t>( const uint32_t& );
template bool Tag::setValue<float>( const float& );
template bool Tag::setValue<std::string>( const std::string& );

template void TagMap::insert<bool>( const bool&, const std::string&, const std::string& );
template void TagMap::insert<int>( const int&, const std::string&, const std::string& );
template void TagMap::insert<uint32_t>( const uint32_t&, const std::string&, const std::string& );
template void TagMap::insert<float>( const float&, const std::string&, const std::string& );
template void TagMap::insert<std::string>( const std::string&, const std::string&, const std::string& );

// template specialization
template<> Tag::Type TagValue<bool>::type() { return Tag::TYPE_BOOL; }
template<> Tag::Type TagValue<int>::type() { return Tag::TYPE_INT; }
template<> Tag::Type TagValue<uint32_t>::type() { return Tag::TYPE_UINT32; }
template<> Tag::Type TagValue<float>::type() { return Tag::TYPE_FLOAT; }
template<> Tag::Type TagValue<std::string>::type() { return Tag::TYPE_STRING; }


template<>
std::string
TagValue<uint32_t>::toString() const
{
    std::ostringstream oss;
    oss << "0x" << std::hex;
    oss << m_value;
    return oss.str();
}

/*
template<>
std::string
TagValue<bool>::toString() const
{
    return m_value ? "1" : "0";
}


template<>
void
TagValue<bool>::fromString( const std::string& str )
{
    m_value = ( 0 == str.compare( 1, 1, "0" ) ) ? false : true;
}
*/


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


template<class T>
bool
Tag::getValue( T& value ) const
{
    if ( TagValue<T>::type() == type() )
    {
        static_cast<const TagValue<T>* >(this)->get( value );
        return true;
    }

    return false;
}


template<class T>
bool
Tag::setValue( const T& value )
{
    if ( TagValue<T>::type() == type() )
    {
        static_cast<TagValue<T>*>(this)->set( value );
        return true;
    }

    return false;
}


TagMap::TagMap()
{ }


TagMap::~TagMap()
{
    clear();
}


void
TagMap::clear()
{
    for ( category_iterator ci = m_data.begin();
            ci != m_data.end(); ++ci )
    {
        for ( tag_iterator ti = ci->second.begin();
                ti != ci->second.end(); ++ti )
        {
            delete *ti;
        }
        ci->second.clear();
    }
    m_data.clear();
}


bool
TagMap::containes( const std::string& id, const std::string& category ) const
{
    Tag *t = tag( id, category );
    if ( NULL != t)
    {
        return true;
    }

    return false;
}


template<class T>
void
TagMap::insert( const T& value, const std::string& id, const std::string& category )
{
    Tag *t = tag( id, category );
    if ( NULL != t )
    {
        delete t;
        t = TagValue<T>::create ( id, value );
        return;
    }

    m_data[category].push_back( TagValue<T>::create ( id, value ) );
}


void
TagMap::remove( const std::string& id, const std::string& category )
{
    category_iterator ci = m_data.find( category );
    if ( ci != m_data.end() )
    {
        for ( tag_iterator ti = ci->second.begin();
                ti != ci->second.end(); ++ti )
        {
            if ( (*ti)->id() == id )
            {
                delete *ti;
                ci->second.erase( ti );
            }
        }

        if ( ci->second.empty() )
        {
            m_data.erase( ci );
        }
    }
}


Tag *
TagMap::tag( const std::string& id, const std::string& category ) const
{
    const_category_iterator ci = m_data.find( category );
    if ( ci != m_data.end() )
    {
        for ( const_tag_iterator ti = ci->second.begin();
                ti != ci->second.end(); ++ti )
        {
            if ( (*ti)->id() == id )
            {
                return *ti;
            }
        }
    }

    return NULL;
}


TagMap::const_category_iterator
TagMap::begin() const
{
    return m_data.begin();
}


TagMap::const_category_iterator
TagMap::end() const
{
    return m_data.end();
}


TagMap::const_tag_iterator
TagMap::begin( const_category_iterator iter ) const
{
    return iter->second.begin();
}


TagMap::const_tag_iterator
TagMap::end( const_category_iterator iter ) const
{
    return iter->second.end();
}


/******************************************************************************
 * ImageExportInfo
 *****************************************************************************/
ImageExportInfo::ImageExportInfo( const char *fileName )
    : m_fileName ( fileName )
{ }


/******************************************************************************
 * ~ImageExportInfo
 *****************************************************************************/
ImageExportInfo::~ImageExportInfo()
{ }


/******************************************************************************
 * fileName
 *****************************************************************************/
const char *
ImageExportInfo::fileName() const
{
    return m_fileName.c_str();
}


/******************************************************************************
 * write
 *****************************************************************************/
void
ImageExportInfo::write() const
{
    std::string fileName( m_fileName );
    size_t founddot = fileName.rfind('.');
    size_t foundslash = fileName.rfind('/');
    if ( ( (founddot + 1) != std::string::npos ) && ( founddot > foundslash ) )
    {
        fileName.replace( founddot + 1, 3, "cfg");
    }
    else
    {
        fileName.append( ".cfg" );
    }

    std::ofstream file;
    file.open( fileName.c_str(), std::ios_base::out | std::ios_base::trunc );

    for ( TagMap::const_category_iterator ci = TagMap::begin(); ci != TagMap::end(); ++ci )
    {
        file << "[" << ci->first << "]" << std::endl;
        for ( TagMap::const_tag_iterator ti = TagMap::begin( ci ); ti != TagMap::end( ci ); ++ti )
        {
            file << (*ti)->id() << "=" << (*ti)->toString() << std::endl;
        }
        file << std::endl;
    }

    file.close();
}


