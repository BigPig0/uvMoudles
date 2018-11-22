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

typedef struct faster_attribute_s {
    char   *name;       //属性名称
    int    name_len;    //属性名称长度
    char   *value;      //属性值
    int    value_len;   //属性值长度
    struct faster_attribute_s *next;    //下一个属性，NULL表示结束
}faster_attribute_t;

typedef struct faster_node_s {
    char   *name;       //节点名称
    int    name_len;    //节点名称长度
    struct faster_attribute_s *attr;    //节点属性
    struct faster_node_s  *parent;      //上级节点
    struct faster_node_s  *next;        //下级节点
    struct faster_node_s  *first_child; //子节点
}faster_node_t;


_THIRD_UTIL_API int parse_json(faster_node_t** root, char* buff);

_THIRD_UTIL_API int parse_xml(faster_node_t** root, char* buff);

_THIRD_UTIL_API void free_faster_node(faster_node_t* root);

#ifdef __cplusplus
}
#endif
#endif