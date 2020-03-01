#include "uvlogconf.h"
#include "pugixml.hpp"
#include "utilc_api.h"

namespace uvLogPlus {
    static Configuration* ConfigParseJsonBuff(char* conf_buff) {
        Configuration* ret = new Configuration;
        return ret;
    }

    static Configuration* ConfigParseXmlBuff(char* conf_buff) {
        Configuration* ret = NULL;
        pugi::xml_document doc;
        do
        {
            if (!doc.load(conf_buff)) {
                break;
            }

            // 根节点
            pugi::xml_node root = doc.child("configuration");
            if (!root)
                break;
            ret = new Configuration;

            //appenders节点
            pugi::xml_node node_appenders = root.child("appenders");
            if(node_appenders) {
                for (pugi::xml_node node = node_appenders.first_child(); node; node = node.next_sibling()) {
                    std::string nodeName  = node.name();
                    if(!strcasecmp(nodeName.c_str(), "console")) {
                        ConsolAppender *appender = new ConsolAppender();
                        for(pugi::xml_node::attribute_iterator iter = node.attributes_begin();
                            iter != node.attributes_end(); ++iter){
                                if(!strcasecmp(iter->name(),"name"))
                                    appender->name = iter->value();
                                else if(!strcasecmp(iter->name(),"target")) {
                                    if(!strncasecmp(iter->value(),"SYSTEM_ERR", 10))
                                        appender->target = ConsolTarget::SYSTEM_ERR;
                                }
                        }
                        ret->appenders.insert(make_pair(appender->name, appender));
                    } else if(!strcasecmp(nodeName.c_str(), "File")) {
                        FileAppender *appender = new FileAppender();
                        for(pugi::xml_node::attribute_iterator iter = node.attributes_begin();
                            iter != node.attributes_end(); ++iter){
                                if(!strcasecmp(iter->name(),"name"))
                                    appender->name = iter->value();
                                else if(!strcasecmp(iter->name(),"fileName"))
                                    appender->file_name = iter->value();
                                else if(!strcasecmp(iter->name(),"append"))
                                    if(!strcasecmp(iter->value(),"true") || !strcasecmp(iter->value(),"yes") || !strcasecmp(iter->value(),"1"))
                                        appender->append = true;
                        }
                        ret->appenders.insert(make_pair(appender->name, appender));
                    } else if(!strcasecmp(nodeName.c_str(), "RollingFile")) {
                        RollingFileAppender *appender = new RollingFileAppender();
                        for(pugi::xml_node::attribute_iterator iter = node.attributes_begin();
                            iter != node.attributes_end(); ++iter){
                                if(!strcasecmp(iter->name(),"name"))
                                    appender->name = iter->value();
                                else if(!strcasecmp(iter->name(),"fileName"))
                                    appender->file_name = iter->value();
                                else if(!strcasecmp(iter->name(),"filePattern"))
                                    appender->filePattern = iter->value();
                        }
                        ret->appenders.insert(make_pair(appender->name, appender));
                    }
                }
            }

            //loggers节点
            pugi::xml_node node_loggers = root.child("loggers");

         }while(0);
        return ret;
    }

    Configuration* ConfigParse(char *buff) {
        Configuration* conf = ConfigParseJsonBuff(buff);
        if(NULL == conf)
            conf = ConfigParseXmlBuff(buff);
        return conf;
    }

    Configuration* ConfigParse(uv_file file) {
        char *base = (char*)calloc(1024,1024);
        uv_buf_t buffer = uv_buf_init(base, 1024*1024);
        uv_fs_t read_req;
        int ret = uv_fs_read(NULL, &read_req, file, &buffer, 1, -1, NULL);
        if(ret < 0) {
            printf("uv fs read failed:%s\n",uv_strerror(ret));
            return NULL;
        }

        return ConfigParse(base);
    }

    Configuration* ConfigParse(std::string path) {
        uv_fs_t open_req;
        int ret = uv_fs_open(NULL, &open_req, path.c_str(), O_RDONLY, 0, NULL);
        if(ret < 0) {
            printf("uv fs open %s failed:%s\n", path.c_str(), uv_strerror(ret));
            return NULL;
        }
        Configuration* conf = ConfigParse(open_req.result);
        uv_fs_close(NULL, &open_req, open_req.result, NULL);

        return conf;
    }
}