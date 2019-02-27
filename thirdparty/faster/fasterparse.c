/*!
* \file fasterparse.c
* \date 2018/11/21 13:25
*
* \author wlla
* Contact: user@company.com
*
* \brief 
*
* TODO: 将faster在封装一层，输出dom结构。只适用于一些小的配置解析适用，大文件用原生接口
*
* \note
*/

#include "fasterxml.h"
#include "fasterjson.h"
#include "fasterparse.h"
#include "utilc.h"

enum xml_state {
    xml_state_no = 0,
    xml_state_branch,   //<test />
    xml_state_enter,    //<test> <XXX>... </test>
    xml_state_leave,    //<test> <XXX>... </test>结束
    xml_state_leaf      //<test>XXX</test>
};

typedef struct _fxml_node_s {
    struct fxml_attr_s *attr;    //节点属性
    struct _fxml_node_s *parent;      //上级节点
    struct _fxml_node_s *next;        //下级节点
    struct _fxml_node_s *first_child; //子节点
    char   *name;       //节点名称
    int    name_len;    //节点名称长度
    char   *content;    //叶子节点内容
    int    content_len;
    //private
    int    node_type;
}_fxml_node_t;

struct parse_info {
    _fxml_node_t  *root;
    _fxml_node_t  *current;
    int last_type;
};

static int _on_json_node(int type, char *jpath, int jpath_len, int jpath_size
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

int parse_json(fxml_node_t** root, char* buff) {
    char	jpath[ 1024 + 1 ] ;
    int     ret = 0;
    struct parse_info p = {NULL, NULL};

    memset( jpath , 0x00 , sizeof(jpath) );
    ret = TravelJsonBuffer( buff, jpath, sizeof(jpath), &_on_json_node, &p );
    if( ret && ret != FASTERJSON_ERROR_END_OF_BUFFER ){
        printf( "TravelJsonTree failed[%d]\n", ret );
        return ret;
    }
    *root = p.root;
    return ret;
}

static int _on_xml_property(int type,  char *xpath, int xpath_len, int xpath_size
                          , char *propname, int propname_len
                          , char *propvalue, int propvalue_len
                          , char *content , int content_len , void *p ) 
{
    struct parse_info* h;
    fxml_attr_t* new_a;
    //printf( "    PROPERTY xpath[%s] propname[%.*s] propvalue[%.*s]\n", xpath , propname_len , propname , propvalue_len , propvalue );
    h = (struct parse_info*)p;
    if(!h->current)
        return -1;

    new_a = (fxml_attr_t*)malloc(sizeof(fxml_attr_t));
    new_a->name = propname;
    new_a->name_len = propname_len;
    new_a->value = propvalue;
    new_a->value_len = propvalue_len;
    new_a->next = NULL;

    if(!h->current->attr) {
        h->current->attr = new_a;
    } else {
        fxml_attr_t* tmp = h->current->attr;
        while(tmp->next)
            tmp = tmp->next;
        tmp->next = new_a;
    }
    return 0;
}

static int _on_xml_node(int type, char *xpath, int xpath_len, int xpath_size
                      , char *nodename , int nodename_len 
                      , char *properties , int properties_len 
                      , char *content , int content_len , void *p ) 
{
    int nret = 0 ;
    struct parse_info* h = (struct parse_info*)p;

    if( type & FASTERXML_NODE_BRANCH ) {
        if( type & FASTERXML_NODE_ENTER ) {
            _fxml_node_t *new_n;
            //printf( "ENTER-BRANCH xpath[%s] nodename[%.*s] properties[%.*s]\n", xpath, nodename_len, nodename, properties_len, properties );
            if ( xpath_len == 4 && xpath[0]=='/' && tolower(xpath[1])=='x'
                && tolower(xpath[2])=='m' && tolower(xpath[3]=='l') )
                return 0;

            new_n = (_fxml_node_t *)malloc(sizeof(_fxml_node_t));
            new_n->name = nodename;
            new_n->name_len = nodename_len;
            new_n->parent = NULL;
            new_n->next = NULL;
            new_n->first_child = NULL;
            new_n->attr = NULL;
            new_n->node_type = xml_state_enter;

            if(h->current == NULL) {
                h->current = new_n;
                h->root = new_n;
            } else if(h->current->node_type != xml_state_enter){
                h->current->next = new_n;
                new_n->parent = h->current->parent;
                h->current = new_n;
            } else {
                h->current->first_child = new_n;
                new_n->parent = h->current;
                h->current = new_n;
            }
        } else if( type & FASTERXML_NODE_LEAVE ) {
            //printf( "LEAVE-BRANCH xpath[%s] nodename[%.*s] properties[%.*s]\n", xpath, nodename_len, nodename, properties_len, properties );
            h->current = h->current->parent;
            if(h->current != NULL) {
                h->current->node_type = xml_state_leave;
            }
            return 0;
        } else {
            _fxml_node_t *new_n;
            //printf( "BRANCH       xpath[%s] nodename[%.*s] properties[%.*s]\n", xpath, nodename_len, nodename , properties_len , properties );
            if ( xpath_len == 4 && xpath[0]=='/' && tolower(xpath[1])=='x'
                && tolower(xpath[2])=='m' && tolower(xpath[3]=='l') )
                return 0;

            new_n = (_fxml_node_t *)malloc(sizeof(_fxml_node_t));
            new_n->name = nodename;
            new_n->name_len = nodename_len;
            new_n->parent = NULL;
            new_n->next = NULL;
            new_n->first_child = NULL;
            new_n->attr = NULL;
            new_n->node_type = xml_state_branch;

            if(h->current == NULL) {
                h->current = new_n;
                h->root = new_n;
            } else if(h->current->node_type != xml_state_enter){
                h->current->next = new_n;
                new_n->parent = h->current->parent;
                h->current = new_n;
            } else {
                h->current->first_child = new_n;
                new_n->parent = h->current;
                h->current = new_n;
            }
        }
    } else if( type & FASTERXML_NODE_LEAF ) {
        _fxml_node_t *new_n;
        //printf( "LEAF         xpath[%s] nodename[%.*s] properties[%.*s] content[%.*s]\n", xpath, nodename_len , nodename , properties_len , properties , content_len , content );

        new_n = (_fxml_node_t *)malloc(sizeof(_fxml_node_t));
        new_n->name = nodename;
        new_n->name_len = nodename_len;
        new_n->parent = NULL;
        new_n->next = NULL;
        new_n->first_child = NULL;
        new_n->attr = NULL;
        new_n->node_type = xml_state_leaf;

        if(h->current == NULL) {
            h->current = new_n;
            h->root = new_n;
        } else if(h->current->node_type != xml_state_enter){
            h->current->next = new_n;
            new_n->parent = h->current->parent;
            h->current = new_n;
        } else {
            h->current->first_child = new_n;
            new_n->parent = h->current;
            h->current = new_n;
        }
    }

    if( properties && properties[0] ) {
        nret = TravelXmlPropertiesBuffer(properties, properties_len, type, xpath, xpath_len, xpath_size, content, content_len, _on_xml_property, p) ;
        if ( nret )
            return nret;
    }

    return 0;
}

int parse_xml(fxml_node_t** root, char* buff) {
    char	xpath[ 1024 + 1 ] ;
    int     ret = 0 ;
    struct parse_info p = {NULL, NULL};

    memset( xpath, 0x00, sizeof(xpath) );
    ret = TravelXmlBuffer( buff, xpath, sizeof(xpath), _on_xml_node, &p) ;
    if( ret && ret != FASTERXML_ERROR_END_OF_BUFFER ) {
        printf( "TravelXmlTree failed[%d]\n", ret );
        return ret;
    }
    *root = p.root;
    return ret;
}

void free_faster_node(fxml_node_t* root) {
    if(!root)
        return;

    while(root->attr) {
        struct fxml_attr_s *attr = root->attr;
        root->attr = root->attr->next;
        free(attr);
    }
    free_faster_node(root->first_child);
    free_faster_node(root->next);
    free(root);
}

int parse_strcasecmp(char* buff, int len, char* str) {
    int str_len = strlen(str);
    if(len == str_len && !strncasecmp(buff, str, str_len))
        return 0;

    return 1;
}

int parse_xmlnode_namecmp(fxml_node_t* node, char* str) {
    return parse_strcasecmp(node->name, node->name_len, str);
}

int parse_xmlattr_namecmp(fxml_attr_t* attr, char* str) {
    return parse_strcasecmp(attr->name, attr->name_len, str);
}

int parse_xmlattr_valuecmp(fxml_attr_t* attr, char* str){
    return parse_strcasecmp(attr->value, attr->value_len, str);
}