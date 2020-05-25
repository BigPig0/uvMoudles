#include "uvlogprivate.h"

namespace uvLogPlus {

    RollingFileAppender::RollingFileAppender() {
        max = 10;
        policies.size_policy.size = 1024*1024;
        policies.time_policy.interval = 0;
        policies.time_policy.modulate = false;
        opening = false;
        opened = false;
    }

    RollingFileAppender::~RollingFileAppender() {

    }

    void RollingFileAppender::Init(uv_loop_t *uv) {
        uv_loop = uv;
        if(!opened && !opening){
        }
    }

    void RollingFileAppender::Write() {

    }

}