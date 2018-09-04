
const char* _err_message_[] = {
    "ok",
	"It's not http protocol",
	"Failed parse the dns",
    "Cannot connect"
};

const int _err_message_len = sizeof(_err_message_) / sizeof(_err_message_[0]);

const char* uvhttp_err_msg(int err) {
	if (err >= _err_message_len || err < 0)
		return "you put a invalid error code";
	else
		return _err_message_[err];
}