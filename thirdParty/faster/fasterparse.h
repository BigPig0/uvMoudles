#ifndef _H_FASTER_PARSE
#define _H_FASTER_PARSE

#ifdef __cplusplus
extern "C" {
#endif

#if ( defined _WIN32 )
#ifndef _THIRD_UTIL_API
#ifdef THIRD_UTIL_EXPORT
#define _THIRD_UTIL_API		_declspec(dllexport)
#else
#define _THIRD_UTIL_API		extern
#endif
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _THIRD_UTIL_API
#define _THIRD_UTIL_API		extern
#endif
#endif

typedef struct fxml_attr_s {
    struct fxml_attr_s *next;    //下一个属性，NULL表示结束
    char   *name;       //属性名称
    int    name_len;    //属性名称长度
    char   *value;      //属性值
    int    value_len;   //属性值长度
}fxml_attr_t;

typedef struct fxml_node_s {
    struct fxml_attr_s *attr;    //节点属性
    struct fxml_node_s  *parent;      //上级节点
    struct fxml_node_s  *next;        //下级节点
    struct fxml_node_s  *first_child; //子节点
    char   *name;       //节点名称
    int    name_len;    //节点名称长度
    char   *content;    //叶子节点内容
    int    content_len;
}fxml_node_t;


_THIRD_UTIL_API int parse_json(fxml_node_t** root, char* buff);

_THIRD_UTIL_API int parse_xml(fxml_node_t** root, char* buff);

_THIRD_UTIL_API void free_faster_node(fxml_node_t* root);

_THIRD_UTIL_API int parse_strcasecmp(char* buff, int len, char* str);

_THIRD_UTIL_API int parse_xmlnode_namecmp(fxml_node_t* node, char* str);

_THIRD_UTIL_API int parse_xmlattr_namecmp(fxml_attr_t* attr, char* str);

_THIRD_UTIL_API int parse_xmlattr_valuecmp(fxml_attr_t* attr, char* str);

#ifdef __cplusplus
}
#endif
#endif