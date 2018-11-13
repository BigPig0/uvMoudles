#ifndef _H_FASTERXML_
#define _H_FASTERXML_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* util */

#if ( defined _WIN32 )
#ifndef _WINDLL_FUNC
#ifdef THIRD_UTIL_EXPORT
#define _WINDLL_FUNC		_declspec(dllexport)
#else
#define _WINDLL_FUNC		extern
#endif
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC
#endif
#endif

#define XMLESCAPE_EXPAND(_src_,_src_len_,_dst_,_dst_len_,_dst_remain_len_)		\
	(_dst_len_) = 0 ;								\
	if( (_src_len_) > 0 )								\
	{										\
		unsigned char	*_p_src_ = (unsigned char *)(_src_) ;			\
		unsigned char	*_p_src_end_ = (unsigned char *)(_src_) + (_src_len_) - 1 ;	\
		unsigned char	*_p_dst_ = (unsigned char *)(_dst_) ;			\
		int		_remain_len_ = (_dst_remain_len_) ;			\
		while( _p_src_ <= _p_src_end_ )						\
		{									\
			if( *(_p_src_) == '<' )						\
			{								\
				(_remain_len_)-=4; if( (_remain_len_) < 0 ) break;	\
				if( memcmp((_p_src_)+1,"![CDATA[",8) == 0 )		\
				{							\
					memcpy( (_p_dst_) , (_p_src_) , (_src_len_) );	\
					(_p_dst_) += (_src_len_) ;			\
					break;						\
				}							\
				memcpy( (_p_dst_) , "&lt;" , 4 );			\
				(_p_src_) += 1 ;					\
				(_p_dst_) += 4 ;					\
			}								\
			else if( *(_p_src_) == '&' )					\
			{								\
				(_remain_len_)-=5; if( (_remain_len_) < 0 ) break;	\
				memcpy( (_p_dst_) , "&amp;" , 5 );			\
				(_p_src_) += 1 ;					\
				(_p_dst_) += 5 ;					\
			}								\
			else if( *(_p_src_) == '>' )					\
			{								\
				(_remain_len_)-=4; if( (_remain_len_) < 0 ) break;	\
				memcpy( (_p_dst_) , "&gt;" , 4 );			\
				(_p_src_) += 1 ;					\
				(_p_dst_) += 4 ;					\
			}								\
			else								\
			{								\
				(_remain_len_)-=1; if( (_remain_len_) < 0 ) break;	\
				*(_p_dst_) = *(_p_src_) ;				\
				(_p_src_) += 1 ;					\
				(_p_dst_) += 1 ;					\
			}								\
		}									\
		if( _remain_len_ < 0 || _p_src_ <= _p_src_end_ )			\
		{									\
			(_dst_len_) = -1 ;						\
		}									\
		else									\
		{									\
			*(_p_dst_) = '\0' ;						\
			(_dst_len_) = (_p_dst_) - (unsigned char *)(_dst_) ;		\
		}									\
	}										\

#define XMLUNESCAPE_FOLD(_src_,_src_len_,_dst_,_dst_len_,_dst_remain_len_)	\
	if( (_src_len_) > 0 )							\
	{									\
		char	*_p_src_ = (_src_) ;					\
		char	*_p_src_end_ = (_src_) + (_src_len_) - 1 ;		\
		char	*_p_dst_ = (_dst_) ;					\
		char	*_p_dst_end_ = (_dst_) + (_dst_remain_len_) - 1 ;	\
		while( _p_src_ <= _p_src_end_ )					\
		{								\
			if( (_p_dst_) > (_p_dst_end_) )				\
				break;						\
			if( strncmp( (_p_src_) , "&lt;" , 4 ) == 0 )		\
			{							\
				*(_p_dst_) = '<' ;				\
				(_p_dst_)++;					\
				(_p_src_) += 4 ;				\
			}							\
			else if( strncmp( (_p_src_) , "&amp;" , 5 ) == 0 )	\
			{							\
				*(_p_dst_) = '&' ;				\
				(_p_dst_)++;					\
				(_p_src_) += 5 ;				\
			}							\
			else if( strncmp( (_p_src_) , "&gt;" , 4 ) == 0 )	\
			{							\
				*(_p_dst_) = '>' ;				\
				(_p_dst_)++;					\
				(_p_src_) += 4 ;				\
			}							\
			else							\
			{							\
				*(_p_dst_) = *(_p_src_) ;			\
				(_p_dst_)++;					\
				(_p_src_)++;					\
			}							\
		}								\
		if( (_p_dst_) > (_p_dst_end_) )					\
		{								\
			(_dst_len_) = -1 ;					\
		}								\
		else								\
		{								\
			*(_p_dst_) = '\0' ;					\
			(_dst_len_) = (_p_dst_) - (_dst_) ;			\
		}								\
	}									\

/* fastxml */

#define FASTERXML_ERROR_ALLOC			-9
#define FASTERXML_ERROR_INTERNAL		-11
#define FASTERXML_ERROR_END_OF_BUFFER		-13
#define FASTERXML_ERROR_FILENAME		-14
#define FASTERXML_ERROR_TOO_MANY_SKIPTAGS	-15
#define FASTERXML_ERROR_XML_INVALID		-100

#define FASTERXML_NODE_BRANCH		0x10
#define FASTERXML_NODE_LEAF		0x20
#define FASTERXML_NODE_ENTER		0x01
#define FASTERXML_NODE_LEAVE		0x02

typedef int funcCallbackOnXmlProperty( int type , char *xpath , int xpath_len , int xpath_size , char *propname , int propname_len , char *propvalue , int propvalue_len , char *content , int content_len , void *p );
_WINDLL_FUNC int TravelXmlPropertiesBuffer( char *properties , int properties_len , int type , char *xpath , int xpath_len , int xpath_size , char *content , int content_len
					   , funcCallbackOnXmlProperty *pfuncCallbackOnXmlProperty
					   , void *p );

typedef int funcCallbackOnXmlNode( int type , char *xpath , int xpath_len , int xpath_size , char *node , int node_len , char *properties , int properties_len , char *content , int content_len , void *p );
_WINDLL_FUNC int TravelXmlBuffer( char *xml_buffer , char *xpath , int xpath_size
				 , funcCallbackOnXmlNode *pfuncCallbackOnXmlNode
				 , void *p );
_WINDLL_FUNC int TravelXmlBuffer4( char *xml_buffer , char *xpath , int xpath_size
				 , funcCallbackOnXmlNode *pfuncCallbackOnXmlNode
				 , funcCallbackOnXmlNode *pfuncCallbackOnEnterXmlNode
				 , funcCallbackOnXmlNode *pfuncCallbackOnLeaveXmlNode
				 , funcCallbackOnXmlNode *pfuncCallbackOnXmlLeaf
				 , void *p );

_WINDLL_FUNC int AddSkipXmlTag( char *tag );
_WINDLL_FUNC int AddSkipHtmlTags();
_WINDLL_FUNC void CleanSkipXmlTags();

#ifdef __cplusplus
}
#endif

#endif

