/******************************************************************************
 *
 * Copyright 2011, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file    calibdb.cpp
 *
 *****************************************************************************/
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <common/return_codes.h>
#include <common/cam_types.h>

#include <cam_calibdb/cam_calibdb_api.h>
#include <string>
#include <iostream>

#include "calib_xml/calibdb.h"
#include "calibtags.h"
#include "xmltags.h"
#include <utils/Log.h>

CREATE_TRACER(CALIBDB_INFO , "CALIBDB: ", INFO,    1);
CREATE_TRACER(CALIBDB_WARN , "CALIBDB: ", WARNING, 1);
CREATE_TRACER(CALIBDB_ERROR, "CALIBDB: ", ERROR,   1);
CREATE_TRACER(CALIBDB_NOTICE0, "", TRACE_NOTICE0,    1);
CREATE_TRACER(CALIBDB_NOTICE1, "CALIBDB: ", TRACE_NOTICE1,    1);
CREATE_TRACER(CALIBDB_DEBUG, "CALIBDB: ", TRACE_DEBUG,    1);

/******************************************************************************
 * Toupper
 *****************************************************************************/
char* Toupper(const char *s)
{
    int i=0;
    char * p= (char *)s;
    char tmp;
    int len = 0;

    if(p){
        len = strlen(p);
        for(i=0;i<len;i++){
            tmp = p[i];
            
            if(tmp>='a' && tmp<='z')
                tmp = tmp-32;
            
            p[i]=tmp;
        }
    }
    return p;
}

/******************************************************************************
 * ParseFloatArray
 *****************************************************************************/
static int ParseFloatArray
(
    const char  *c_string,          /**< trimmed c string */
    float       *values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
)
{
    float *value = values;
    char *str    = (char *)c_string;
    int last = strlen( str );
    /* calc. end adress of string */
    char *str_last = str + (last-1);

    std::string s_string(str);
    size_t find_start = s_string.find("[", 0);
    size_t find_end = s_string.find("]", 0);
    
    if((find_start==std::string::npos) || (find_end==std::string::npos))
    {
        TRACE( CALIBDB_ERROR, "%s(%d): start=%d,end=%d\n", __FUNCTION__,__LINE__,find_start,find_end);
        return -1;
    }

    str = (char *)c_string + find_start;
    str_last = (char *)c_string + find_end;
    
    #if 0
    while ( *str == 0x20 )
    {
        str++;
    }

    while ( *str_last == 0x20 )
    {
        str_last--;
    }
    
    /* check for beginning/closing parenthesis */
    if ( (str[0]!='[') || (str_last[0]!=']') )
    {
        std::cout << __func__ <<std::endl;
        std::cout << "str[0]:(" << str[0] << ")" << std::endl;
        std::cout << "str_last[0]:(" << str_last[0] << ")" << std::endl;
        std::cout << "strings:(" << str << ")" << std::endl;
        return ( -1 );
    }
    #endif
    
    /* skipped left parenthesis */
    str++;

    /* skip spaces */
    while ( *str == 0x20 || *str==0x09)
    {
        str++;
    }

    int cnt = 0;
    int scanned;
    float f;

    /* parse the c-string */
    while ( (str != str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%f", &f);
        if ( scanned != 1 )
        {
            TRACE( CALIBDB_INFO, "%s(%d): %f err\n", __FUNCTION__,__LINE__,f);
            goto err1;
        }
        else
        {
            value[cnt] = f;
            cnt++;
        }

        /* remove detected float */
        while ( (*str != 0x20)  && (*str != ',') && (*str != ']') )
        {
            str++;
        }

        /* skip spaces and comma */
        while ( (*str == 0x20) || (*str == ',')|| (*str==0x09) )
        {
            str++;
        }
    }

    return ( cnt );

err1:
    MEMSET( values, 0, (sizeof(float) * num) );

    return ( 0 );

}


/******************************************************************************
 * ParseUchartArray//cxf
 *****************************************************************************/
static int ParseUcharArray
(
    const char  *c_string,          /**< trimmed c string */
    unsigned char      *values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
)
{
    unsigned char  *value  = values;
    char *str       = (char *)c_string;

    int last = strlen( str );
    char *str_last = str + (last-1);

    std::string s_string(str);
    size_t find_start = s_string.find("[", 0);
    size_t find_end = s_string.find("]", 0);
    
    if((find_start==std::string::npos) || (find_end==std::string::npos))
    {
        std::cout << __func__ <<"start"<<find_start<< "end"<< find_end<<std::endl;
        TRACE( CALIBDB_ERROR, "%s(%d):start:%d,end:%d\n", __FUNCTION__,__LINE__,find_start,find_end);
        return -1;
    }

    str = (char *)c_string + find_start;
    str_last = (char *)c_string + find_end;
    
    #if 0
    while ( *str == 0x20 )
    {
        str++;
    }

    while ( *str_last == 0x20 )
    {
        str_last--;
    }
    
    /* check for beginning/closing parenthesis */
    if ( (str[0]!='[') || (str_last[0]!=']') )
    {
        std::cout << __func__ <<std::endl;
        std::cout << "str[0]:(" << str[0] << ")" << std::endl;
        std::cout << "str_last[0]:(" << str_last[0] << ")" << std::endl;
        std::cout << "strings:(" << str << ")" << std::endl;
        return ( -1 );
    }
    #endif
    
    /* calc. end adress of string */
   // char *str_last = str + (last-1);

    /* skipped left parenthesis */
    str++;

    /* skip spaces */
    while ( *str == 0x20 || *str==0x09 || (*str==0x0a) || (*str==0x0d))
    {
        str++;
    }

    int cnt = 0;
    int scanned;
    uint8_t f;

    /* parse the c-string */
    while ( (str != str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%hhu", &f);
        if ( scanned != 1 )
        {
            std::cout << __func__ << "f" << f << "err" << std::endl;
            TRACE( CALIBDB_ERROR, "%s(%d):f:%f\n", __FUNCTION__,__LINE__,f);
            goto err1;
        }
        else
        {
            value[cnt] = f;
            cnt++;
        }

        /* remove detected float */
        while ( (*str != 0x20)  && (*str != ',') && (*str != ']') )
        {
            str++;
        }

        /* skip spaces and comma */
        while ( (*str == 0x20) || (*str == ',') || (*str==0x09) || (*str==0x0a) || (*str==0x0d))
        {
            str++;
        }
    }

    std::cout << std::endl;
    std::cout << std::endl;
    return ( cnt );

err1:
    MEMSET( values, 0, (sizeof(uint8_t) * num) );

    return ( 0 );

}




/******************************************************************************
 * ParseUshortArray
 *****************************************************************************/
static int ParseUshortArray
(
    const char  *c_string,          /**< trimmed c string */
    uint16_t    *values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
)
{
    uint16_t *value = values;
    char *str       = (char *)c_string;
    int last = strlen( str );
    char *str_last = str + (last-1);

    std::string s_string(str);
    size_t find_start = s_string.find("[", 0);
    size_t find_end = s_string.find("]", 0);
    
    if((find_start==std::string::npos) || (find_end==std::string::npos))
    {
        std::cout << __func__ <<"start"<<find_start<< "end"<< find_end<<std::endl;
        TRACE( CALIBDB_ERROR, "%s(%d): parse error!\n", __FUNCTION__,__LINE__);
        return -1;
    }

    while(*str!='[')
    {
        printf("'%c'=%d\n", *str, *str);
        str++;
    }
    str = (char *)c_string + find_start;
    str_last = (char *)c_string + find_end;
    
    #if 0
    while ( *str == 0x20 )
    {
        str++;
    }

    while ( *str_last == 0x20 )
    {
        str_last--;
    }
    
    /* check for beginning/closing parenthesis */
    if ( (str[0]!='[') || (str_last[0]!=']') )
    {
        std::cout << __func__ <<std::endl;
        std::cout << "str[0]:(" << str[0] << ")" << std::endl;
        std::cout << "str_last[0]:(" << str_last[0] << ")" << std::endl;
        std::cout << "strings:(" << str << ")" << std::endl;
        return ( -1 );
    }
    #endif
    
    /* calc. end adress of string */
    //char *str_last = str + (last-1);

    /* skipped left parenthesis */
    str++;

    /* skip spaces */
    while ( *str == 0x20 || *str==0x09)
    {
        str++;
    }

    int cnt = 0;
    int scanned;
    uint16_t f;

    /* parse the c-string */
    while ( (str != str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%hu", &f);
        if ( scanned != 1 )
        {
            std::cout << __func__ << "f" << f << "err" << std::endl;
            TRACE( CALIBDB_ERROR, "%s(%d): parse error!\n", __FUNCTION__,__LINE__);
            goto err1;
        }
        else
        {
            value[cnt] = f;
            cnt++;
        }

        /* remove detected float */
        while ( (*str != 0x20)  && (*str != ',') && (*str != ']') )
        {
            str++;
        }

        /* skip spaces and comma */
        while ( (*str == 0x20) || (*str == ',') || (*str==0x09) )
        {
            str++;
        }
    }

    std::cout << std::endl;
    std::cout << std::endl;
    return ( cnt );

err1:
    MEMSET( values, 0, (sizeof(uint16_t) * num) );

    return ( 0 );

}


/******************************************************************************
 * ParseShortArray
 *****************************************************************************/
static int ParseShortArray
(
    const char  *c_string,          /**< trimmed c string */
    int16_t     *values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
)
{
    int16_t *value  = values;
    char *str       = (char *)c_string;

    int last = strlen( str );
    char *str_last = str + (last-1);

    std::string s_string(str);
    size_t find_start = s_string.find("[", 0);
    size_t find_end = s_string.find("]", 0);
    
    if((find_start==std::string::npos) || (find_end==std::string::npos))
    {
        TRACE( CALIBDB_ERROR, "%s(%d): parse error!\n", __FUNCTION__,__LINE__);
        return -1;
    }

    str = (char *)c_string + find_start;
    str_last = (char *)c_string + find_end;
    
    #if 0
    while ( *str == 0x20 )
    {
        str++;
    }

    while ( *str_last == 0x20 )
    {
        str_last--;
    }
    
    /* check for beginning/closing parenthesis */
    if ( (str[0]!='[') || (str_last[0]!=']') )
    {
        std::cout << __func__ <<std::endl;
        std::cout << "str[0]:(" << str[0] << ")" << std::endl;
        std::cout << "str_last[0]:(" << str_last[0] << ")" << std::endl;
        std::cout << "strings:(" << str << ")" << std::endl;
        return ( -1 );
    }
    #endif
    
    /* calc. end adress of string */
   // char *str_last = str + (last-1);

    /* skipped left parenthesis */
    str++;

    /* skip spaces */
    while ( *str == 0x20 || *str==0x09)
    {
        str++;
    }

    int cnt = 0;
    int scanned;
    int16_t f;

    /* parse the c-string */
    while ( (str != str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%hd", &f);
        if ( scanned != 1 )
        {
            TRACE( CALIBDB_ERROR, "%s(%d): parse error!\n", __FUNCTION__,__LINE__);
            goto err1;
        }
        else
        {
            value[cnt] = f;
            cnt++;
        }

        /* remove detected float */
        while ( (*str != 0x20)  && (*str != ',') && (*str != ']') )
        {
            str++;
        }

        /* skip spaces and comma */
        while ( (*str == 0x20) || (*str == ',') || (*str==0x09) )
        {
            str++;
        }
    }

    return ( cnt );

err1:
    MEMSET( values, 0, (sizeof(uint16_t) * num) );

    return ( 0 );

}



/******************************************************************************
 * ParseByteArray
 *****************************************************************************/
static int ParseByteArray
(
    const char  *c_string,          /**< trimmed c string */
    uint8_t     *values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
)
{
    uint8_t *value  = values;
    char *str       = (char *)c_string;
    int last = strlen( str );
    char *str_last = str + (last-1);

    std::string s_string(str);
    size_t find_start = s_string.find("[", 0);
    size_t find_end = s_string.find("]", 0);
    
    if((find_start==std::string::npos) || (find_end==std::string::npos))
    {
        TRACE( CALIBDB_ERROR, "%s(%d): parse error!\n", __FUNCTION__,__LINE__);
        return -1;
    }

    str = (char *)c_string + find_start;
    str_last = (char *)c_string + find_end;
    
    #if 0
    /* check for beginning/closing parenthesis */
    if ( (str[0]!='[') || (str_last[0]!=']') )
    {
        std::cout << __func__ <<std::endl;
        std::cout << "str[0]:(" << str[0] << ")" << std::endl;
        std::cout << "str_last[0]:(" << str_last[0] << ")" << std::endl;
        std::cout << "strings:(" << str << ")" << std::endl;
        return ( -1 );
    }
    #endif

    /* calc. end adress of string */
    //char *str_last = str + (last-1);

    /* skipped left parenthesis */
    str++;

    /* skip spaces */
    while ( *str == 0x20 || *str==0x09)
    {
        str++;
    }

    int cnt = 0;
    int scanned;
    uint16_t f;

    /* parse the c-string */
    while ( (str != str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%hu", &f);
        if ( scanned != 1 )
        {
            TRACE( CALIBDB_ERROR, "%s(%d): parse error!\n", __FUNCTION__,__LINE__);
            goto err1;
        }
        else
        {
            value[cnt] = (uint8_t)f;
            cnt++;
        }

        /* remove detected float */
        while ( (*str != 0x20)  && (*str != ',') && (*str != ']') )
        {
            str++;
        }

        /* skip spaces and comma */
        while ( (*str == 0x20) || (*str == ',') || (*str==0x09))
        {
            str++;
        }
    }


    return ( cnt );

err1:
    MEMSET( values, 0, (sizeof(uint8_t) * num) );

    return ( 0 );

}


/******************************************************************************
 * ParseCcProfileArray
 *****************************************************************************/
static int ParseCcProfileArray
(
    const char          *c_string,          /**< trimmed c string */
    CamCcProfileName_t  values[],           /**< pointer to memory */
    const int           num                 /**< number of expected float values */
)
{
    char *str = (char *)c_string;

    int last = strlen( str );

    /* calc. end adress of string */
    char *str_last = str + (last-1);

    /* skip beginning spaces */
    while ( *str == 0x20 || *str==0x09)
    {
        str++;
    }

    /* skip ending spaces */
    while ( *str_last == 0x20 || *str_last==0x09)
    {
        str_last--;
    }

    int cnt = 0;
    int scanned;
    CamCcProfileName_t f;
    memset( f, 0, sizeof(f) );

    /* parse the c-string */
    while ( (str != str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%s", f);
        if ( scanned != 1 )
        {
            TRACE( CALIBDB_ERROR, "%s(%d): parse error!\n", __FUNCTION__,__LINE__);
            goto err1;
        }
        else
        {
            strncpy( values[cnt], f, strlen( f ) );
            cnt++;
        }

        /* remove detected string */
        while ( (*str != 0x20)  && (*str != ',') && (*str != ']') && (str!=str_last) )
        {
            str++;
        }

        if ( str != str_last )
        {
            /* skip spaces and comma */
            while ( (*str == 0x20) || (*str == ',') )
            {
                str++;
            }
        }

        memset( f, 0, sizeof(f) );
    }

    return ( cnt );

err1:
    memset( values, 0, (sizeof(uint16_t) * num) );

    return ( 0 );
}



/******************************************************************************
 * ParseLscProfileArray
 *****************************************************************************/
static int ParseLscProfileArray
(
    const char          *c_string,          /**< trimmed c string */
    CamLscProfileName_t values[],           /**< pointer to memory */
    const int           num                 /**< number of expected float values */
)
{
    char *str = (char *)c_string;

    int last = strlen( str );

    /* calc. end adress of string */
    char *str_last = str + (last-1);

    /* skip beginning spaces */
    while ( *str == 0x20 || *str==0x09)
    {
        str++;
    }

    /* skip ending spaces */
    while ( *str_last == 0x20 || *str_last==0x09)
    {
        str_last--;
    }

    int cnt = 0;
    int scanned;
    CamLscProfileName_t f;
    memset( f, 0, sizeof(f) );

    /* parse the c-string */
    while ( (str != str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%s", f);
        if ( scanned != 1 )
        {
            TRACE( CALIBDB_ERROR, "%s(%d): parse error!\n", __FUNCTION__,__LINE__);
            goto err1;
        }
        else
        {
            strncpy( values[cnt], f, strlen( f ) );
            cnt++;
        }

        /* remove detected string */
        while ( (*str != 0x20)  && (*str != ',') && (*str != ']') && (str!=str_last) )
        {
            str++;
        }

        if ( str != str_last )
        {
            /* skip spaces and comma */
            while ( (*str == 0x20) || (*str == ',') )
            {
                str++;
            }
        }

        memset( f, 0, sizeof(f) );
    }

    return ( cnt );

err1:
    memset( values, 0, (sizeof(uint16_t) * num) );

    return ( 0 );
}




/******************************************************************************
 * CalibDb::CalibDb
 *****************************************************************************/
CalibDb::CalibDb
(
)
{
    m_CalibDbHandle = NULL;
}



/******************************************************************************
 * CalibReader::CalibReader
 *****************************************************************************/
CalibDb::~CalibDb()
{
    if ( m_CalibDbHandle != NULL )
    {
        RESULT result = CamCalibDbRelease( &m_CalibDbHandle );
        DCT_ASSERT( result == RET_SUCCESS );
    }
}



/******************************************************************************
 * CalibDb::CalibDb
 *****************************************************************************/
bool CalibDb::CreateCalibDb
(
    const XMLElement  *root
)
{
    bool res = true;

    // create calibration-database (c-code)
    RESULT result = CamCalibDbCreate( &m_CalibDbHandle );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    // get and parse header section
    const XMLElement *header = root->FirstChildElement( CALIB_HEADER_TAG );
    if ( !header )
    {
        res = parseEntryHeader( header->ToElement(), NULL );
        if ( !res )
        {
            return ( res );
        }
    }


    // get and parse sensor section
    const XMLElement *sensor = root->FirstChildElement( CALIB_SENSOR_TAG );
    if ( !sensor )
    {
        res = parseEntrySensor( sensor->ToElement(), NULL );
        if ( !res )
        {
            return ( res );
        }
    }

    // get and parse system section
    const XMLElement *system = root->FirstChildElement( CALIB_SYSTEM_TAG );
    if ( !system )
    {
        res = parseEntrySystem( system->ToElement(), NULL );
        if ( !res )
        {
            return ( res );
        }
    }

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::readFile
 *****************************************************************************/
bool CalibDb::CreateCalibDb
(
    const char *device
)
{
    //QString errorString;
    int errorID;
    XMLDocument doc;

    bool res = true;
    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);
    
	RESULT result = CamCalibDbCreate( &m_CalibDbHandle );
    DCT_ASSERT( result == RET_SUCCESS );
    errorID = doc.LoadFile(device); 
    std::cout << __func__ << " doc.LoadFile" << "filename"<<device<< "error"<<errorID<<std::endl;
    if ( doc.Error() )
    {
        TRACE( CALIBDB_ERROR, "%s(%d): Error: Parse error errorID %d,errorstr1 %s :errstr2\n", 
        					__FUNCTION__,__LINE__,errorID,doc.GetErrorStr1(),doc.GetErrorStr2());
        return ( false);
    }
    XMLElement *proot = doc.RootElement();
    std::string tagname(proot->Name());
    if ( tagname != CALIB_FILESTART_TAG )
    {
		TRACE( CALIBDB_ERROR, "%s(%d): Error: Not a calibration data file\n");
        return ( false );
    }

    // parse header section
    XMLElement *pheader = proot->FirstChildElement( CALIB_HEADER_TAG );
    if ( pheader )
    {
        res = parseEntryHeader( pheader->ToElement(), NULL );
        if ( !res )
        {
            return ( res );
        }
    }

    // parse sensor section
    XMLElement *psensor = proot->FirstChildElement( CALIB_SENSOR_TAG );
    if ( psensor )
    {
        res = parseEntrySensor( psensor->ToElement(), NULL );
        if ( !res )
        {
            return ( res );
        }
    }

    // parse system section
    XMLElement *psystem = proot->FirstChildElement( CALIB_SYSTEM_TAG );
    if ( psystem )
    {
        res = parseEntrySystem( psystem->ToElement(), NULL );
        if ( !res )
        {
            return ( res );
        }
    }

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( res );
}




/******************************************************************************
 * CalibDb::parseEntryCell
 *****************************************************************************/
bool CalibDb::parseEntryCell
(
    const XMLElement   *pelement,
    int                 noElements,
    parseCellContent    func,
    void                *param
)
{
    int cnt = 0;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);
    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild && (cnt < noElements) )
    {
        XmlCellTag tag = XmlCellTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());
            
        if ( tagname == CALIB_CELL_TAG )
        {
            bool result = (this->*func)( pchild->ToElement(), param );
            if ( !result )
            {
                return ( result );
            }
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): unknown cell tag: %s\n", __FUNCTION__,__LINE__, tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
        cnt ++;
    }

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryHeader
 *****************************************************************************/
bool CalibDb::parseEntryHeader
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamCalibDbMetaData_t meta_data;
    MEMSET( &meta_data, 0, sizeof( meta_data ) );

    const XMLNode *pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());

        TRACE( CALIBDB_INFO, "%s(%d):tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
        if ( (tagname == CALIB_HEADER_CREATION_DATE_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            strncpy( meta_data.cdate, value, sizeof( meta_data.cdate ) );
        }
        else if ( (tagname == CALIB_HEADER_CREATOR_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            strncpy( meta_data.cname, value, sizeof( meta_data.cname ) );
        }
        else if ( (tagname == CALIB_HEADER_GENERATOR_VERSION_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            strncpy( meta_data.cversion, value, sizeof( meta_data.cversion ) );
        }
        else if ( (tagname == CALIB_HEADER_SENSOR_NAME_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            strncpy( meta_data.sname, value, sizeof( meta_data.sname ) );
        }
        else if ( (tagname == CALIB_HEADER_SAMPLE_NAME_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            strncpy( meta_data.sid, value, sizeof( meta_data.sid ) );
        }
        else if ( tagname == CALIB_HEADER_SENSOR_OTP_RG_TAG
			     && (tag.isType( XmlTag::TAG_TYPE_DOUBLE))
                 && (tag.Size() > 0) )
        {
        	int no = ParseUshortArray( value,  &meta_data.OTPInfo.nRG_Ratio_Typical, 1 );
            DCT_ASSERT( (no == 1) );           
        }
		else if ( tagname == CALIB_HEADER_SENSOR_OTP_BG_TAG
			&& (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
			&& (tag.Size() > 0) )
		{
        	int no = ParseUshortArray( value,  &meta_data.OTPInfo.nBG_Ratio_Typical, 1 );
            DCT_ASSERT( (no == 1) );
		}		
        else if ( tagname == CALIB_HEADER_RESOLUTION_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryResolution ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d):parse error in header resolution section (%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else
        {
            TRACE( CALIBDB_ERROR, "%s(%d):parse error in header section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbSetMetaData( m_CalibDbHandle, &meta_data );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryResolution
 *****************************************************************************/
bool CalibDb::parseEntryResolution
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamResolution_t resolution;
    MEMSET( &resolution, 0, sizeof( resolution ) );
    ListInit( &resolution.framerates );

    const XMLNode *pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_HEADER_RESOLUTION_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            strncpy( resolution.name, value, sizeof( resolution.name ) );
        }
        else if ( (tagname == CALIB_HEADER_RESOLUTION_WIDTH_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseUshortArray( value,  &resolution.width, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_HEADER_RESOLUTION_HEIGHT_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseUshortArray( value, &resolution.height, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_HEADER_RESOLUTION_FRATE_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CELL ))
                    && (tag.Size() > 0) )
        {
            if(!parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryFramerates, &resolution))
            {
				TRACE( CALIBDB_ERROR, "%s(%d): parse error in header resolution(unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
               
           
        }
        else if ( tagname == CALIB_HEADER_RESOLUTION_ID_TAG )
        {
            bool ok;

            resolution.id = tag.ValueToUInt( &ok );
            if ( !ok )
            {
                TRACE( CALIBDB_ERROR, "%s(%d): parse error: invalid resolution %s/%s\n", __FUNCTION__,__LINE__,tagname.c_str(),tag.Value());
                return ( false );
            }
        }
        else
        {
            TRACE( CALIBDB_ERROR, "%s(%d): unknown tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
        
    }
    
    RESULT result = CamCalibDbAddResolution( m_CalibDbHandle, &resolution );
    DCT_ASSERT( result == RET_SUCCESS );

    // free linked framerates
    List *l = ListRemoveHead( &resolution.framerates );
    while ( l )
    {
        List *tmp = ListRemoveHead( l );
        free( l );
        l = tmp;
    }

	TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);
    return ( true );
}

/******************************************************************************
 * CalibDb::parseEntryOTPInfo
 *****************************************************************************/
bool CalibDb::parseEntryOTPInfo
(
    const XMLElement   *pelement,
    void                *param
)
{
    CamCalibDbMetaData_t *pMetaData = (CamCalibDbMetaData_t *)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    const XMLNode *pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());

        TRACE( CALIBDB_INFO, "%s(%d): tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
        if ( (tagname == CALIB_HEADER_RESOLUTION_RG_TYPICAL_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_DOUBLE))
                && (tag.Size() > 0) )
        {
            int no = ParseUshortArray( value,  &pMetaData->OTPInfo.nRG_Ratio_Typical, 1 );
            DCT_ASSERT( (no == 1) );            
        }
        else if ( (tagname == CALIB_HEADER_RESOLUTION_BG_TYPICAL_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseUshortArray( value,  &pMetaData->OTPInfo.nBG_Ratio_Typical, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else
        {
            TRACE( CALIBDB_ERROR, "%s(%d):unknown tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
        
    }
    
    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}

/******************************************************************************
 * CalibDb::parseEntryDpccRegisters
 *****************************************************************************/
bool CalibDb::parseEntryFramerates
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamResolution_t *pResolution = (CamResolution_t *)param;
    CamFrameRate_t *pFrate = (CamFrameRate_t *) malloc( sizeof(CamFrameRate_t) );
    if ( !pFrate )
    {
        return false;
    }
    MEMSET( pFrate, 0, sizeof(*pFrate) );

    const XMLNode *pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());

		TRACE( CALIBDB_INFO, "%s(%d): tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());

            
        if ( (tagname == CALIB_HEADER_RESOLUTION_FRATE_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {            
            snprintf( pFrate->name, CAM_FRAMERATE_NAME, "%s_%s",
                        pResolution->name, value );
        }
        else if ( (tagname == CALIB_HEADER_RESOLUTION_FRATE_FPS_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( value, &pFrate->fps, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d):parse error in framerate section (unknow tag: %s)\n",
														__FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    if ( pResolution )
    {
        ListPrepareItem( pFrate );
        ListAddTail( &pResolution->framerates, pFrate );
    }

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}


/******************************************************************************
 * CalibDb::parseEntrySensor
 *****************************************************************************/
bool CalibDb::parseEntrySensor
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    const XMLNode *pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());
        TRACE( CALIBDB_INFO, "%s(%d): tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());

        if ( tagname == CALIB_SENSOR_AWB_TAG )
        {
            if ( !parseEntryAwb( pchild->ToElement() ) )
            {
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_LSC_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryLsc ) )
            {
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_CC_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryCc ) )
            {
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_AF_TAG )
        {
            TRACE( CALIBDB_INFO, "%s(%d): tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
        }
        else if ( tagname == CALIB_SENSOR_AEC_TAG )
        {
            TRACE( CALIBDB_INFO, "%s(%d): tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
            if ( !parseEntryAec( pchild->ToElement() ) )
            {
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_BLS_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryBls ) )
            {
                TRACE( CALIBDB_ERROR, "%s(%d):parse error in BLS section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_DEGAMMA_TAG )
        {
            TRACE( CALIBDB_INFO, "%s(%d): tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
        }
		 else if ( tagname == CALIB_SENSOR_GAMMAOUT_TAG )
        {
            TRACE( CALIBDB_INFO, "%s(%d): tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
            if ( !parseEntryGammaOut( pchild->ToElement() ) )
            {
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_WDR_TAG )
        {
            TRACE( CALIBDB_INFO, "%s(%d): tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
        }
        else if ( tagname == CALIB_SENSOR_CAC_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryCac ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d):parse error in CAC section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_DPF_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryDpf ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d):parse error in DPF section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_DPCC_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryDpcc ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d):parse error in DPCC section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d):parse error in header section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

	TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryAec
 *****************************************************************************/
bool CalibDb::parseEntryAec
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamCalibAecGlobal_t aec_data;
	MEMSET(&aec_data, 0, sizeof(aec_data));
	aec_data.MeasuringWinWidthScale = 1.0;
	aec_data.MeasuringWinHeightScale = 1.0;
    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_AEC_SETPOINT_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &aec_data.SetPoint , 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_CLM_TOLERANCE_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &aec_data.ClmTolerance, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_DAMP_OVER_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &aec_data.DampOverStill, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_DAMP_UNDER_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &aec_data.DampUnderStill, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_DAMP_OVER_VIDEO_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &aec_data.DampOverVideo, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_DAMP_UNDER_VIDEO_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &aec_data.DampUnderVideo, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_AFPS_MAX_GAIN_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &aec_data.AfpsMaxGain, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( tagname == CALIB_SENSOR_AEC_ECM_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryAecEcm ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d): parse error in AEC section (%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_AEC_GRIDWEIGHTS_TAG )//cxf
        {
            int i = ( sizeof(aec_data.GridWeights) / sizeof(aec_data.GridWeights.uCoeff[0]) );
            int no = ParseUcharArray( tag.Value(), aec_data.GridWeights.uCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if(tagname == CALIB_SENSOR_AEC_ECMDOTENABLE_TAG 
			&& (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
             && (tag.Size() > 0))
		{
			 int no = ParseFloatArray( tag.Value(), &aec_data.EcmDotEnable , 1 );
             DCT_ASSERT( (no == tag.Size()) );
			 
		}
        else if ( (tagname == CALIB_SENSOR_AEC_ECMTIMEDOT_TAG )//cxf
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0) )
        {
            int i = ( sizeof(aec_data.EcmTimeDot) / sizeof(aec_data.EcmTimeDot.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), aec_data.EcmTimeDot.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_ECMGAINDOT_TAG )//cxf
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0) )
        {
            int i = ( sizeof(aec_data.EcmGainDot) / sizeof(aec_data.EcmGainDot.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), aec_data.EcmGainDot.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if(tagname==CALIB_SENSOR_AEC_MEASURINGWINWIDTHSCALE_TAG)//cxf
		{
			int no = ParseFloatArray( tag.Value(), &aec_data.MeasuringWinWidthScale, 1 );
			DCT_ASSERT( (no == tag.Size()) );
		}
		else if(tagname==CALIB_SENSOR_AEC_MEASURINGWINHEIGHTSCALE_TAG)//cxf
		{
			int no = ParseFloatArray( tag.Value(), &aec_data.MeasuringWinHeightScale, 1 );
			DCT_ASSERT( (no == tag.Size()) );
		}
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in AEC section (unknow tag:%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddAecGlobal( m_CalibDbHandle, &aec_data );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryGammaOut
 *****************************************************************************/
bool CalibDb::parseEntryGammaOut
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamCalibGammaOut_t gamma_data;

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());
       // std::cout << "tag: " << tagname<< std::endl;

        if ( (tagname == CALIB_SENSOR_GAMMAOUT_GAMMAY_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
        
			int i = 0;
			i=( sizeof(gamma_data.Curve.GammaY) / sizeof(gamma_data.Curve.GammaY[0]) );
            int no =0;
			no=ParseUshortArray( tag.Value(), gamma_data.Curve.GammaY, i );			
            DCT_ASSERT( (no == tag.Size()) );
        }
		
		else if ( (tagname == CALIB_SENSOR_GAMMAOUT_XSCALE_TAG)//cxf
							&& (tag.isType( XmlTag::TAG_TYPE_CHAR ))
							&& (tag.Size() > 0) )								
		{
			char* value = Toupper(tag.Value());
			std::string s_value(value);

			if ( s_value == CALIB_SENSOR_GAMMAOUT_XSCALE_LOG )
			{
				gamma_data.Curve.xScale = CAM_ENGINE_GAMMAOUT_XSCALE_LOG;
			}
			else if ( s_value == CALIB_SENSOR_GAMMAOUT_XSCALE_EQU )
			{
				gamma_data.Curve.xScale = CAM_ENGINE_GAMMAOUT_XSCALE_EQU;
			}		
			else
			{
				gamma_data.Curve.xScale = CAM_ENGINE_GAMMAOUT_XSCALE_INVALID;
			   TRACE( CALIBDB_ERROR, "%s(%d): invalid GAMMAOUT XSCALE (%s)\n", __FUNCTION__,__LINE__,s_value.c_str());
			}
		}	
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in Gamma out section (unknow tag:%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddGammaOut( m_CalibDbHandle, &gamma_data );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryAecEcm
 *****************************************************************************/
bool CalibDb::parseEntryAecEcm
(
    const XMLElement *plement,
    void *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);	

    CamEcmProfile_t EcmProfile;
    MEMSET( &EcmProfile, 0, sizeof(EcmProfile) );
    ListInit( &EcmProfile.ecm_scheme );

    const XMLNode * pchild = plement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());
        
        if ( (tagname == CALIB_SENSOR_AEC_ECM_NAME_TAG )
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( EcmProfile.name, value, sizeof( EcmProfile.name ) );
        }
        else if ( tagname == CALIB_SENSOR_AEC_ECM_SCHEMES_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryAecEcmPriorityScheme, &EcmProfile ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d): parse error in ECM  section (%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in ECM section (unknow tag:%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddEcmProfile( m_CalibDbHandle, &EcmProfile );
    DCT_ASSERT( result == RET_SUCCESS );

    // free linked ecm_schemes
    List *l = ListRemoveHead( &EcmProfile.ecm_scheme );
    while ( l )
    {
        List *temp = ListRemoveHead( l );
        free( l );
        l = temp;
    }

	TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryAecEcmPriorityScheme
 *****************************************************************************/
bool CalibDb::parseEntryAecEcmPriorityScheme
(
    const XMLElement *pelement,
    void *param
)
{
    CamEcmProfile_t *pEcmProfile = (CamEcmProfile_t *)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamEcmScheme_t *pEcmScheme = (CamEcmScheme_t*) malloc( sizeof(CamEcmScheme_t) );
    if ( !pEcmScheme )
    {
        return false;
    }
    MEMSET( pEcmScheme, 0, sizeof(*pEcmScheme) );

    const XMLNode *pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname( pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_AEC_ECM_SCHEME_NAME_TAG )
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( pEcmScheme->name, value, sizeof( pEcmScheme->name ) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_ECM_SCHEME_OFFSETT0FAC_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &pEcmScheme->OffsetT0Fac, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AEC_ECM_SCHEME_SLOPEA0_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &pEcmScheme->SlopeA0, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in ECM section (%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            free(pEcmScheme);
            pEcmScheme = NULL;

            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    if ( pEcmScheme )
    {
        ListPrepareItem( pEcmScheme );
        ListAddTail( &pEcmProfile->ecm_scheme, pEcmScheme );
    }

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryAwb
 *****************************************************************************/
bool CalibDb::parseEntryAwb
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);	

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( tagname == CALIB_SENSOR_AWB_GLOBALS_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryAwbGlobals ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB globals (%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryAwbIllumination ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB illumination (%s)\n", __FUNCTION__,__LINE__,tagname.c_str());

                return ( false );
            }
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB section (unknow tag:%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}


/******************************************************************************
 * CalibDb::parseEntryAwbGlobals
 *****************************************************************************/
bool CalibDb::parseEntryAwbGlobals
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamCalibAwbGlobal_t awb_data;
	MEMSET(&awb_data, 0, sizeof(awb_data));
	awb_data.awbMeasWinWidthScale = 1.0f;
	awb_data.awbMeasWinHeightScale = 1.0f;
    /* CamAwbClipParm_t */
    float *pRg1         = NULL; int nRg1        = 0;
    float *pMaxDist1    = NULL; int nMaxDist1   = 0;
    float *pRg2         = NULL; int nRg2        = 0;
    float *pMaxDist2    = NULL; int nMaxDist2   = 0;

    /* CamAwbGlobalFadeParm_t */
    float *pGlobalFade1         = NULL; int nGlobalFade1         = 0;
    float *pGlobalGainDistance1 = NULL; int nGlobalGainDistance1 = 0;
    float *pGlobalFade2         = NULL; int nGlobalFade2         = 0;
    float *pGlobalGainDistance2 = NULL; int nGlobalGainDistance2 = 0;

    /* CamAwbFade2Parm_t */
    float *pFade                = NULL; int nFade                = 0;
    float *pCbMinRegionMax      = NULL; int nCbMinRegionMax      = 0;
    float *pCrMinRegionMax      = NULL; int nCrMinRegionMax      = 0;
    float *pMaxCSumRegionMax    = NULL; int nMaxCSumRegionMax    = 0;
    float *pCbMinRegionMin      = NULL; int nCbMinRegionMin      = 0;
    float *pCrMinRegionMin      = NULL; int nCrMinRegionMin      = 0;
    float *pMaxCSumRegionMin    = NULL; int nMaxCSumRegionMin    = 0;

	float *pMinCRegionMax 		= NULL; int nMinCRegionMax = 0;
	float *pMinCRegionMin 		= NULL; int nMinCRegionMin = 0;	
	float *pMaxYRegionMax 		= NULL; int nMaxYRegionMax = 0;
	float *pMaxYRegionMin 		= NULL; int nMaxYRegionMin = 0;
	float *pMinYMaxGRegionMax 	= NULL; int nMinYMaxGRegionMax = 0;
	float *pMinYMaxGRegionMin 	= NULL; int nMinYMaxGRegionMin = 0;
	float *pRefCr 				= NULL; int nRefCr = 0;
	float *pRefCb 				= NULL; int nRefCb = 0;

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            strncpy( awb_data.name, value, sizeof( awb_data.name ) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RESOLUTION_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            strncpy( awb_data.resolution, value, sizeof( awb_data.resolution ) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_SENSOR_FILENAME_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            // do nothing
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_SVDMEANVALUE_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )

        {
            int i = ( sizeof(awb_data.SVDMeanValue) / sizeof(awb_data.SVDMeanValue.fCoeff[0]) );
            int no = ParseFloatArray( value, awb_data.SVDMeanValue.fCoeff, i );

            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_PCAMATRIX_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(awb_data.PCAMatrix) / sizeof(awb_data.PCAMatrix.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), awb_data.PCAMatrix.fCoeff, i );

            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_CENTERLINE_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(awb_data.CenterLine) / sizeof(awb_data.CenterLine.f_N0_Rg) );
            int no = ParseFloatArray( tag.Value(), &awb_data.CenterLine.f_N0_Rg, i );

            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_KFACTOR_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(awb_data.KFactor) / sizeof(awb_data.KFactor.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), awb_data.KFactor.fCoeff, i );

            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RG1_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pRg1) )
        {
            nRg1 = tag.Size();
            pRg1 = (float *)malloc( sizeof(float) * nRg1 );

            int no = ParseFloatArray( tag.Value(), pRg1, nRg1 );
            DCT_ASSERT( (no == nRg1) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MAXDIST1_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pMaxDist1) )
        {
            nMaxDist1 = tag.Size();
            pMaxDist1 = (float *)malloc( sizeof(float) * nMaxDist1 );

            int no = ParseFloatArray( tag.Value(), pMaxDist1, nMaxDist1 );
            DCT_ASSERT( (no == nRg1) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RG2_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pRg2) )
        {
            nRg2 = tag.Size();
            pRg2 = (float *)malloc( sizeof(float) * nRg2 );

            int no = ParseFloatArray( tag.Value(), pRg2, nRg2 );
            DCT_ASSERT( (no == nRg2) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MAXDIST2_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pMaxDist2) )
        {
            nMaxDist2 = tag.Size();
            pMaxDist2 = (float *)malloc( sizeof(float) * nMaxDist2 );

            int no = ParseFloatArray( tag.Value(), pMaxDist2, nMaxDist2 );
            DCT_ASSERT( (no == nMaxDist2) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_GLOBALFADE1_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pGlobalFade1) )
        {
            nGlobalFade1 = tag.Size();
            pGlobalFade1 = (float *)malloc( sizeof(float) * nGlobalFade1 );

            int no = ParseFloatArray( tag.Value(), pGlobalFade1, nGlobalFade1 );
            DCT_ASSERT( (no == nGlobalFade1) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_GLOBALGAINDIST1_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pGlobalGainDistance1) )
        {
            nGlobalGainDistance1 = tag.Size();
            pGlobalGainDistance1 = (float *)malloc( sizeof(float) * nGlobalGainDistance1 );

            int no = ParseFloatArray( tag.Value(), pGlobalGainDistance1, nGlobalGainDistance1 );
            DCT_ASSERT( (no == nGlobalGainDistance1) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_GLOBALFADE2_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pGlobalFade2) )
        {
            nGlobalFade2 = tag.Size();
            pGlobalFade2 = (float *)malloc( sizeof(float) * nGlobalFade2 );

            int no = ParseFloatArray( tag.Value(), pGlobalFade2, nGlobalFade2 );
            DCT_ASSERT( (no == nGlobalFade2) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_GLOBALGAINDIST2_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pGlobalGainDistance2) )
        {
            nGlobalGainDistance2 = tag.Size();
            pGlobalGainDistance2 = (float *)malloc( sizeof(float) * nGlobalGainDistance2 );

            int no = ParseFloatArray( tag.Value(), pGlobalGainDistance2, nGlobalGainDistance2 );
            DCT_ASSERT( (no == nGlobalGainDistance2) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_FADE2_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pFade) )
        {
            nFade = tag.Size();
            pFade = (float *)malloc( sizeof(float) * nFade );

            int no = ParseFloatArray( tag.Value(), pFade, nFade );
            DCT_ASSERT( (no == nFade) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_CB_MIN_REGIONMAX_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pCbMinRegionMax) )
        {
            nCbMinRegionMax = tag.Size();
            pCbMinRegionMax = (float *)malloc( sizeof(float) * nCbMinRegionMax );

            int no = ParseFloatArray( tag.Value(), pCbMinRegionMax, nCbMinRegionMax );
            DCT_ASSERT( (no == nCbMinRegionMax) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_CR_MIN_REGIONMAX_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pCrMinRegionMax) )
        {
            nCrMinRegionMax = tag.Size();
            pCrMinRegionMax = (float *)malloc( sizeof(float) * nCrMinRegionMax );

            int no = ParseFloatArray( tag.Value(), pCrMinRegionMax, nCrMinRegionMax );
            DCT_ASSERT( (no == nCrMinRegionMax) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MAX_CSUM_REGIONMAX_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pMaxCSumRegionMax) )
        {
            nMaxCSumRegionMax = tag.Size();
            pMaxCSumRegionMax = (float *)malloc( sizeof(float) * nMaxCSumRegionMax );

            int no = ParseFloatArray( tag.Value(), pMaxCSumRegionMax, nMaxCSumRegionMax );
            DCT_ASSERT( (no == nMaxCSumRegionMax) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_CB_MIN_REGIONMIN_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pCbMinRegionMin) )
        {
            nCbMinRegionMin = tag.Size();
            pCbMinRegionMin = (float *)malloc( sizeof(float) * nCbMinRegionMin );

            int no = ParseFloatArray( tag.Value(), pCbMinRegionMin, nCbMinRegionMin );
            DCT_ASSERT( (no == nCbMinRegionMin) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_CR_MIN_REGIONMIN_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pCrMinRegionMin) )
        {
            nCrMinRegionMin = tag.Size();
            pCrMinRegionMin = (float *)malloc( sizeof(float) * nCrMinRegionMin );

            int no = ParseFloatArray( tag.Value(), pCrMinRegionMin, nCrMinRegionMin );
            DCT_ASSERT( (no == nCrMinRegionMin) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MAX_CSUM_REGIONMIN_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0)
                    && (NULL == pMaxCSumRegionMin) )
        {
            nMaxCSumRegionMin = tag.Size();
            pMaxCSumRegionMin = (float *)malloc( sizeof(float) * nMaxCSumRegionMin );

            int no = ParseFloatArray( tag.Value(), pMaxCSumRegionMin, nMaxCSumRegionMin );
            DCT_ASSERT( (no == nMaxCSumRegionMin) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MINC_REGIONMAX_TAG)
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0)
                  && (NULL == pMinCRegionMax) )
        {
            nMinCRegionMax = tag.Size();
            pMinCRegionMax = (float *)malloc( sizeof(float) * nMinCRegionMax );

            int no = ParseFloatArray( tag.Value(), pMinCRegionMax, nMinCRegionMax );
            DCT_ASSERT( (no == nMinCRegionMax) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MINC_REGIONMIN_TAG)
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0)
                  && (NULL == pMinCRegionMin) )
        {
            nMinCRegionMin = tag.Size();
            pMinCRegionMin = (float *)malloc( sizeof(float) * nMinCRegionMin );

            int no = ParseFloatArray( tag.Value(), pMinCRegionMin, nMinCRegionMin );
            DCT_ASSERT( (no == nMinCRegionMin) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MAXY_REGIONMAX_TAG)
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0)
                  && (NULL == pMaxYRegionMax) )
        {
            nMaxYRegionMax = tag.Size();
            pMaxYRegionMax = (float *)malloc( sizeof(float) * nMaxYRegionMax );

            int no = ParseFloatArray( tag.Value(), pMaxYRegionMax, nMaxYRegionMax );
            DCT_ASSERT( (no == nMaxYRegionMax) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MAXY_REGIONMIN_TAG)
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0)
                  && (NULL == pMaxYRegionMin) )
        {
            nMaxYRegionMin = tag.Size();
            pMaxYRegionMin = (float *)malloc( sizeof(float) * nMaxYRegionMin );

            int no = ParseFloatArray( tag.Value(), pMaxYRegionMin, nMaxYRegionMin );
            DCT_ASSERT( (no == nMaxYRegionMin) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MINY_MAXG_REGIONMAX_TAG)
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0)
                  && (NULL == pMinYMaxGRegionMax) )
        {
            nMinYMaxGRegionMax = tag.Size();
            pMinYMaxGRegionMax = (float *)malloc( sizeof(float) * nMinYMaxGRegionMax );

            int no = ParseFloatArray( tag.Value(), pMinYMaxGRegionMax, nMinYMaxGRegionMax );
            DCT_ASSERT( (no == nMinYMaxGRegionMax) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_MINY_MAXG_REGIONMIN_TAG)
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0)
                  && (NULL == pMinYMaxGRegionMin) )
        {
            nMinYMaxGRegionMin = tag.Size();
            pMinYMaxGRegionMin = (float *)malloc( sizeof(float) * nMinYMaxGRegionMin );

            int no = ParseFloatArray( tag.Value(), pMinYMaxGRegionMin, nMinYMaxGRegionMin );
            DCT_ASSERT( (no == nMinYMaxGRegionMin) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_REFCB_TAG)
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0)
                  && (NULL == pRefCb) )
        {
            nRefCb = tag.Size();
            pRefCb = (float *)malloc( sizeof(float) * nRefCb );

            int no = ParseFloatArray( tag.Value(), pRefCb, nRefCb );
            DCT_ASSERT( (no == nRefCb) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_REFCR_TAG)
                  && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                  && (tag.Size() > 0)
                  && (NULL == pRefCr) )
        {
            nRefCr = tag.Size();
            pRefCr = (float *)malloc( sizeof(float) * nRefCr );

            int no = ParseFloatArray( tag.Value(), pRefCr, nRefCr );
            DCT_ASSERT( (no == nRefCr) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_REGION_ADJUST_ENABLE_TAG)
                  && (tag.Size() > 0) )
        {
			int no = ParseUshortArray( tag.Value(), &awb_data.AwbFade2Parm.regionAdjustEnable, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if(tagname==CALIB_SENSOR_AWB_GLOBALS_MEASWINWIDTHSCALE_TAG)
		{
			int no = ParseFloatArray( tag.Value(), &awb_data.awbMeasWinWidthScale, 1 );
			DCT_ASSERT( (no == tag.Size()) );
		}
		else if(tagname==CALIB_SENSOR_AWB_GLOBALS_MEASWINHEIGHTSCALE_TAG)
		{
			int no = ParseFloatArray( tag.Value(), &awb_data.awbMeasWinHeightScale, 1 );
			DCT_ASSERT( (no == tag.Size()) );
		}
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_INDOOR_MIN_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjIndoorMin, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_OUTDOOR_MIN_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjOutdoorMin, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_MAX_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjMax, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_MAX_SKY_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjMaxSky, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_A_LIMIT )	//oyyf
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjALimit, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_A_WEIGHT )	//oyyf
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjAWeight, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_YELLOW_LIMIT )	//oyyf
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjYellowLimit, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_ILL_TO_CWF )	//oyyf
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjIllToCwf, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_RGPROJ_ILL_TO_CWF_WEIGHT )	//oyyf
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRgProjIllToCwfWeight, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_CLIP_OUTDOOR )
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( awb_data.outdoor_clipping_profile,
                    value, sizeof( awb_data.outdoor_clipping_profile ) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_REGION_SIZE )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRegionSize, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_REGION_SIZE_INC )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRegionSizeInc, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_GLOBALS_REGION_SIZE_DEC )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &awb_data.fRegionSizeDec, 1 );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( tagname == CALIB_SENSOR_AWB_GLOBALS_IIR )
        {
            const XMLNode *psubchild = pchild->ToElement()->FirstChild();
            while ( psubchild )
            {
                XmlTag tag = XmlTag( psubchild->ToElement() );
                std::string subTagname( psubchild->ToElement()->Name() ); 
                
                if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_DAMP_COEF_ADD)
                        && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                        && (tag.Size() > 0) )
                {
                    int no = ParseFloatArray( tag.Value(), &awb_data.IIR.fIIRDampCoefAdd, 1 );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_DAMP_COEF_SUB)
                           && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                           && (tag.Size() > 0) )
                {
                    int no = ParseFloatArray( tag.Value(), &awb_data.IIR.fIIRDampCoefSub, 1 );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_DAMP_FILTER_THRESHOLD)
                           && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                           && (tag.Size() > 0) )
                {
                    int no = ParseFloatArray( tag.Value(), &awb_data.IIR.fIIRDampFilterThreshold, 1 );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_DAMPING_COEF_MIN)
                           && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                           && (tag.Size() > 0) )
                {
                    int no = ParseFloatArray( tag.Value(), &awb_data.IIR.fIIRDampingCoefMin, 1 );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_DAMPING_COEF_MAX)
                           && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                           && (tag.Size() > 0) )
                {
                    int no = ParseFloatArray( tag.Value(), &awb_data.IIR.fIIRDampingCoefMax, 1 );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_DAMPING_COEF_INIT)
                           && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                           && (tag.Size() > 0) )
                {
                    int no = ParseFloatArray( tag.Value(), &awb_data.IIR.fIIRDampingCoefInit, 1 );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_EXP_PRIOR_FILTER_SIZE_MAX)
                           && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                           && (tag.Size() > 0) )
                {
                    int no = ParseUshortArray( tag.Value(), &awb_data.IIR.IIRFilterSize, 1 );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_EXP_PRIOR_FILTER_SIZE_MIN)
                            && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                            && (tag.Size() > 0) )
                {
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_GLOBALS_IIR_EXP_PRIOR_MIDDLE)
                           && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                           && (tag.Size() > 0) )
                {
                    int no = ParseFloatArray( tag.Value(), &awb_data.IIR.fIIRFilterInitValue, 1 );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else
                {
                    TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB GLOBALS - IIR  section (unknow tag:%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                    return ( false );
                }

                psubchild = psubchild->NextSibling();
            }
        }
        else
        {
            TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB section (unknow tag:%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    DCT_ASSERT( (nRg1 == nMaxDist1) );
    DCT_ASSERT( (nRg2 == nMaxDist2) );

    DCT_ASSERT( (nGlobalFade1 == nGlobalGainDistance1) );
    DCT_ASSERT( (nGlobalFade2 == nGlobalGainDistance2) );

    DCT_ASSERT( (nFade == nCbMinRegionMax) );
    DCT_ASSERT( (nFade == nCrMinRegionMax) );
    DCT_ASSERT( (nFade == nMaxCSumRegionMax) );
    DCT_ASSERT( (nFade == nCbMinRegionMin) );
    DCT_ASSERT( (nFade == nCrMinRegionMin) );
    DCT_ASSERT( (nFade == nMaxCSumRegionMin) );

    /* CamAwbClipParm_t */
    awb_data.AwbClipParam.ArraySize1    = nRg1;
    awb_data.AwbClipParam.pRg1          = pRg1;
    awb_data.AwbClipParam.pMaxDist1     = pMaxDist1;
    awb_data.AwbClipParam.ArraySize2    = nRg2;
    awb_data.AwbClipParam.pRg2          = pRg2;
    awb_data.AwbClipParam.pMaxDist2     = pMaxDist2;

    /* CamAwbGlobalFadeParm_t */
    awb_data.AwbGlobalFadeParm.ArraySize1           = nGlobalFade1;
    awb_data.AwbGlobalFadeParm.pGlobalFade1         = pGlobalFade1;
    awb_data.AwbGlobalFadeParm.pGlobalGainDistance1 = pGlobalGainDistance1;
    awb_data.AwbGlobalFadeParm.ArraySize2           = nGlobalFade2;
    awb_data.AwbGlobalFadeParm.pGlobalFade2         = pGlobalFade2;
    awb_data.AwbGlobalFadeParm.pGlobalGainDistance2 = pGlobalGainDistance2;

    /* CamAwbFade2Parm_t */
    awb_data.AwbFade2Parm.ArraySize         = nFade;
    awb_data.AwbFade2Parm.pFade             = pFade;
    awb_data.AwbFade2Parm.pCbMinRegionMax   = pCbMinRegionMax;
    awb_data.AwbFade2Parm.pCrMinRegionMax   = pCrMinRegionMax;
    awb_data.AwbFade2Parm.pMaxCSumRegionMax = pMaxCSumRegionMax;
    awb_data.AwbFade2Parm.pCbMinRegionMin   = pCbMinRegionMin;
    awb_data.AwbFade2Parm.pCrMinRegionMin   = pCrMinRegionMin;
    awb_data.AwbFade2Parm.pMaxCSumRegionMin = pMaxCSumRegionMin;

	awb_data.AwbFade2Parm.pMinCRegionMax	= pMinCRegionMax;
	awb_data.AwbFade2Parm.pMinCRegionMin	= pMinCRegionMax;
	awb_data.AwbFade2Parm.pMaxYRegionMax	= pMaxYRegionMax;
	awb_data.AwbFade2Parm.pMaxYRegionMin	= pMaxYRegionMin;
	awb_data.AwbFade2Parm.pMinYMaxGRegionMax = pMinYMaxGRegionMax;
	awb_data.AwbFade2Parm.pMinYMaxGRegionMin = pMinYMaxGRegionMin;
	awb_data.AwbFade2Parm.pRefCb = pRefCb;
	awb_data.AwbFade2Parm.pRefCr = pRefCr;

    RESULT result = CamCalibDbAddAwbGlobal( m_CalibDbHandle, &awb_data );
    DCT_ASSERT( result == RET_SUCCESS );

    /* cleanup */
    free( pRg1 );
    free( pMaxDist1 );
    free( pRg2 );
    free( pMaxDist2 );

    free( pGlobalFade1 );
    free( pGlobalGainDistance1 );
    free( pGlobalFade2 );
    free( pGlobalGainDistance2 );

    free( pFade );
    free( pCbMinRegionMax );
    free( pCrMinRegionMax );
    free( pMaxCSumRegionMax );
    free( pCbMinRegionMin );
    free( pCrMinRegionMin );
    free( pMaxCSumRegionMin );

    free( pMinCRegionMax );
    free( pMinCRegionMin );
    free( pMaxYRegionMax );
    free( pMaxYRegionMin );
    free( pMinYMaxGRegionMax );
    free( pMinYMaxGRegionMin );	
    free( pRefCb );
    free( pRefCr );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);
    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryAwbIllumination
 *****************************************************************************/
bool CalibDb::parseEntryAwbIllumination
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);	

    CamIlluProfile_t illu;
    MEMSET( &illu, 0, sizeof( illu ) );

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_AWB_ILLUMINATION_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( illu.name, value, sizeof( illu.name ) );  
            std::cout << value << std::endl;
        }
        else if ( (tagname == CALIB_SENSOR_AWB_ILLUMINATION_DOOR_TYPE_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            std::string s_value(value);

            if ( s_value == CALIB_SENSOR_AWB_ILLUMINATION_DOOR_TYPE_INDOOR )
            {
                illu.DoorType = CAM_DOOR_TYPE_INDOOR;
            }
            else if ( s_value == CALIB_SENSOR_AWB_ILLUMINATION_DOOR_TYPE_OUTDOOR )
            {
                illu.DoorType = CAM_DOOR_TYPE_OUTDOOR;
            }
            else
            {
               TRACE( CALIBDB_ERROR, "%s(%d): invalid illumination doortype (%s)\n", __FUNCTION__,__LINE__,s_value.c_str());
            }          
           
        }
        else if ( (tagname == CALIB_SENSOR_AWB_ILLUMINATION_AWB_TYPE_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            std::string s_value(value);

            if ( s_value == CALIB_SENSOR_AWB_ILLUMINATION_AWB_TYPE_MANUAL )
            {
                illu.AwbType = CAM_AWB_TYPE_MANUAL;
            }
            else if ( s_value == CALIB_SENSOR_AWB_ILLUMINATION_AWB_TYPE_AUTO )
            {
                illu.AwbType = CAM_AWB_TYPE_AUTO;
            }
            else
            {
               TRACE( CALIBDB_ERROR, "%s(%d): invalid AWB type (%s)\n", __FUNCTION__,__LINE__,s_value.c_str());
            }

        }
        else if ( (tagname == CALIB_SENSOR_AWB_ILLUMINATION_MANUAL_WB_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(illu.ComponentGain) / sizeof(illu.ComponentGain.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), illu.ComponentGain.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_ILLUMINATION_MANUAL_CC_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(illu.CrossTalkCoeff) / sizeof(illu.CrossTalkCoeff.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), illu.CrossTalkCoeff.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_AWB_ILLUMINATION_MANUAL_CTO_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(illu.CrossTalkOffset) / sizeof(illu.CrossTalkOffset.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), illu.CrossTalkOffset.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_GMM_TAG )
        {
            const XMLNode * psubchild = pchild->ToElement()->FirstChild();
            while ( psubchild )
            {
                XmlTag tag = XmlTag( psubchild->ToElement() );
                std::string subTagname(psubchild->ToElement()->Name());
                
                if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_GMM_GAUSSIAN_MVALUE_TAG)
                        && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                        && (tag.Size() > 0) )
                {
                    int i = ( sizeof(illu.GaussMeanValue) / sizeof(illu.GaussMeanValue.fCoeff[0]) );
                    int no = ParseFloatArray( tag.Value(), illu.GaussMeanValue.fCoeff, i );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_GMM_INV_COV_MATRIX_TAG)
                            && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                            && (tag.Size() > 0) )
                {
                    int i = ( sizeof(illu.CovarianceMatrix) / sizeof(illu.CovarianceMatrix.fCoeff[0]) );
                    int no = ParseFloatArray( tag.Value(), illu.CovarianceMatrix.fCoeff, i );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_GMM_GAUSSIAN_SFACTOR_TAG)
                            && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                            && (tag.Size() > 0) )
                {
                    int i = ( sizeof(illu.GaussFactor) / sizeof(illu.GaussFactor.fCoeff[0]) );
                    int no = ParseFloatArray( tag.Value(), illu.GaussFactor.fCoeff, i );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_GMM_TAU_TAG )
                            && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                            && (tag.Size() > 0) )
                {
                    int i = ( sizeof(illu.Threshold) / sizeof(illu.Threshold.fCoeff[0]) );
                    int no = ParseFloatArray( tag.Value(), illu.Threshold.fCoeff, i );
                    DCT_ASSERT( (no == tag.Size()) );
                }
                else
                {   
                    TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB gaussian mixture modell section (unknow tag:%s)\n", __FUNCTION__,__LINE__,subTagname.c_str());
                    return ( false );
                }

                psubchild = psubchild->NextSibling();
            }
        }
        else if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_SAT_CT_TAG )
        {
            float *afGain   = NULL;
            int n_gains     = 0;
            float *afSat    = NULL;
            int n_sats      = 0;

            const XMLNode *psubchild = pchild->ToElement()->FirstChild();
            while ( psubchild )
            {
                XmlTag tag = XmlTag( psubchild->ToElement() );
                std::string subTagname(psubchild->ToElement()->Name());

                if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_SAT_CT_GAIN_TAG)
                        && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                        && (tag.Size() > 0) )
                {
                    if ( !afGain )
                    {
                        n_gains = tag.Size();
                        afGain  = (float *)malloc( (n_gains * sizeof( float )) );
                        MEMSET( afGain, 0, (n_gains * sizeof( float )) );
                    }

                    int no = ParseFloatArray( tag.Value(), afGain, n_gains );
                    DCT_ASSERT( (no == n_gains) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_SAT_CT_SAT_TAG)
                            && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                            && (tag.Size() > 0) )
                {
                    if ( !afSat )
                    {
                        n_sats = tag.Size();
                        afSat = (float *)malloc( (n_sats * sizeof( float )) );
                        MEMSET( afSat, 0, (n_sats * sizeof( float )) );
                    }

                    int no = ParseFloatArray( tag.Value(), afSat, n_sats );
                    DCT_ASSERT( (no == n_sats) );
                }
                else
                {
                    TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB saturation curve section (unknow tag:%s)\n", __FUNCTION__,__LINE__,subTagname.c_str());
                    return ( false );
                }

                psubchild = psubchild->NextSibling();
            }

            DCT_ASSERT( (n_gains == n_sats) );
            illu.SaturationCurve.ArraySize      = n_gains;
            illu.SaturationCurve.pSensorGain    = afGain;
            illu.SaturationCurve.pSaturation    = afSat;
        }
        else if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_VIG_CT_TAG )
        {
            float *afGain   = NULL;
            int n_gains     = 0;
            float *afVig    = NULL;
            int n_vigs      = 0;

            const XMLNode * psubchild = pchild->ToElement()->FirstChild();
            while ( psubchild )
            {
                XmlTag tag = XmlTag( psubchild->ToElement() );
                std::string subTagname(psubchild->ToElement()->Name());

                if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_VIG_CT_GAIN_TAG )
                        && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                        && (tag.Size() > 0) )
                {
                    if ( !afGain )
                    {
                        n_gains = tag.Size();
                        afGain  = (float *)malloc( (n_gains * sizeof( float )) );
                        MEMSET( afGain, 0, (n_gains * sizeof( float )) );
                    }

                    int no = ParseFloatArray( tag.Value(), afGain, n_gains );
                    DCT_ASSERT( (no == n_gains) );
                }
                else if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_VIG_CT_VIG_TAG )
                            && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                            && (tag.Size() > 0) )
                {
                    if ( !afVig )
                    {
                        n_vigs = tag.Size();
                        afVig = (float *)malloc( (n_vigs * sizeof( float )) );
                        MEMSET( afVig, 0, (n_vigs * sizeof( float )) );
                    }

                    int no = ParseFloatArray( tag.Value(), afVig, n_vigs);
                    DCT_ASSERT( (no == n_vigs) );
                }
                else
                {
                    TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB vignetting curve section (unknow tag:%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                    return ( false );
                }

                psubchild = psubchild->NextSibling();
            }

            DCT_ASSERT( (n_gains == n_vigs) );
            illu.VignettingCurve.ArraySize      = n_gains;
            illu.VignettingCurve.pSensorGain    = afGain;
            illu.VignettingCurve.pVignetting    = afVig;
        }
        else if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_ALSC_TAG )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryAwbIlluminationAlsc, &illu  ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB aLSC (%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_ACC_TAG )
        {
            const XMLNode * psubchild = pchild->ToElement()->FirstChild();
            while ( psubchild )
            {
                XmlTag tag = XmlTag( psubchild->ToElement() );
                std::string subTagname(psubchild->ToElement()->Name());

                if ( (subTagname == CALIB_SENSOR_AWB_ILLUMINATION_ACC_CC_PROFILE_LIST_TAG)
                        && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                        && (tag.Size() > 0) )
                {
                    char* value = Toupper(tag.Value());
                    int no = ParseCcProfileArray( value, illu.cc_profiles, CAM_NO_CC_PROFILES );
                    DCT_ASSERT( (no <= CAM_NO_CC_PROFILES) );
                    illu.cc_no = no;
                }
                else
                {
                    TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB aCC (%s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                    return ( false );
                }

                psubchild = psubchild->NextSibling();
            }
        }
        else
        {
            TRACE( CALIBDB_ERROR, "%s(%d): parse error in AWB illumination section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddIllumination( m_CalibDbHandle, &illu );
    DCT_ASSERT( result == RET_SUCCESS );

    /* cleanup */
    free( illu.SaturationCurve.pSensorGain );
    free( illu.SaturationCurve.pSaturation );
    free( illu.VignettingCurve.pSensorGain );
    free( illu.VignettingCurve.pVignetting );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryAwbIlluminationAlsc
 *****************************************************************************/
bool CalibDb::parseEntryAwbIlluminationAlsc
(
    const XMLElement   *pelement,
    void                *param
)
{
    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);	

    if ( !param )
    {
        return ( false );
    }

    CamIlluProfile_t *pIllu = ( CamIlluProfile_t * )param;

    char* lsc_profiles;
    int resIdx = -1;

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_ALSC_RES_LSC_PROFILE_LIST_TAG )
        {
            lsc_profiles = Toupper(tag.Value());
        }
        else if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_ALSC_RES_TAG )
        {
            const char* value = tag.Value();

            RESULT result = CamCalibDbGetResolutionIdxByName( m_CalibDbHandle, value, &resIdx);
            DCT_ASSERT( result == RET_SUCCESS );
        }
        else
        {
            TRACE( CALIBDB_ERROR, "%s(%d): unknown aLSC tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    DCT_ASSERT( resIdx != -1 );

    int no = ParseLscProfileArray( lsc_profiles, pIllu->lsc_profiles[resIdx], CAM_NO_LSC_PROFILES );
    DCT_ASSERT( (no <= CAM_NO_LSC_PROFILES) );
    pIllu->lsc_no[resIdx] = no;

    pIllu->lsc_res_no++;

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryAwbIlluminationAcc
 *****************************************************************************/
bool CalibDb::parseEntryAwbIlluminationAcc
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( tagname == CALIB_SENSOR_AWB_ILLUMINATION_ACC_CC_PROFILE_LIST_TAG )
        {
        }
        else
        {
            TRACE( CALIBDB_ERROR, "%s(%d): unknown aCC tag: %s\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}

/******************************************************************************
 * CalibDb::parseEntryLsc
 *****************************************************************************/
bool CalibDb::parseEntryLsc
(
    const XMLElement   *pelement,
    void                *param
)
{
    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamLscProfile_t lsc_profile;
    MEMSET( &lsc_profile, 0, sizeof( lsc_profile ) );

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_LSC_PROFILE_NAME_TAG )
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( lsc_profile.name, value, sizeof( lsc_profile.name ) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_RESOLUTION_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            const char* value = tag.Value();
            strncpy( lsc_profile.resolution, value, sizeof( lsc_profile.resolution ) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_ILLUMINATION_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( lsc_profile.illumination, value, sizeof( lsc_profile.illumination ) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_SECTORS_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseUshortArray( tag.Value(), &lsc_profile.LscSectors, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_NO_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseUshortArray( tag.Value(), &lsc_profile.LscNo, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_XO_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseUshortArray( tag.Value(), &lsc_profile.LscXo, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( tagname == CALIB_SENSOR_LSC_PROFILE_LSC_YO_TAG )
        {
            int no = ParseUshortArray( tag.Value(), &lsc_profile.LscYo, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_SECTOR_SIZE_X_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(lsc_profile.LscXSizeTbl) / sizeof(lsc_profile.LscXSizeTbl[0]) );
            int no = ParseUshortArray( tag.Value(), lsc_profile.LscXSizeTbl, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_SECTOR_SIZE_Y_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(lsc_profile.LscYSizeTbl) / sizeof(lsc_profile.LscYSizeTbl[0]) );
            int no = ParseUshortArray( tag.Value(), lsc_profile.LscYSizeTbl, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_VIGNETTING_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), (float *)(&lsc_profile.vignetting), 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_SAMPLES_RED_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_RED])
                            / sizeof(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_RED].uCoeff[0]) );
            int no = ParseUshortArray( tag.Value(),
                            (lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_RED].uCoeff), i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_SAMPLES_GREENR_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_GREENR])
                            / sizeof(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_GREENR].uCoeff[0]) );
            int no = ParseUshortArray( tag.Value(),
                            lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_GREENR].uCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_SAMPLES_GREENB_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_GREENB])
                            / sizeof(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_GREENB].uCoeff[0]) );
            int no = ParseUshortArray( tag.Value(),
                            (uint16_t *)(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_GREENB].uCoeff), i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_LSC_PROFILE_LSC_SAMPLES_BLUE_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_BLUE])
                            / sizeof(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_BLUE].uCoeff[0]) );
            int no = ParseUshortArray( tag.Value(),
                            (uint16_t *)(lsc_profile.LscMatrix[CAM_4CH_COLOR_COMPONENT_BLUE].uCoeff), i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in LSC section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddLscProfile( m_CalibDbHandle, &lsc_profile );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryCc
 *****************************************************************************/
bool CalibDb::parseEntryCc
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamCcProfile_t cc_profile;
    MEMSET( &cc_profile, 0, sizeof( cc_profile ) );

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_CC_PROFILE_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( cc_profile.name, value, sizeof( cc_profile.name ) );
        }
        else if ( (tagname == CALIB_SENSOR_CC_PROFILE_SATURATION_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseFloatArray( tag.Value(), &cc_profile.saturation, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SENSOR_CC_PROFILE_CC_MATRIX_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(cc_profile.CrossTalkCoeff) / sizeof(cc_profile.CrossTalkCoeff.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), cc_profile.CrossTalkCoeff.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_CC_PROFILE_CC_OFFSETS_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(cc_profile.CrossTalkOffset) / sizeof(cc_profile.CrossTalkOffset.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), cc_profile.CrossTalkOffset.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SENSOR_CC_PROFILE_WB_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(cc_profile.ComponentGain) / sizeof(cc_profile.ComponentGain.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), cc_profile.ComponentGain.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in CC section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddCcProfile( m_CalibDbHandle, &cc_profile );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryBls
 *****************************************************************************/
bool CalibDb::parseEntryBls
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamBlsProfile_t bls_profile;
    MEMSET( &bls_profile, 0, sizeof( bls_profile ) );

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_BLS_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( bls_profile.name, value, sizeof( bls_profile.name ) );
        }
        else if ( (tagname == CALIB_SENSOR_BLS_RESOLUTION_TAG )
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            const char* value = tag.Value();
            strncpy( bls_profile.resolution, value, sizeof( bls_profile.resolution ) );
        }
        else if ( (tagname == CALIB_SENSOR_BLS_DATA_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                && (tag.Size() > 0) )
        {
            int i = ( sizeof(bls_profile.level) / sizeof(bls_profile.level.uCoeff[0]) );
            int no = ParseUshortArray( tag.Value(), bls_profile.level.uCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in BLS section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddBlsProfile( m_CalibDbHandle, &bls_profile );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryCac
 *****************************************************************************/
bool CalibDb::parseEntryCac
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamCacProfile_t cac_profile;
    MEMSET( &cac_profile, 0, sizeof( cac_profile ) );

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_CAC_NAME_TAG )
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( cac_profile.name, value, sizeof( cac_profile.name ) );
        }
        else if ( (tagname == CALIB_SENSOR_CAC_RESOLUTION_TAG )
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            const char* value = tag.Value();
            strncpy( cac_profile.resolution, value, sizeof( cac_profile.resolution ) );
        }
        else if ( (tagname == CALIB_SESNOR_CAC_X_NORMSHIFT_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseByteArray( tag.Value(), &cac_profile.x_ns, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SESNOR_CAC_X_NORMFACTOR_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseByteArray( tag.Value(), &cac_profile.x_nf, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SESNOR_CAC_Y_NORMSHIFT_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseByteArray( tag.Value(), &cac_profile.y_ns, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SESNOR_CAC_Y_NORMFACTOR_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseByteArray( tag.Value(), &cac_profile.y_nf, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SESNOR_CAC_X_OFFSET_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseShortArray( tag.Value(), &cac_profile.hCenterOffset, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SESNOR_CAC_Y_OFFSET_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseShortArray( tag.Value(), &cac_profile.vCenterOffset, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( (tagname == CALIB_SESNOR_CAC_RED_PARAMETERS_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(cac_profile.Red) / sizeof(cac_profile.Red.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), cac_profile.Red.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( (tagname == CALIB_SESNOR_CAC_BLUE_PARAMETERS_TAG )
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int i = ( sizeof(cac_profile.Blue) / sizeof(cac_profile.Blue.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), cac_profile.Blue.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in CAC section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddCacProfile( m_CalibDbHandle, &cac_profile );
    DCT_ASSERT( result == RET_SUCCESS );
 
	TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryDpf
 *****************************************************************************/
bool CalibDb::parseEntryDpf
(
    const XMLElement  *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamDpfProfile_t dpf_profile;
    MEMSET( &dpf_profile, 0, sizeof( dpf_profile ) );

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_DPF_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( dpf_profile.name, value, sizeof( dpf_profile.name ) );
        }
        else if ( (tagname == CALIB_SENSOR_DPF_RESOLUTION_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            const char* value = tag.Value();
            strncpy( dpf_profile.resolution, value, sizeof( dpf_profile.resolution ) );
        }
        else if ( (tagname == CALIB_SENSOR_DPF_NLL_SEGMENTATION_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                    && (tag.Size() > 0) )
        {
            int no = ParseUshortArray( tag.Value(), &dpf_profile.nll_segmentation, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( tagname == CALIB_SENSOR_DPF_NLL_COEFF_TAG )
        {
            int i = ( sizeof(dpf_profile.nll_coeff) / sizeof(dpf_profile.nll_coeff.uCoeff[0]) );
            int no = ParseUshortArray( tag.Value(), dpf_profile.nll_coeff.uCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( tagname == CALIB_SENSOR_DPF_SIGMA_GREEN_TAG )
        {
            int no = ParseUshortArray( tag.Value(), &dpf_profile.SigmaGreen, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( tagname == CALIB_SENSOR_DPF_SIGMA_RED_BLUE_TAG )
        {
            int no = ParseUshortArray( tag.Value(), &dpf_profile.SigmaRedBlue, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( tagname == CALIB_SENSOR_DPF_GRADIENT_TAG )
        {
            int no = ParseFloatArray( tag.Value(), &dpf_profile.fGradient, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( tagname == CALIB_SENSOR_DPF_OFFSET_TAG )
        {
            int no = ParseFloatArray( tag.Value(), &dpf_profile.fOffset, 1 );
            DCT_ASSERT( (no == 1) );
        }
        else if ( tagname == CALIB_SENSOR_DPF_NLGAINS_TAG )
        {
            int i = ( sizeof(dpf_profile.NfGains) / sizeof(dpf_profile.NfGains.fCoeff[0]) );
            int no = ParseFloatArray( tag.Value(), dpf_profile.NfGains.fCoeff, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( tagname == CALIB_SESNOR_MFD_ENABLE_TAG )
        {
			int no = ParseByteArray( tag.Value(), &dpf_profile.Mfd.enable, 1 );
			DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( tagname == CALIB_SESNOR_MFD_GAIN_TAG )
        {
            int i = ( sizeof(dpf_profile.Mfd.gain) / sizeof(dpf_profile.Mfd.gain[0]) );
            int no = ParseFloatArray( tag.Value(), dpf_profile.Mfd.gain, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( tagname == CALIB_SESNOR_MFD_FRAMES_TAG )
        {
            int i = ( sizeof(dpf_profile.Mfd.frames) / sizeof(dpf_profile.Mfd.frames[0]) );
            int no = ParseFloatArray( tag.Value(), dpf_profile.Mfd.frames, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( tagname == CALIB_SESNOR_UVNR_ENABLE_TAG )
        {
			int no = ParseByteArray( tag.Value(), &dpf_profile.Uvnr.enable, 1 );
			DCT_ASSERT( (no == tag.Size()) );
        }
		else if ( tagname == CALIB_SESNOR_UVNR_GAIN_TAG )
		{
			int i = ( sizeof(dpf_profile.Uvnr.gain) / sizeof(dpf_profile.Uvnr.gain[0]) );
			int no = ParseFloatArray( tag.Value(), dpf_profile.Uvnr.gain, i );
			DCT_ASSERT( (no == tag.Size()) );
		}
		else if ( tagname == CALIB_SESNOR_UVNR_RATIO_TAG )
        {
            int i = ( sizeof(dpf_profile.Uvnr.ratio) / sizeof(dpf_profile.Uvnr.ratio[0]) );
            int no = ParseFloatArray( tag.Value(), dpf_profile.Uvnr.ratio, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
        else if ( tagname == CALIB_SESNOR_UVNR_DISTANCE_TAG )
        {
            int i = ( sizeof(dpf_profile.Uvnr.distances) / sizeof(dpf_profile.Uvnr.distances[0]) );
            int no = ParseFloatArray( tag.Value(), dpf_profile.Uvnr.distances, i );
            DCT_ASSERT( (no == tag.Size()) );
        }
		else if(tagname == CALIB_SENSOR_DPF_FILTERENABLE_TAG 
			&& (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
             && (tag.Size() > 0))
		{
			 int no = ParseFloatArray( tag.Value(), &dpf_profile.FilterEnable , 1 );
             DCT_ASSERT( (no == tag.Size()) );
			 
		}
		else if ( tagname == CALIB_SENSOR_DPF_DENOISELEVEL_TAG )
        {
            float *afGain   = NULL;
            int n_gains     = 0;
            float *afDlevel    = NULL;
            int n_Dlevels      = 0;
			int index=0;
            const XMLNode *psubchild = pchild->ToElement()->FirstChild();
            while ( psubchild )
            {
                XmlTag tag = XmlTag( psubchild->ToElement() );
                std::string subTagname(psubchild->ToElement()->Name());

                if ( (subTagname == CALIB_SENSOR_DPF_DENOISELEVEL_GAINS_TAG)
                        && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                        && (tag.Size() > 0) )
                {
                    if ( !afGain )
                    {
                        n_gains = tag.Size();
                        afGain  = (float *)malloc( (n_gains * sizeof( float )) );
                        MEMSET( afGain, 0, (n_gains * sizeof( float )) );
                    }

                    int no = ParseFloatArray( tag.Value(), afGain, n_gains );
                    DCT_ASSERT( (no == n_gains) );
                }
                else if ( (subTagname == CALIB_SENSOR_DPF_DENOISELEVEL_DLEVEL_TAG)
                            && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                            && (tag.Size() > 0) )
                {
                    if ( !afDlevel )
                    {
                        n_Dlevels = tag.Size();
                        afDlevel = (float *)malloc( (n_Dlevels * sizeof( float )) );
                        MEMSET( afDlevel, 0, (n_Dlevels * sizeof( float )) );
                    }

                    int no = ParseFloatArray( tag.Value(), afDlevel, n_Dlevels );
                    DCT_ASSERT( (no == n_Dlevels) );
                }
                else
                {
                    return ( false );
                }

                psubchild = psubchild->NextSibling();
            }

            DCT_ASSERT( (n_gains == n_Dlevels) );
            dpf_profile.DenoiseLevelCurve.ArraySize      = n_gains;
            dpf_profile.DenoiseLevelCurve.pSensorGain    = afGain;           
			dpf_profile.DenoiseLevelCurve.pDlevel = (CamerIcIspFltDeNoiseLevel_t *)malloc( (n_Dlevels * sizeof( CamerIcIspFltDeNoiseLevel_t )) );
				
            for(index=0; index < dpf_profile.DenoiseLevelCurve.ArraySize; index++)
            {
				dpf_profile.DenoiseLevelCurve.pDlevel[index] = (CamerIcIspFltDeNoiseLevel_t)((int)afDlevel[index]+1);
			}

			free(afDlevel);			
        }
	    else if ( tagname == CALIB_SENSOR_DPF_SHARPENINGLEVEL_TAG )
        {
            float *afGain   = NULL;
            int n_gains     = 0;
            float *afSlevel    = NULL;
            int n_Slevels      = 0;
			int index=0;
            const XMLNode *psubchild = pchild->ToElement()->FirstChild();
            while ( psubchild )
            {
                XmlTag tag = XmlTag( psubchild->ToElement() );
                std::string subTagname(psubchild->ToElement()->Name());

                if ( (subTagname == CALIB_SENSOR_DPF_SHARPENINGLEVEL_GAINS_TAG)
                        && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                        && (tag.Size() > 0) )
                {
                    if ( !afGain )
                    {
                        n_gains = tag.Size();
                        afGain  = (float *)malloc( (n_gains * sizeof( float )) );
                        MEMSET( afGain, 0, (n_gains * sizeof( float )) );
                    }

                    int no = ParseFloatArray( tag.Value(), afGain, n_gains );
                    DCT_ASSERT( (no == n_gains) );
                }
                else if ( (subTagname == CALIB_SENSOR_DPF_SHARPENINGLEVEL_SLEVEL_TAG)
                            && (tag.isType( XmlTag::TAG_TYPE_DOUBLE ))
                            && (tag.Size() > 0) )
                {
                    if ( !afSlevel )
                    {
                        n_Slevels = tag.Size();
                        afSlevel = (float *)malloc( (n_Slevels * sizeof( float )) );
                        MEMSET( afSlevel, 0, (n_Slevels * sizeof( float )) );
                    }

                    int no = ParseFloatArray( tag.Value(), afSlevel, n_Slevels );
                    DCT_ASSERT( (no == n_Slevels) );
                }
                else
                {
                    return ( false );
                }

                psubchild = psubchild->NextSibling();
            }

            DCT_ASSERT( (n_gains == n_Slevels) );
            dpf_profile.SharpeningLevelCurve.ArraySize      = n_gains;
            dpf_profile.SharpeningLevelCurve.pSensorGain    = afGain;          
            dpf_profile.SharpeningLevelCurve.pSlevel=(CamerIcIspFltSharpeningLevel_t *)malloc( (n_Slevels * sizeof( CamerIcIspFltSharpeningLevel_t )) );
            for(index=0;index<dpf_profile.SharpeningLevelCurve.ArraySize;index++)
            {
				dpf_profile.SharpeningLevelCurve.pSlevel[index]    	= (CamerIcIspFltSharpeningLevel_t)((int)afSlevel[index]+1);
			}			
			free(afSlevel);	
	    }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in DPF section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddDpfProfile( m_CalibDbHandle, &dpf_profile );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryDpcc
 *****************************************************************************/
bool CalibDb::parseEntryDpcc
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamDpccProfile_t dpcc_profile;
    MEMSET( &dpcc_profile, 0, sizeof( dpcc_profile ) );

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_DPCC_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            char* value = Toupper(tag.Value());
            strncpy( dpcc_profile.name, value, sizeof( dpcc_profile.name ) );
        }
        else if ( (tagname == CALIB_SENSOR_DPCC_RESOLUTION_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            const char* value = tag.Value();
            strncpy( dpcc_profile.resolution, value, sizeof( dpcc_profile.resolution ) );
        }
        else if ( (tagname == CALIB_SENSOR_DPCC_REGISTER_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CELL ))
                    && (tag.Size() > 0) )
        {
            if ( !parseEntryCell( pchild->ToElement(), tag.Size(), &CalibDb::parseEntryDpccRegisters, &dpcc_profile ) )
            {
				TRACE( CALIBDB_ERROR, "%s(%d): parse error in DPF section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
                return ( false );
            }
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in DPCC section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());

            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbAddDpccProfile( m_CalibDbHandle, &dpcc_profile );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntryDpccRegisters
 *****************************************************************************/
bool CalibDb::parseEntryDpccRegisters
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamDpccProfile_t *pDpcc_profile = (CamDpccProfile_t *)param;

    char*     reg_name;
    uint32_t    reg_value = 0U;

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        std::string tagname(pchild->ToElement()->Name());

        if ( (tagname == CALIB_SENSOR_DPCC_REGISTER_NAME_TAG)
                && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                && (tag.Size() > 0) )
        {
            reg_name = Toupper(tag.Value());
        }
        else if ( (tagname == CALIB_SENSOR_DPCC_REGISTER_VALUE_TAG)
                    && (tag.isType( XmlTag::TAG_TYPE_CHAR ))
                    && (tag.Size() > 0) )
        {
            bool ok;

            reg_value = tag.ValueToUInt( &ok );
            if ( !ok )
            {
				TRACE( CALIBDB_ERROR, "%s(%d): parse error: invalid DPCC register value %s/%s\n", __FUNCTION__,__LINE__,tagname.c_str(),tag.Value());
                return ( false );
            }
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in DPCC register section (unknow tag: %s)\n", __FUNCTION__,__LINE__,pchild->ToElement()->Name());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    std::string s_regname(reg_name);
    
    if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_MODE )
    {
        pDpcc_profile->isp_dpcc_mode = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_OUTPUT_MODE )
    {
        pDpcc_profile->isp_dpcc_output_mode = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_SET_USE )
    {
        pDpcc_profile->isp_dpcc_set_use = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_METHODS_SET_1 )
    {
        pDpcc_profile->isp_dpcc_methods_set_1 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_METHODS_SET_2 )
    {
        pDpcc_profile->isp_dpcc_methods_set_2 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_METHODS_SET_3 )
    {
        pDpcc_profile->isp_dpcc_methods_set_3 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_LINE_THRESH_1 )
    {
        pDpcc_profile->isp_dpcc_line_thresh_1 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_LINE_MAD_FAC_1 )
    {
        pDpcc_profile->isp_dpcc_line_mad_fac_1 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_PG_FAC_1 )
    {
        pDpcc_profile->isp_dpcc_pg_fac_1 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_RND_THRESH_1 )
    {
        pDpcc_profile->isp_dpcc_rnd_thresh_1 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_RG_FAC_1 )
    {
        pDpcc_profile->isp_dpcc_rg_fac_1 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_LINE_THRESH_2 )
    {
        pDpcc_profile->isp_dpcc_line_thresh_2 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_LINE_MAD_FAC_2 )
    {
        pDpcc_profile->isp_dpcc_line_mad_fac_2 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_PG_FAC_2 )
    {
        pDpcc_profile->isp_dpcc_pg_fac_2 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_RND_THRESH_2 )
    {
        pDpcc_profile->isp_dpcc_rnd_thresh_2 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_RG_FAC_2 )
    {
        pDpcc_profile->isp_dpcc_rg_fac_2 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_LINE_THRESH_3 )
    {
        pDpcc_profile->isp_dpcc_line_thresh_3 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_LINE_MAD_FAC_3 )
    {
        pDpcc_profile->isp_dpcc_line_mad_fac_3 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_PG_FAC_3 )
    {
        pDpcc_profile->isp_dpcc_pg_fac_3 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_RND_THRESH_3 )
    {
        pDpcc_profile->isp_dpcc_rnd_thresh_3 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_RG_FAC_3 )
    {
        pDpcc_profile->isp_dpcc_rg_fac_3 = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_RO_LIMITS )
    {
        pDpcc_profile->isp_dpcc_ro_limits = reg_value;
    }
    else if ( s_regname == CALIB_SENSOR_DPCC_REGISTER_ISP_DPCC_RND_OFFS )
    {
        pDpcc_profile->isp_dpcc_rnd_offs = reg_value;
    }
    else
    {
        TRACE( CALIBDB_ERROR, "%s(%d): unknown DPCC register (%s)\n", __FUNCTION__,__LINE__,s_regname.c_str());
    }
#if 0
    if(reg_name){
        free(reg_name);
        reg_name=NULL;
    }
#endif
	TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);
    return ( true );
}



/******************************************************************************
 * CalibDb::parseEntrySystem
 *****************************************************************************/
bool CalibDb::parseEntrySystem
(
    const XMLElement   *pelement,
    void                *param
)
{
    (void)param;

    TRACE( CALIBDB_INFO, "%s(%d): (enter)\n", __FUNCTION__,__LINE__);

    CamCalibSystemData_t system_data;
    MEMSET( &system_data, 0, sizeof( CamCalibSystemData_t ) );

    const XMLNode * pchild = pelement->FirstChild();
    while ( pchild )
    {
        XmlTag tag = XmlTag( pchild->ToElement() );
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());  

        if (tagname == CALIB_SYSTEM_AFPS_TAG)
        {
            const XMLNode * firstchild = pchild->ToElement()->FirstChild();
            if ( firstchild)
            {
                XmlTag firstTag = XmlTag( firstchild->ToElement() );
                std::string firstTagname(firstchild->ToElement()->Name()); 
                
                if ( (firstTagname == CALIB_SYSTEM_AFPS_DEFAULT_TAG)
                              && (firstTag.isType( XmlTag::TAG_TYPE_CHAR ))
                              && (firstTag.Size() > 0) )
                {
                    const char* value = firstTag.Value();
                    std::string s_value(value);                 
                    size_t find = s_value.find("on", 0);                    
                    system_data.AfpsDefault = (  (find==std::string::npos) ? BOOL_FALSE : BOOL_TRUE);
                }

            }
        }
        else
        {
			TRACE( CALIBDB_ERROR, "%s(%d): parse error in system section (unknow tag: %s)\n", __FUNCTION__,__LINE__,tagname.c_str());
            return ( false );
        }

        pchild = pchild->NextSibling();
    }

    RESULT result = CamCalibDbSetSystemData( m_CalibDbHandle, &system_data );
    DCT_ASSERT( result == RET_SUCCESS );

    TRACE( CALIBDB_INFO, "%s(%d): (exit)\n", __FUNCTION__,__LINE__);

    return ( true );
}

bool CalibDb::GetCalibXMLVersion(char *XMLVerBuf, int buflen)
{
	char buf[64];
	
	if(XMLVerBuf == NULL)
		return (false);
	if(m_CalibDbHandle == NULL)
		return (false);
	RESULT result = CamCalibDbGetSensorXmlVersion(m_CalibDbHandle, &buf);
	if(result == RET_SUCCESS)
		strncpy(XMLVerBuf,buf,buflen-1);
	else
		return (false);

	return (true);
}

