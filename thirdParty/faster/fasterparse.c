/*!
* \file fasterparse.c
* \date 2018/11/21 13:25
*
* \author wlla
* Contact: user@company.com
*
* \brief 
*
* TODO: ��faster�ڷ�װһ�㣬���dom�ṹ��ֻ������һЩС�����ý������ã����ļ���ԭ���ӿ�
*
* \note
*/

#include "fasterxml.h"
#include "fasterjson.h"
#include "fasterparse.h"

struct parse_info {
    struct faster_node_s  *root;
    struct faster_node_s  *current;
};

int CallbackOnJsonNode(int type, char *jpath, int jpath_len, int jpath_size
                       , char *nodename, int nodename_len
                       , char *content, int content_len, void *p ) 
{
    if( type & FASTERJSON_NODE_BRANCH ) {
        if( type & FASTERJSON_NODE_ENTER ) {
            printf( "ENTER-BRANCH p[%s] jpath[%.*s] nodename[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename );
        } else if( type & FASTERJSON_NODE_LEAVE ) {
            printf( "LEAVE-BRANCH p[%s] jpath[%.*s] nodename[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename );
        }
    } else if( type & FASTERJSON_NODE_ARRAY ) {
        if( type & FASTERJSON_NODE_ENTER ) {
            printf( "ENTER-ARRAY  p[%s] jpath[%.*s] nodename[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename );
        } else if( type & FASTERJSON_NODE_LEAVE ) {
            printf( "LEAVE-ARRAY  p[%s] jpath[%.*s] nodename[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename );
        }
    } else if( type & FASTERJSON_NODE_LEAF ) {
        printf( "LEAF         p[%s] jpath[%.*s] nodename[%.*s] content[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename , content_len , content );
    }

    return 0;
}

int parse_json(faster_node_t** root, char* buff) {
    char	jpath[ 1024 + 1 ] ;
    int     ret = 0;
    struct parse_info p = {NULL, NULL};

    memset( jpath , 0x00 , sizeof(jpath) );
    ret = TravelJsonBuffer( buff, jpath, sizeof(jpath), &CallbackOnJsonNode, &p );
    if( ret && ret != FASTERJSON_ERROR_END_OF_BUFFER ){
        printf( "TravelJsonTree failed[%d]\n", ret );
        return ret;
    }
    *root = p.root;
    return ret;
}

int CallbackOnXmlProperty(int type,  char *xpath, int xpath_len, int xpath_size
                          , char *propname, int propname_len
                          , char *propvalue, int propvalue_len
                          , char *content , int content_len , void *p ) 
{
    struct parse_info* h;
    faster_attribute_t* new_a;
    printf( "    PROPERTY xpath[%s] propname[%.*s] propvalue[%.*s]\n", xpath , propname_len , propname , propvalue_len , propvalue );
    h = (struct parse_info*)p;
    if(!h->current)
        return -1;

    new_a = (faster_attribute_t*)malloc(sizeof(faster_attribute_t));
    new_a->name = propname;
    new_a->name_len = propname_len;
    new_a->value = propvalue;
    new_a->value_len = propvalue_len;
    new_a->next = NULL;

    if(!h->current->attr) {
        h->current->attr = new_a;
    } else {
        faster_attribute_t* tmp = h->current->attr;
        while(tmp->next)
            tmp = tmp->next;
        tmp->next = new_a;
    }
    return 0;
}

int CallbackOnXmlNode(int type, char *xpath, int xpath_len, int xpath_size
                      , char *nodename , int nodename_len 
                      , char *properties , int properties_len 
                      , char *content , int content_len , void *p ) 
{
    int nret = 0 ;
    struct parse_info* h = (struct parse_info*)p;

    if( type & FASTERXML_NODE_BRANCH ) {
        if( type & FASTERXML_NODE_ENTER ) {
            faster_node_t *new_n;
            printf( "ENTER-BRANCH xpath[%s] nodename[%.*s] properties[%.*s]\n", xpath, nodename_len, nodename, properties_len, properties );
            if ( xpath_len == 4 && xpath[0]=='/' && tolower(xpath[1])=='x'
                && tolower(xpath[2])=='m' && tolower(xpath[3]=='l') )
                return 0;

            new_n = (faster_node_t *)malloc(sizeof(faster_node_t));
            new_n->name = nodename;
            new_n->name_len = nodename_len;
            new_n->parent = NULL;
            new_n->next = NULL;
            new_n->first_child = NULL;
            new_n->attr = NULL;
            if(h->current == NULL) {
                h->current = new_n;
                h->root = new_n;
            } else {
                h->current->first_child = new_n;
                new_n->parent = h->current;
                h->current = new_n;
            }
        } else if( type & FASTERXML_NODE_LEAVE ) {
            printf( "LEAVE-BRANCH xpath[%s] nodename[%.*s] properties[%.*s]\n", xpath, nodename_len, nodename, properties_len, properties );
            h->current = h->current->parent;
            return 0;
        } else {
            faster_node_t *new_n;
            printf( "BRANCH       xpath[%s] nodename[%.*s] properties[%.*s]\n", xpath, nodename_len, nodename , properties_len , properties );
            if ( xpath_len == 4 && xpath[0]=='/' && tolower(xpath[1])=='x'
                && tolower(xpath[2])=='m' && tolower(xpath[3]=='l') )
                return 0;

            new_n = (faster_node_t *)malloc(sizeof(faster_node_t));
            new_n->name = nodename;
            new_n->name_len = nodename_len;
            new_n->parent = NULL;
            new_n->next = NULL;
            new_n->first_child = NULL;
            new_n->attr = NULL;
            if(h->current == NULL) {
                h->current = new_n;
                h->root = new_n;
            } else {
                h->current->next = new_n;
                new_n->parent = h->current->parent;
                h->current = new_n;
            }
        }
    } else if( type & FASTERXML_NODE_LEAF ) {
        printf( "LEAF         xpath[%s] nodename[%.*s] properties[%.*s] content[%.*s]\n", xpath, nodename_len , nodename , properties_len , properties , content_len , content );
    }

    if( properties && properties[0] ) {
        nret = TravelXmlPropertiesBuffer(properties, properties_len, type, xpath, xpath_len, xpath_size, content, content_len, CallbackOnXmlProperty, p) ;
        if ( nret )
            return nret;
    }

    return 0;
}

int parse_xml(faster_node_t** root, char* buff) {
    char	xpath[ 1024 + 1 ] ;
    int     ret = 0 ;
    struct parse_info p = {NULL, NULL};

    memset( xpath, 0x00, sizeof(xpath) );
    ret = TravelXmlBuffer( buff, xpath, sizeof(xpath), CallbackOnXmlNode, &p) ;
    if( ret && ret != FASTERXML_ERROR_END_OF_BUFFER ) {
        printf( "TravelXmlTree failed[%d]\n", ret );
        return ret;
    }
    *root = p.root;
    return ret;
}

void free_faster_node(faster_node_t* root) {
    if(!root)
        return;

    while(root->attr) {
        struct faster_attribute_s *attr = root->attr;
        root->attr = root->attr->next;
        free(attr);
    }
    free_faster_node(root->first_child);
    free_faster_node(root->next);
    free(root);
}